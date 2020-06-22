#pragma once
#include <map>
#include <string>
#include <vector>

class GameManager final {
  static GameManager* instance_;

  GameManager();
  ~GameManager();

 public:
  static GameManager* getInstance();
  static void destroy();
  void reset();
};