#pragma once

//-----------------------------------------------------------------------------
// System Includes
//-----------------------------------------------------------------------------
#pragma region

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#pragma endregion

//-----------------------------------------------------------------------------
// Declarations and Definitions
//-----------------------------------------------------------------------------
namespace smallpt {

  //-------------------------------------------------------------------------
  // Constants
  //-------------------------------------------------------------------------

  constexpr double g_pi = 3.14159265358979323846;

  //-------------------------------------------------------------------------
  // Utilities
  //-------------------------------------------------------------------------

  template <class T>
  [[nodiscard]]
  constexpr T clamp(const T& v, const T& lo, const T& hi) noexcept {
    return (v < lo) ? lo : (hi < v) ? hi : v;
  }

  template <class T, class Compare>
  [[nodiscard]]
  constexpr T clamp(const T& v, const T& lo, const T& hi, Compare comp) noexcept {
    return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
  }

  [[nodiscard]]
  inline std::uint8_t ToByte(double color, double gamma = 2.2) noexcept {
    const double gcolor = std::pow(color, 1.0 / gamma);
    return static_cast<std::uint8_t>(clamp(255.0 * gcolor, 0.0, 255.0));
  }

  inline double FromByte(std::uint8_t byte, double gamma = 2.2) noexcept {
    double gcolor = byte / 255.0;
    return std::pow(gcolor, gamma);
  }
} // namespace smallpt
