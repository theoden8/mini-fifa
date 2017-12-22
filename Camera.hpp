#pragma once

#include "incgraphics.h"
#include "Transformation.hpp"

#include <glm/gtc/matrix_transform.hpp>

struct Camera {
  /* glm::mat4 perspective; */
  /* glm::mat4 lookat; */
  /* glm::mat4 projection; */
  bool has_changed = true;

  Camera() {
    /* perspective = glm::perspective(glm::radians(45.0f), 1.33f, 0.1f, 10.0f); */
  }
    /* WindowResize(w, h); */

  void init() {
  }

  glm::vec3 cameraPos;
  glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cameraDirection;
  glm::vec3 up = glm::vec3(0, 0, 1);
  glm::vec3 cameraRight;
  glm::vec3 cameraUp;
  glm::mat4 view;
  glm::mat4 zoom;
  float fov = 20.;
  /* glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 0.0f); */
  /* glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f); */
  void Keyboard(GLFWwindow *w) {
    /* static float accel = 1.01; */
    float cameraSpeed = 0.05f; // adjust accordingly
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) {
      cameraTarget.y -= cameraSpeed; // * cameraFront;
    } else if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) {
      cameraTarget.y += cameraSpeed; // * cameraFront;
    } else if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) {
      cameraTarget.x += cameraSpeed; // glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    } else if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) {
      cameraTarget.x -= cameraSpeed; // glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    } else if (glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS) {
      fov = std::fmin<double>(fov + 1, 60);
    } else if (glfwGetKey(w, GLFW_KEY_EQUAL) == GLFW_PRESS) {
      fov = std::fmax<double>(fov - 1, 1);
    }
    cameraPos = cameraTarget + glm::vec3(0, 1, 1.5);
    cameraDirection = glm::normalize(cameraPos - cameraTarget);
    cameraRight = glm::normalize(glm::cross(up, cameraDirection));
    cameraUp = glm::cross(cameraDirection, cameraRight);
    view = glm::lookAt(
      cameraPos,
      cameraTarget,
      up
    );
    zoom = glm::perspective(glm::radians(fov), 1.33f, 0.1f, 100.0f);
    /* lookat = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp); */
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
