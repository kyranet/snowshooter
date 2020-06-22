#include "Game.h"

#include "Constants.h"
#include "FontManager.h"
#include "MenuScene.h"
#include "SDLAudioManager.h"
#include "SDLError.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"
#include "Scene.h"
#include "SceneMachine.h"
#include "Texture.h"
#include "TextureManager.h"

Game::Game() {
  if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
    const auto message =
        std::string("Error initializing SDL.\nReason: ") + SDL_GetError();
    throw SDLError(message);
  }

  if (TTF_Init() != 0) {
    const auto message =
        std::string("Error initializing TTF.\nReason: ") + TTF_GetError();
    throw SDLError(message);
  }

  if (Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG) == 0) {
    const auto message =
        std::string("Error initializing MIX.\nReason: ") + Mix_GetError();
    throw SDLError(message);
  }

  // Create the window and renderer
  window_ = SDL_CreateWindow("SnowShooter", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, WIN_WIDTH, WIN_HEIGHT,
                             SDL_WINDOW_SHOWN);
  renderer_ = SDL_CreateRenderer(
      window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  // If window or renderer is a null pointer, throw a SDLError
  if (window_ == nullptr || renderer_ == nullptr)
    throw SDLError("Error loading the SDL window or renderer");
}

void Game::load() {
  if (loaded_) throw SnowShooterError("Cannot run Game::load twice.");
  loaded_ = true;

  auto textures = TextureManager::getInstance();
  textures->init();

  auto fonts = FontManager::getInstance();
  fonts->init();

  auto audio = SDLAudioManager::getInstance();
  audio->init();

  sceneMachine_ = new SceneMachine();
  sceneMachine_->pushScene(new MenuScene());
}

Game::~Game() {
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);

  delete sceneMachine_;

  SDL_Quit();
  TTF_Quit();
  Mix_Quit();
}

void Game::run() const {
  while (!sceneMachine_->isEmpty()) {
    sceneMachine_->getCurrentScene()->run();
  }
}

SDL_Renderer* Game::getRenderer() { return getInstance()->renderer_; }

Game* Game::instance_ = nullptr;

Game* Game::getInstance() {
  if (instance_ == nullptr) {
    instance_ = new Game();
  }

  return instance_;
}

SceneMachine* Game::getSceneMachine() { return getInstance()->sceneMachine_; }

SDL_Window* Game::getWindow() { return getInstance()->window_; }