#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"

struct Player {
// opengl stuff
  glm::mat4 matrix;
  glm::mat4 extra_rotate;
  Transformation transform;

	Model playerModel;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;

  Player(std::pair<float, float> pos={0, 0}):
    uTransform("transform"),
    playerModel("assets/nanosuit/nanosuit.obj"),
    program({"player.vert", "player.frag"})
  {
    dest = glm::vec2(pos.first, pos.second);
    cur_pos = glm::vec3(pos.first, pos.second, 0);
    transform.SetScale(.01, .01, .01);
    transform.SetPosition(cur_pos.x, cur_pos.y, 0);
    extra_rotate =
      glm::rotate(glm::radians(90.f), glm::vec3(0, 0, 1))
      * glm::rotate(glm::radians(90.f), glm::vec3(1, 0, 0));
    transform.rotation = extra_rotate;
  }

  void init() {
    program.compile_program();
    playerModel.init();
    uTransform.set_id(program.id());
  }

  void Keyboard(GLFWwindow *w) {
    if(glfwGetKey(w, GLFW_KEY_LEFT)) {
      dest += glm::vec2(.05, 0);
    } else if(glfwGetKey(w, GLFW_KEY_RIGHT)) {
      dest -= glm::vec2(.05, 0);
    } else if(glfwGetKey(w, GLFW_KEY_UP)) {
      dest -= glm::vec2(0, .05);
    } else if(glfwGetKey(w, GLFW_KEY_DOWN)) {
      dest += glm::vec2(0, .05);
    } else if(glfwGetKey(w, GLFW_KEY_V)) {
      jump();
    }
  }

  void display(Camera &cam) {
    idle();

    program.use();

    if(cam.has_changed || transform.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    playerModel.display(program);

    decltype(program)::unuse();
  }

  void clear() {
    playerModel.clear();
    program.clear();
  }

// gameplay
  double speed = 0.001;
  double control_range = .008;
  glm::vec2 dest = glm::vec2(0, 0);
  glm::vec3 cur_pos = glm::vec3(0, 0, 0);
  bool has_ball = false;
  double lost_ball = 0.;
  double last_jump = 0.;
  double last_slide = 0.;

  void set_pos(float x, float y) {
    cur_pos.x = x, cur_pos.y = y;
  }

  void set_dest(float x, float y) {
    dest = glm::vec2(x, y);
  }

  void lose_ball() {
    has_ball = false;
    lost_ball = glfwGetTime();
  }

  bool can_tackle() {
    return glfwGetTime() - lost_ball < 1.5;
  }

  void idle() {
    run();
    transform.SetPosition(cur_pos.x, cur_pos.y, cur_pos.z);
    // jumping height
    double curtime = glfwGetTime();
    double max_height = .1;
    double diff = curtime = last_jump;
    if(diff < 3) {
      if(diff < 1.5) {
        cur_pos.z = max_height * (diff / 1.5);
      } else {
        cur_pos.z = max_height * (diff / 1.5);
      }
    }
  }

  void jump() {
    if(glfwGetTime() - last_jump < 3) {
      return;
    }
    last_jump = glfwGetTime();
  }

  void run() {
    glm::vec2 pos_xy(cur_pos.x, cur_pos.y);
    if(glm::length(dest - pos_xy) > .001) {
      auto direction = dest - pos_xy;
      direction *= speed / glm::length(direction);
      cur_pos += glm::vec3(direction.x, direction.y, 0);
      float facing = atan2(direction.y, direction.x) * (180 / M_PI);
      transform.rotation = extra_rotate;
      transform.Rotate(0, 0, 1, facing);
    }
  }
};
