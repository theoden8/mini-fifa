#pragma once

#include "Logger.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

// is affine
struct Transformation {
  glm::mat4 translation;
  glm::mat4 rotation;
  glm::mat4 scaling;
  bool has_changed = true;

  Transformation():
    translation(1.f), rotation(1.f), scaling(1.f)
  {}

  Transformation(glm::mat4 translation, glm::mat4 rotation, glm::mat4 scaling):
    translation(translation),
    rotation(rotation),
    scaling(scaling)
  {}

  Transformation operator*(const Transformation &other) {
    return Transformation(
      translation * other.translation,
      rotation * other.rotation,
      scaling * other.scaling
    );
  }

  decltype(auto) get_matrix_const() const {
    return translation * rotation * scaling;
  }

  decltype(auto) get_matrix() {
    return get_matrix_const();
  }

  decltype(auto) inverse() {
    return glm::affineInverse(get_matrix());
  }

  void SetPosition(float x, float y, float z) {
    translation = glm::translate(glm::vec3(x, y, z));
    has_changed = true;
  }

  void SetPosition(glm::vec3 p) {
    SetPosition(p.x, p.y, p.z);
  }

  void MovePosition(float x, float y, float z) {
    translation = glm::translate(glm::vec3(x, y, z)) * translation;
    has_changed = true;
  }

  void MovePosition(glm::vec3 p) {
    MovePosition(p.x, p.y, p.z);
  }

  glm::vec4 GetPosition() const {
    return translation * glm::vec4(0.f);
  }

  void Rotate(float x, float y, float z, float deg) {
    rotation = glm::rotate(glm::radians(deg), glm::vec3(x, y, z)) * rotation;
    has_changed = true;
  }

  void SetRotation(float x, float y, float z, float deg) {
    rotation = glm::rotate(glm::radians(deg), glm::vec3(x, y, z));
    has_changed = true;
  }

  void Scale(float scale) {
    scaling = glm::scale(glm::vec3(scale, scale, scale)) * scaling;
    has_changed = true;
  }

  void Scale(float sx, float sy, float sz) {
    scaling = glm::scale(glm::vec3(sx, sy, sz)) * scaling;
    has_changed = true;
  }

  void SetScale(float scale) {
    scaling = glm::scale(glm::vec3(scale, scale, scale));
    has_changed = true;
  }

  void SetScale(float sx, float sy, float sz) {
    scaling = glm::scale(glm::vec3(sx, sy, sz));
    has_changed = true;
  }

  void SetScale(glm::vec3 s) {
    SetScale(s.x, s.y, s.z);
  }

  glm::vec3 GetScale() const {
    return glm::vec3(scaling * glm::vec4(1.f));
  }

  void print() {
    glm::mat4 m = get_matrix_const();
    printf("%.6f %.6f %.6f %.6f\n", m[0][0], m[0][1], m[0][2], m[0][3]);
    printf("%.6f %.6f %.6f %.6f\n", m[1][0], m[1][1], m[1][2], m[1][3]);
    printf("%.6f %.6f %.6f %.6f\n", m[2][0], m[2][1], m[2][2], m[2][3]);
    printf("%.6f %.6f %.6f %.6f\n", m[3][0], m[3][1], m[3][2], m[3][3]);
  }
};
