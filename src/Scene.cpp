#include "Scene.h"

#include <utility>

#include "Constants.h"
#include "Game.h"
#include "GameObject.h"
#include "Input.h"
#include "SDL.h"
#include "SDLError.h"
#include "SceneMachine.h"
#include "TextureManager.h"
#include "TimePool.h"

Scene::Scene() = default;
Scene::~Scene() = default;

bool Scene::getTransition() const { return transition_; }

bool Scene::getGame() const { return game_; }

void Scene::setTransition(const bool transition) { transition_ = transition; }

void Scene::setGame(bool game) { game_ = game; }

void Scene::setOnEndHandler(std::function<void()> callback) {
  onEndHandler_ = std::move(callback);
}

void Scene::activateTutorialBox() {}

bool Scene::isFinished() const { return finished_; }
bool Scene::isRunning() const { return !paused_; }
bool Scene::isPaused() const { return paused_; }
bool Scene::isLoaded() const { return loaded_; }

void Scene::run() {
  // If it hasn't loaded yet, preload and set it to loaded
  if (!isLoaded()) {
    loaded_ = true;
  }

  // If it's already running, don't re-run the event loop twice
  if (isRunning()) return;

  // Set current status to resume
  resume();

  const auto textureManager = TextureManager::getInstance();
  auto poolFrameRate = new TimePool(1000 / GAME_FRAMERATE, SDL_GetTicks());

  // Run the event loop
  while (!isFinished()) {
    textureManager->tick();

    create();
    handleEvents();
    update();
    tick();
    destroy();
    render();

    if (poolFrameRate->next(SDL_GetTicks())) {
      SDL_Delay(poolFrameRate->getRemaining());
    }
  }

  delete poolFrameRate;

  end();
}

void Scene::tick() {
  if (nextTick_.empty()) return;

  for (const auto& callback : nextTick_) {
    callback();
  }
  nextTick_.clear();
}

void Scene::create() {
  for (auto gameObject : pendingOnCreate_) {
    // If the gameObject was removed early, skip
    if (gameObject == nullptr) continue;
    gameObjects_.push_back(gameObject);
    if (gameObject->getActive()) gameObject->awake();
  }

  pendingOnCreate_.clear();
}

void Scene::handleEvents() {
  // Clear the Input's cache
  Input::instance()->clear();

  // Handle all events
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) return finish(true);

    Input::instance()->update(event);
    for (auto gameObject : gameObjects_)
      if (gameObject->getActive()) gameObject->handleEvents(event);
  }

  // If the escape key was pressed, pause the game
  if (getGame() && Input::isKeyDown(KeyboardKey::ESCAPE)) {
    if (!isPaused()) {
      pause();
    } else {
      getGameObjects().back()->destroy();
      resume();
    }
  }

  // If the F key was pressed, toggle fullscreen
  if (Input::isKeyDown(KeyboardKey::F)) {
    SDL_Window* window_ = Game::getWindow();
    const auto flags = SDL_GetWindowFlags(window_);
    const auto flag = static_cast<Uint32>(
        flags & SDL_WINDOW_FULLSCREEN ? 0 : SDL_WINDOW_FULLSCREEN);
    if (SDL_SetWindowFullscreen(window_, flag) != 0) {
      throw SDLError("Failed to change full screen: " +
                     std::string(SDL_GetError()));
    }
  }
}

void Scene::update() {
  for (auto gameObject : gameObjects_)
    if (gameObject->getActive()) gameObject->update();
}

void Scene::render() {
  // Clear the screen
  SDL_RenderClear(Game::getRenderer());

  // Render each game object
  for (auto gameObject : gameObjects_) {
    gameObject->render();
  }

  // Render the new frame
  SDL_RenderPresent(Game::getRenderer());
}

void Scene::destroy() {
  for (auto gameObject : pendingOnDestroy_) {
    // If the gameObject was already deleted from memory,
    // skip this search
    if (gameObject == nullptr) continue;

    auto it = gameObjects_.begin();
    while (it != gameObjects_.end()) {
      if (*it == gameObject) {
        gameObjects_.erase(it);
        delete gameObject;
        break;
      }
      ++it;
    }
  }

  pendingOnDestroy_.clear();
}

void Scene::end() {
  // Delete the Scene's cache.
  for (auto gameObject : gameObjects_) {
    delete gameObject;
  }
  gameObjects_.clear();

  onEndHandler_();
}

void Scene::resume() {
  paused_ = false;
  for (const auto gameObject : getGameObjects()) gameObject->setActive(true);
}

void Scene::pause() {
  paused_ = true;
  for (const auto gameObject : getGameObjects()) gameObject->setActive(false);
}

void Scene::finish(bool force) {
  if (force) return Game::getSceneMachine()->popScene();

  finished_ = true;
  paused_ = true;
}

std::list<GameObject*> Scene::getGameObjects() const { return gameObjects_; }

void Scene::addGameObject(GameObject* gameObject) {
  pendingOnCreate_.push_back(gameObject);
}

void Scene::removeGameObject(GameObject* gameObject) {
  pendingOnDestroy_.push_back(gameObject);
}

void Scene::processNextTick(std::function<void()> callback) {
  nextTick_.push_back(callback);
}
