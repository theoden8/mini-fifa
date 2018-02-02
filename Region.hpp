#pragma once

#include <string>
#include <glm/glm.hpp>

struct Region {
  glm::vec2 xs, ys;
  Region(glm::vec2 xs, glm::vec2 ys):
    xs(xs), ys(ys)
  {
    if(xs.x > xs.y)std::swap(xs.x, xs.y);
    if(ys.x > ys.y)std::swap(ys.x, ys.y);
  }

  bool contains(float x, float y) const {
    return
      xs.x <= x && x <= xs.y
      && ys.x <= y && y <= ys.y;
  }
  bool contains(const glm::vec2 &other) const {
    return contains(other.x, other.y);
  }

  glm::vec2 center() const {
    return {
      xs.x + (xs.y - xs.x) / 2,
      ys.x + (ys.y - ys.x) / 2
    };
  }

  glm::vec2 x() const { return xs; }
  glm::vec2 y() const { return ys; }
  float x1() const { return xs.x; }
  float &x1() { return xs.x; }
  float x2() const { return xs.y; }
  float &x2() { return xs.y; }
  float y1() const { return ys.x; }
  float &y1() { return ys.x; }
  float y2() const { return ys.y; }
  float &y2() { return ys.y; }

  std::string str() const {
    return "X[" + std::to_string(xs.x) + " .. " + std::to_string(xs.y) + "] Y[" + std::to_string(ys.x) + std::to_string(ys.y) + "]";
  }
};
