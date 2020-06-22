#pragma once
#include <stdexcept>

/**
 * \brief The SnowShooter Error class for any game-related error.
 */
class SnowShooterError : public std::logic_error {
 public:
  /**
   * \brief Create a SnowShooter Error instance.
   * \param message The error message description.
   */
  explicit SnowShooterError(const std::string& message)
      : std::logic_error("SnowShooter Error " + message) {}
};