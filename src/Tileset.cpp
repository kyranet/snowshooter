#include "Tileset.h"

#include <fstream>
#include <sstream>

#include "SnowShooterError.h"
#include "Texture.h"

void Tileset::load(const std::string& path, const int size, Texture* texture) {
  tiles_.clear();
  size_ = size;
  texture_ = texture;

  // Create an input filestream
  std::ifstream myFile(path);

  // Make sure the file is open
  if (!myFile.is_open()) throw new SnowShooterError("Could not open file");

  // Helper vars
  std::string line;
  int16_t val;

  uint16_t x = 0, y = 0;
  // Read data, line by line
  while (std::getline(myFile, line)) {
    // Create a stringstream of the current line
    std::stringstream ss(line);

    // Extract each integer
    while (ss >> val) {
      // If the next token is a comma, ignore it and move on
      if (ss.peek() == ',') ss.ignore();

      if (val != -1) {
        const auto position =
            texture->getFramePosition(static_cast<uint16_t>(val));
        tiles_.push_back(
            {val,
             {x * size, y * size},
             {position.getX() * size, position.getY() * size, size, size}});
      }

      ++x;
    }

    ++y;
    x = 0;
  }

  // Close file
  myFile.close();
}

void Tileset::render() const {
  for (const auto tile : tiles_) {
    const auto position = tile.offset + getPosition();
    texture_->render({position.getY(), position.getY(), size_, size_}, 0.0,
                     &tile.region);
  }
}
