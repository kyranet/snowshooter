#pragma once
#include "GameObject.h"

class Texture;

class Tileset final : public GameObject {
  struct Tile {
    int16_t id;
    Vector2D<int> offset;
    SDL_Rect region;
  };

  std::vector<Tile> tiles_ = {};
  int size_ = 0;
  Texture* texture_ = nullptr;

 public:
  void load(const std::string& path, const int size, Texture* texture);
  void render() const;
};
