#include "GameManager.h"

GameManager* GameManager::instance_ = nullptr;

GameManager::GameManager() = default;
GameManager::~GameManager() = default;

GameManager* GameManager::getInstance() {
  if (instance_ == nullptr) instance_ = new GameManager();
  return instance_;
}

void GameManager::destroy() {
  if (instance_ != nullptr) {
    delete instance_;
    instance_ = nullptr;
  }
}

void GameManager::reset() {}
