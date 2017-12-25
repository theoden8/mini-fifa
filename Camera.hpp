#pragma once

#include <cstdio>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "incgraphics.h"
#include "Window.hpp"
#include "Transformation.hpp"

namespace gl {
struct Window;
}

struct Camera {
  /* glm::mat4 perspective; */
  /* glm::mat4 lookat; */
  /* glm::mat4 projection; */
  bool has_changed = true;

  Camera()
  {
    /* perspective = glm::perspective(glm::radians(45.0f), 1.33f, 0.1f, 10.0f); */
  }
    /* WindowResize(w, h); */

  void init() {
    update();
  }

  void update(float ratio=1.33f) {
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
    zoom = glm::perspective(glm::radians(fov), ratio, 0.1f, 100.0f);
    /* lookat = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); */
  }

  glm::vec3 cameraPos;
  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cameraDirection;
  glm::vec3 up = glm::vec3(0, 0, 1);
  glm::vec3 cameraRight;
  glm::vec3 cameraUp;
  glm::mat4 view;
  glm::mat4 zoom;
  float fov = 75.;
  float angle = 60.f;
  /* glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 0.0f); */
  /* glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f); */
  float cameraSpeed = 0.05f; // adjust accordingly
  void move_up() { cameraTarget.y = std::max<float>(cameraTarget.y - cameraSpeed, -1.5); /* * cameraFront; */ }
  void move_down() { cameraTarget.y = std::min<float>(cameraTarget.y + cameraSpeed, 2); /* * cameraFront; */ }
  void move_left() { cameraTarget.x = std::min<float>(cameraTarget.x + cameraSpeed, 3); /* glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; */ }
  void move_right() { cameraTarget.x = std::max<float>(cameraTarget.x - cameraSpeed, -3); /* glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed; */ }
  void keyboard(GLFWwindow *w, float ratio) {
    /* static float accel = 1.01; */
    if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS) {
      move_up();
       // * cameraFront;
    } else if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS) {
      move_down();
       // * cameraFront;
    } else if (glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS) {
      move_left();
    } else if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      move_right();
    } else if (glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS) {
      fov = std::fmin<double>(fov + 1, 150);
    } else if (glfwGetKey(w, GLFW_KEY_EQUAL) == GLFW_PRESS) {
      fov = std::fmax<double>(fov - 1, 1);
    } else if (glfwGetKey(w, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
      angle = std::fmin<double>(angle + 1, 89);
    } else if (glfwGetKey(w, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
      angle = std::fmax<double>(angle - 1, 10);
    }
    update(ratio);
  }

  void mouse(double x, double y) {
    double border = .05;
    if(x < border) {
      move_left();
    } else if(x > 1.-border) {
      move_right();
    }
    if(y < border) {
      move_up();
    } else if(y > 1.-border) {
      move_down();
    }
  }

  void WindowResize(float new_width, float new_height) {
    /* glm::vec3 target = transform.GetPosition(); */
    /* lookat = glm::lookAt( */
    /*   target + glm::vec3(0, 0, 1), */
    /*   target, */
    /*   glm::vec3(0, 1, 0) */
    /* ); */
    /* if(new_width > new_height) { */
    /*   projection = glm::ortho(-new_width/new_height, new_width/new_height, -1.0f, 1.0f, 1.0f, -1.0f); */
    /* } else if(new_width <= new_height) { */
    /*   projection = glm::ortho(-1.0f, 1.0f, -new_height/new_width, new_height/new_width, 1.0f, -1.0f); */
    /* } */
  }

  decltype(auto) get_matrix() {
    return zoom * view;
  }

  void clear() {
  }
};
