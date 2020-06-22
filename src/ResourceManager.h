#pragma once
#include <SDL.h>

#include <map>

template <class V>
class ResourceManager {
 protected:
  std::map<std::string, V> map_;
};