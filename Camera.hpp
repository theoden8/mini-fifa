#pragma once

#include <cstdio>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "incgraphics.h"
#include "Transformation.hpp"

struct Camera {
  bool has_changed = true;

  Camera()
  {}

  void update(float ratio) {
    /* cameraPos = cameraTarget + glm::vec3(0, std::cos(glm::radians(angle)), std::sin(glm::radians(angle))); */
    cameraPos = cameraTarget + glm::vec3(0, std::cos(glm::radians(angle)), std::sin(glm::radians(angle)));
    cameraDirection = glm::normalize(cameraPos - cameraTarget);
    cameraRight = glm::normalize(glm::cross(up, cameraDirection));
    cameraUp = glm::cross(cameraDirection, cameraRight);
    view = glm::lookAt(
      cameraPos,
      cameraTarget,
      up
    );
    projection = glm::infinitePerspective(
      glm::radians(fov),
      ratio,
      0.1f
    );
    /* * glm::ortho( */
    /*   -ratio, ratio, */
    /*   -ratio, ratio, */
    /*   std::cos(glm::radians(fov)), 0.f */
    /* ); */
  }

  glm::vec3 cameraPos;
  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cameraDirection;
  glm::vec3 up = glm::vec3(0, 0, 1);
  glm::vec3 cameraRight;
  glm::vec3 cameraUp;
  glm::mat4 view;
  glm::mat4 projection;
  float fov = 75.;
  float angle = 60.f;
  /* glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 0.0f); */
  /* glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f); */
  float cameraSpeedKeys = 0.05f; // adjust accordingly
  float cameraSpeedMouse = 0.02f; // adjust accordingly

  void move_up(float movespeed) {
    cameraTarget.y = std::fmax(cameraTarget.y - movespeed, -1.5); /* * cameraFront; */
    has_changed = true;
  }

  void move_down(float movespeed) {
    cameraTarget.y = std::fmin(cameraTarget.y + movespeed, 2); /* * cameraFront; */
    has_changed = true;
  }

  void move_left(float movespeed) {
    cameraTarget.x = std::fmin(cameraTarget.x + movespeed, 3); /* glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; */
    has_changed = true;
  }

  void move_right(float movespeed) {
    cameraTarget.x = std::fmax(cameraTarget.x - movespeed, -3); /* glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; */
    has_changed = true;
  }

  void keyboard(GLFWwindow *w, float ratio) {
    /* static float accel = 1.01; */
    if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS) {
      move_up(cameraSpeedKeys);
       // * cameraFront;
    } else if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS) {
      move_down(cameraSpeedKeys);
       // * cameraFront;
    } else if (glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS) {
      move_left(cameraSpeedKeys);
    } else if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      move_right(cameraSpeedKeys);
    } else if (glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS) {
    } else if (glfwGetKey(w, GLFW_KEY_EQUAL) == GLFW_PRESS) {
      fov = std::fmax<double>(fov - 1, 1);
      has_changed = true;
    } else if (glfwGetKey(w, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
      angle = std::fmin<double>(angle + 1, 89);
      has_changed = true;
    } else if (glfwGetKey(w, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
      angle = std::fmax<double>(angle - 1, 10);
      has_changed = true;
    }
    update(ratio);
  }

  void mouse(double x, double y) {
    return;
    double border = .02;
    if(x < border) {
      move_left(cameraSpeedMouse);
    } else if(x > 1.-border) {
      move_right(cameraSpeedMouse);
    }
    if(y < border) {
      move_up(cameraSpeedMouse);
    } else if(y > 1.-border) {
      move_down(cameraSpeedMouse);
    }
  }

  decltype(auto) get_matrix() {
    return projection * view;
  }
};
