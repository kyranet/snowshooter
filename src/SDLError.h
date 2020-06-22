#pragma once
#include "SnowShooterError.h"

/**
 * \brief The SDL error class for any operative error related to the SDL2
 * library.
 */
class SDLError final : public SnowShooterError {
 public:
  /**
   * \brief Create a SDL Error instance.
   * \param message The error message description.
   */
  explicit SDLError(const std::string& message)
      : SnowShooterError("[SDL]: " + message) {}
};