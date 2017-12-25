#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Ball.hpp"
#include "Model.hpp"
#include "Shadow.hpp"

#include "Timer.hpp"
#include "Unit.hpp"

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
  Shadow shadow;

  Unit::loc_t initial_position;
  Player(int id, bool team, std::pair<float, float> pos={0, 0}):
    team(team), playerId(id),
    initial_position(pos.first, pos.second, 0),
    uTransform("transform"),
    playerModel("assets/nanosuit/nanosuit.obj"),
    program({"player.vert", "player.frag"}),
    unit(Unit::loc_t(initial_position), 4*M_PI)
  {
    transform.SetScale(.01);
    transform.Scale(1, .75, 1);
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    extra_rotate =
      glm::rotate(glm::radians(90.f), glm::vec3(0, 0, 1))
      * glm::rotate(glm::radians(90.f), glm::vec3(1, 0, 0));
    transform.rotation = extra_rotate;
    shadow.transform.SetScale(.025);
  }

  void init() {
    program.compile_program();
    playerModel.init();
    uTransform.set_id(program.id());
    shadow.init();
    set_timer();
  }

  void keyboard(int key) {
    double step = .3;
    if(key == GLFW_KEY_A) {
      unit.move(Unit::loc_t(unit.pos.x + step, unit.pos.y, unit.pos.z));
    } else if(key == GLFW_KEY_D) {
      unit.move(Unit::loc_t(unit.pos.x - step, unit.pos.y, unit.pos.z));
    } else if(key == GLFW_KEY_W) {
      unit.move(Unit::loc_t(unit.pos.x, unit.pos.y - step, unit.pos.z));
    } else if(key == GLFW_KEY_S) {
      unit.move(Unit::loc_t(unit.pos.x, unit.pos.y + step, unit.pos.z));
    } else if(key == GLFW_KEY_H) {
      unit.stop();
    }
  }

  void display(Camera &cam) {
    shadow.transform.SetPosition(unit.pos.x, unit.pos.y, .001);
    shadow.display(cam);

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
  Unit unit;
  Timer timer;
  bool team;
  int playerId;
  static constexpr int
    TIME_LOST_BALL = 0,
    TIME_GOT_BALL = 1,
    TIME_OF_LAST_JUMP = 2,
    TIME_OF_LAST_SLIDE = 3,
    TIME_OF_LAST_PASS = 4;
  const float running_speed = 0.1;
  bool has_ball = false;
  const Timer::time_t possession_cooldown = 1.1;
  const float possession_range = .05;
  const float possession_offset = .03;
  const float possession_running_speed = running_speed * .75;
  const Timer::time_t jump_cooldown = 3.;
  const float max_jump_height = .07;
  const Timer::time_t jump_duration = 1.75;
  const Timer::time_t pass_cooldown = 2.;
  const Timer::time_t slide_duration = .5;
  const Timer::time_t slide_slowdown_duration = .5;
  const float slide_speed = 2 * running_speed;
  const float slide_slowdown_speed = .5 * running_speed;
  const float slide_cooldown = slide_duration + slide_slowdown_duration;

  void set_timer() {
    timer.set_event(TIME_GOT_BALL);
    timer.set_timeout(TIME_LOST_BALL, possession_cooldown);
    timer.set_timeout(TIME_OF_LAST_JUMP, jump_cooldown);
    timer.set_timeout(TIME_OF_LAST_SLIDE, slide_cooldown);
    timer.set_timeout(TIME_OF_LAST_PASS, pass_cooldown);
    /* timer.dump_times(); */
  }

  int id() const {
    return playerId;
  }

  Unit::vec_t velocity() const {
    return unit.velocity();
  }

  void timestamp_got_ball() {
    has_ball = true;
    timer.set_event(TIME_GOT_BALL);
  }

  void timestamp_lost_ball() {
    has_ball = false;
    timer.set_event(TIME_LOST_BALL);
  }

  Unit::loc_t possession_point() const {
    return unit.point_offset(possession_offset);
  }

  float get_control_potential(const Ball &ball) const {
    if(
      !timer.timed_out(TIME_LOST_BALL)
      || unit.height() - .01 > ball.unit.height()
    )
    {
      printf("control: elapsed %f, return NAN\n", timer.elapsed(TIME_LOST_BALL));
      return NAN;
    }
    float range = Unit::length(ball.unit.pos - possession_point());
    printf("potential %f vs %f\n", range, possession_range);
    if(range > possession_range) return NAN;
    return range;
  }

  void idle(double curtime) {
    printf("\nplayer idle:\n");
    timer.set_time(curtime);
    update_speed();
    unit.idle(timer.current_time);
    transform.rotation = extra_rotate;
    transform.Rotate(0, 0, 1, unit.facing / M_PI * 180.f);
    set_jump_height();
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    printf("rotate to %f\n", unit.facing);
  }

  void update_speed() {
    if(is_sliding_fast()) {
      unit.moving_speed = slide_speed;
    } else if(is_sliding_slowndown()) {
      unit.moving_speed = slide_slowdown_speed;
    } else if(has_ball) {
      unit.moving_speed = possession_running_speed;
    } else {
      unit.moving_speed = running_speed;
    }
  }

  void kick_the_ball(Ball &ball, Unit::vec_t direction, float angle) {
    ball.velocity() = glm::rotate(direction, angle, Unit::vec_t(0, 0, 1));;
    timestamp_lost_ball();
  }

  bool can_jump() const {
    return timer.timed_out(TIME_OF_LAST_JUMP);
  }

  bool is_jumping() const {
    return timer.elapsed(TIME_OF_LAST_JUMP) < jump_duration;
  }

  bool is_going_up() const {
    return timer.elapsed(TIME_OF_LAST_JUMP) < jump_duration / 2;
  }

  bool is_landing() const {
    return is_jumping() && !is_going_up();
  }

  void jump() {
    if(!can_jump())return;
    timer.set_event(TIME_OF_LAST_JUMP);
  }

  void set_jump_height() {
    double diff = timer.elapsed(TIME_OF_LAST_JUMP);
    if(is_going_up()) {
      unit.height() = max_jump_height * std::sin((diff / jump_duration * 2) * M_PI/2);
    } else if(is_landing()) {
      unit.height() = max_jump_height * std::sin((2. - (diff / jump_duration * 2)) * M_PI/2);
    } else {
      unit.height() = 0;
    }
  }

  bool can_pass() const {
    return timer.timed_out(TIME_OF_LAST_PASS);
  }

  void timestamp_passed() {
    if(!can_pass())return;
    timer.set_event(TIME_OF_LAST_PASS);
  }

  bool can_slide() {
    return !is_jumping() && timer.timed_out(TIME_OF_LAST_SLIDE);
  }

  bool is_sliding() const {
    return timer.elapsed(TIME_OF_LAST_SLIDE) < slide_cooldown;
  }

  bool is_sliding_fast() const {
    return timer.elapsed(TIME_OF_LAST_SLIDE) < slide_duration;
  }

  bool is_sliding_slowndown() const {
    return is_sliding() && !is_sliding_fast();
  }

  void timestamp_slide() {
    ASSERT(!has_ball);
    if(!can_slide())return;
    timer.set_event(TIME_OF_LAST_SLIDE);
  }
};
