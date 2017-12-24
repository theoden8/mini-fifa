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

  bool team;
  Player(bool team, std::pair<float, float> pos={0, 0}):
    team(team),
    uTransform("transform"),
    playerModel("assets/nanosuit/nanosuit.obj"),
    program({"player.vert", "player.frag"})
  {
    dest = glm::vec2(pos.first, pos.second);
    cur_pos = glm::vec3(pos.first, pos.second, 0);
    transform.SetScale(.01);
    transform.Scale(1, .75, 1);
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
    double step = .3;
    if(glfwGetKey(w, GLFW_KEY_A)) {
      dest = glm::vec2(cur_pos.x + step, cur_pos.y);
    } else if(glfwGetKey(w, GLFW_KEY_D)) {
      dest = glm::vec2(cur_pos.x - step, cur_pos.y);
    } else if(glfwGetKey(w, GLFW_KEY_W)) {
      dest = glm::vec2(cur_pos.x, cur_pos.y - step);
    } else if(glfwGetKey(w, GLFW_KEY_S)) {
      dest = glm::vec2(cur_pos.x, cur_pos.y + step);
    } else if(glfwGetKey(w, GLFW_KEY_H)) {
      dest = cur_pos;
    } else if(glfwGetKey(w, GLFW_KEY_V)) {
      jump();
    }
  }

  void display(Camera &cam) {
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
  double current_time = 0.;
  double running_speed = 0.002;
  double control_range = .07;
  double control_time_diff = 1.1;
  glm::vec2 dest = glm::vec2(0, 0);
  glm::vec3 cur_pos = glm::vec3(0, 0, 0);
  float facing = 0.;
  bool has_ball = false;
  double time_lost_ball = 0.;
  double time_last_jump = 0.;
  double jump_reload = 3.;
  double max_jump_height = .1;
  double jump_period = 1.75;
  double time_last_slide = 0.;
  double slide_reload = 1.5;
  double sliding_speed = 2 * running_speed;

  void set_pos(float x, float y) {
    cur_pos.x = x, cur_pos.y = y;
  }

  void set_dest(float x, float y) {
    dest = glm::vec2(x, y);
  }

  void lose_ball() {
    has_ball = false;
    time_lost_ball = glfwGetTime();
  }

  glm::vec2 direction() const {
    return glm::vec2(
      std::cos(facing),
      std::sin(facing)
    );
  }

  double possession_offset = .03;
  glm::vec3 possession_point() const {
    glm::vec2 dir = direction();
    return glm::vec3(
      cur_pos.x + dir.x * possession_offset,
      cur_pos.y + dir.y * possession_offset,
      cur_pos.z
    );
  }

  double tackle_value(const glm::vec3 &ball_position) const {
    if(current_time - time_lost_ball < control_time_diff) {
      return NAN;
    }
    double range = glm::length(ball_position - possession_point());
    if(range > control_range) return NAN;
    return range;
  }

  void idle(double curtime=NAN) {
    current_time = std::isnan(curtime) ? glfwGetTime() : curtime;
    set_jump_height();
    perform_slide();
    run();
    transform.SetPosition(cur_pos.x, cur_pos.y, cur_pos.z);
  }

  bool is_jumping() const {
    return current_time - time_last_jump < jump_period;
  }

  void jump() {
    if(current_time - time_last_jump < jump_reload) {
      return;
    }
    time_last_jump = current_time;
  }

  void set_jump_height() {
    double diff = current_time - time_last_jump;
    double period = jump_period;
    if(diff < period) {
      if(diff < period/2) {
        cur_pos.z = max_jump_height * std::sin((diff / period * 2) * M_PI/2);
      } else {
        cur_pos.z = max_jump_height * std::sin((2. - (diff / period * 2)) * M_PI/2);
      }
    }
  }

  bool is_sliding() const {
    return current_time - time_last_slide < slide_reload;
  }

  double last_slide_frame = -slide_reload;
  void slide() {
    if(!is_sliding() && !is_jumping()) {
      time_last_slide = current_time;
      last_slide_frame = time_last_slide;
    }
  }

  void perform_slide() {
    if(!is_sliding())return;
    double cur_slide_frame = current_time;
    double timediff = cur_slide_frame - last_slide_frame;
    glm::vec2 dir = direction();
    cur_pos.x += dir.x * sliding_speed;
    cur_pos.y += dir.y * sliding_speed;
  }

  bool is_running() const {
    glm::vec2 pos_xy(cur_pos.x, cur_pos.y);
    return glm::length(dest - pos_xy) > .001;
  }

  void run() {
    if(!is_running()) {
      return;
    }
    glm::vec2 pos_xy(cur_pos.x, cur_pos.y);
    auto dir = dest - pos_xy;
    dir *= running_speed / glm::length(dir);
    if(is_sliding()) {
      dest = cur_pos;
    } else {
      cur_pos += glm::vec3(dir.x, dir.y, 0);
      facing = atan2(dir.y, dir.x);
      transform.rotation = extra_rotate;
      transform.Rotate(0, 0, 1, facing * (180. / M_PI));
    }
  }
};
