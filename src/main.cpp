#include <iostream>

#include "FontManager.h"
#include "Game.h"
#include "GameManager.h"
#include "Input.h"
#include "SDL.h"
#include "SDLAudioManager.h"
#include "TextureManager.h"

#undef main

int main(int, char*[]) {
#if _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
                 _CRTDBG_LEAK_CHECK_DF);  // Check Memory Leaks
#endif
  try {
    const auto game = Game::getInstance();
    game->load();
    game->run();
    SDLAudioManager::destroy();
    TextureManager::destroy();
    FontManager::destroy();
    GameManager::destroy();
    Input::destroy();
    delete game;
    return 0;
  } catch (std::exception& e) {
    std::cerr << e.what();
    return 1;
  }
}