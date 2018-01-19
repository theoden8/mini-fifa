#pragma once

#include <string>
#include <glm/glm.hpp>

struct Region {
  glm::vec2 xs, ys;
  constexpr Region(glm::vec2 xs, glm::vec2 ys):
    xs(xs), ys(ys)
  {
    if(xs.x > xs.y)std::swap(xs.x, xs.y);
    if(ys.x > ys.y)std::swap(ys.x, ys.y);
  }

  constexpr bool contains(float x, float y) const {
    return
      xs.x <= x && x <= xs.y
      && ys.x <= y && y <= ys.y;
  }
  constexpr bool contains(const glm::vec2 &other) const {
    return contains(other.x, other.y);
  }

  constexpr glm::vec2 center() const {
    return glm::vec2(
      xs.x + (xs.y - xs.x) / 2,
      ys.x + (ys.y - ys.x) / 2
    );
  }

  constexpr glm::vec2 x() const { return xs; }
  constexpr glm::vec2 y() const { return ys; }
  constexpr float x1() const { return xs.x; }
  float &x1() { return xs.x; }
  constexpr float x2() const { return xs.y; }
  float &x2() { return xs.y; }
  constexpr float y1() const { return ys.x; }
  float &y1() { return ys.x; }
  constexpr float y2() const { return ys.y; }
  float &y2() { return ys.y; }

  std::string str() const {
    return "X[" + std::to_string(xs.x) + " .. " + std::to_string(xs.y) + "] Y[" + std::to_string(ys.x) + std::to_string(ys.y) + "]";
  }
};
