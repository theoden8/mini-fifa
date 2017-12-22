#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Transformation {
  glm::mat4 translation;
  glm::mat4 rotation;
  glm::mat4 scaling;
  bool has_changed = true;

  Transformation():
    translation(), rotation(), scaling()
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

  decltype(auto) get_matrix() {
    return translation * rotation * scaling;
  }

  void SetPosition(float x, float y, float z) {
    translation = glm::translate(glm::vec3(x, y, z));
    has_changed = true;
  }

  void MovePosition(float x, float y, float z) {
    translation = glm::translate(glm::vec3(x, y, z)) * translation;
    has_changed = true;
  }

  glm::vec4 GetPosition() const {
    return translation * glm::vec4(1.0f);
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
};
