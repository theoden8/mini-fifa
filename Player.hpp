#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Ball.hpp"
#include "Model.hpp"
#include "Shadow.hpp"
#include "Sprite.hpp"

#include "Timer.hpp"
#include "Unit.hpp"

#include "File.hpp"

struct Player {
// opengl stuff
  glm::mat4 matrix;
  glm::mat4 extra_rotate;
  Transformation transform;

	Sprite<Model, class RedPlayer> *playerModelRed;
	Sprite<Model, class BluePlayer> *playerModelBlue;
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
    playerModelRed(Sprite<Model, class RedPlayer>::create("assets/ninja/ninja.3ds")),
    playerModelBlue(Sprite<Model, class BluePlayer>::create("assets/ninja/ninja.3ds")),
    program({"player.vert", "player.frag"}),
    unit(Unit::loc_t(initial_position), 4*M_PI)
  {
    transform.SetScale(.01);
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    extra_rotate =
      glm::rotate(glm::radians(-90.f), glm::vec3(0, 0, 1))
      * glm::rotate(glm::radians(90.f), glm::vec3(1, 0, 0));
    transform.rotation = extra_rotate;
    shadow.transform.SetScale(.025);
  }

  void init() {
    program.compile_program();
    if(!playerModelRed->initialized) {
      HACK::swap_files("assets/ninja/nskinbl.jpg", "assets/ninja/nskinrd.jpg");
    }
    playerModelRed->init();
    if(!playerModelBlue->initialized) {
      HACK::swap_files("assets/ninja/nskinbl.jpg", "assets/ninja/nskinrd.jpg");
    }
    playerModelBlue->init();
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
    } else if(key == GLFW_KEY_SPACE) {
      unit.move(unit.point_offset(step));
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

    if(!team) {
      playerModelRed->display(program);
    } else {
      playerModelBlue->display(program);
    }

    decltype(program)::unuse();
  }

  void clear() {
    playerModelRed->clear();
    playerModelBlue->clear();
    program.clear();
  }

// gameplay
  Unit unit;
  Timer timer;
  bool team;
  int playerId;
  static constexpr int
    TIME_DISPOSSESSED = 0,
    TIME_GOT_BALL = 1,
    TIME_OF_LAST_JUMP = 2,
    TIME_OF_LAST_SLIDE = 3,
    TIME_LAST_SLOWN_DOWN = 4,
    TIME_OF_LAST_PASS = 5;
  static constexpr Timer::time_t CANT_HOLD_BALL_DISPOSSESS = 1.45;
  static constexpr Timer::time_t CANT_HOLD_BALL_SHOT = 0.9;
  const float running_speed = 0.29 * .5;
  float tallness = .03;
  bool is_in_air = false;
  float default_height = .0001;
  float vertical_speed = .0;
  const Timer::time_t jump_cooldown = 3.;
  bool has_ball = false;
  const float possession_range = .06;
  const float possession_offset = .03;
  const float possession_running_speed = running_speed * .69;
  const Timer::time_t pass_cooldown = 2.;
  const Timer::time_t slide_duration = .7;
  const Timer::time_t slide_slowdown_duration = 1.8;
  const Timer::time_t slown_down_duration = .95;
  const float slide_speed = 1.38 * running_speed;
  const float slide_slowdown_speed = .5 * running_speed;
  const float slide_cooldown = slide_duration + slide_slowdown_duration;

  void set_timer() {
    timer.set_event(TIME_GOT_BALL);
    timer.set_event(TIME_DISPOSSESSED);
    timer.set_timeout(TIME_OF_LAST_JUMP, jump_cooldown);
    timer.set_timeout(TIME_OF_LAST_SLIDE, slide_cooldown);
    timer.set_timeout(TIME_LAST_SLOWN_DOWN, slown_down_duration);
    timer.set_timeout(TIME_OF_LAST_PASS, pass_cooldown);
    /* timer.dump_times(); */
  }

  int id() const { return playerId; }
  Unit::vec_t velocity() const { return unit.velocity(); }

  void idle(double curtime) {
    /* printf("\nplayer idle:\n"); */
    timer.set_time(curtime);
    idle_speed();
    idle_jump();
    unit.idle(timer);
    transform.rotation = extra_rotate;
    transform.Rotate(0, 0, 1, unit.facing / M_PI * 180.f);
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    /* printf("rotate to %f\n", unit.facing); */
  }

  void idle_speed() {
    if(is_sliding_fast()) {
      unit.moving_speed = slide_speed;
    } else if(is_sliding_slowndown() || is_slown_down()) {
      unit.moving_speed = slide_slowdown_speed;
    } else if(has_ball) {
      unit.moving_speed = possession_running_speed;
    } else {
      unit.moving_speed = running_speed;
    }
  }

  void idle_jump() {
    double timediff = timer.elapsed();
    if(is_jumping() && id()==0) {
      printf("mode: %s [%d %d]\n", is_in_air?"jumping":"running", is_going_up(), is_landing());
      printf("height: %f\n", unit.height());
      printf("vertical speed: %f\n", vertical_speed);
    }
    if(is_jumping()) {
      if(vertical_speed > .0 || unit.height() > default_height) {
        float h = unit.height();
        unit.height() += 30 * vertical_speed * timediff;
        printf("height: %f -> %f\n", h, unit.height());
        vertical_speed -= .0069 * timediff;
      } else {
        if(!id())printf("landing\n");
        unit.height() = default_height;
        vertical_speed = .0;
        is_in_air = false;
      }
    } else {
      unit.height() = default_height;
    }
  }

  bool can_jump() const {
    return timer.timed_out(TIME_OF_LAST_JUMP);
  }

  bool is_jumping() const {
    return is_in_air;
  }

  bool is_going_up() const {
    return is_jumping() && vertical_speed > 0.;
  }

  bool is_landing() const {
    return is_jumping() && vertical_speed < 0.;
  }

  void jump(float vspeed) {
    if(!can_jump())return;
    timer.set_event(TIME_OF_LAST_JUMP);
    ASSERT(!is_in_air);
    is_in_air = true;
    vertical_speed = vspeed;
  }

  bool is_owner(Ball &ball) {
    bool res = ball.current_owner == id();
    ASSERT(res && has_ball || (!res && !has_ball));
    return res;
  }

  bool can_possess() const {
    return timer.timed_out(TIME_DISPOSSESSED);
  }

  void timestamp_got_ball(Ball &ball) {
    has_ball = true;
    timer.set_event(TIME_GOT_BALL);
    ball.timestamp_set_owner(playerId);
  }

  void timestamp_dispossess(Ball &ball, float lock_for) {
    ASSERT(is_owner(ball));
    has_ball = false;
    timer.set_event(TIME_DISPOSSESSED);
    timer.set_timeout(TIME_DISPOSSESSED, lock_for);
    ball.timestamp_set_owner(Ball::NO_OWNER);
  }

  Unit::loc_t possession_point() const {
    if(is_going_up()) {
      return unit.pos;
    }
    return unit.point_offset(possession_offset);
  }

  float get_control_potential(const Ball &ball) const {
    if(
      !ball.can_interact()
      || !can_possess()
      || ball.unit.height() <= unit.height()
      || unit.height() + tallness <= ball.unit.height()
      || is_sliding_slowndown()
      || is_slown_down()
    )
    {
      if(id() == 0) {
        /* printf("control: elapsed %f, return NAN\n", timer.elapsed(TIME_DISPOSSESSED)); */
        /* printf("control: conditions %d %d %d [%f %f] %d %d\n", */
        /*   !ball.can_interact(), */
        /*   !can_possess(), */
        /*   !( */
        /*     ball.unit.height() >= unit.height() */
        /*     && unit.height() + tallness >= ball.unit.height() */
        /*   ), ball.unit.height(), unit.height(), */
        /*   is_sliding_slowndown(), */
        /*   is_slown_down() */
        /* ); */
      }
      return NAN;
    }
    auto pp = possession_point();
    glm::vec2 bpos(ball.unit.pos.x, ball.unit.pos.y);
    float range = glm::length(bpos - glm::vec2(pp.x, pp.y));
    /* printf("potential %f vs %f\n", range, possession_range); */
    /* if(id()==0)printf("control: ranges %f %f\n", range, possession_range); */
    if(range > possession_range)return NAN;
    /* if(id()==0)printf("control: tackle potential %f\n", range); */
    return range;
  }

  void kick_the_ball(Ball &ball, float speed, float vspeed, float angle) {
    ball.face(angle);
    ball.unit.moving_speed = speed;
    ball.vertical_speed = vspeed;
    timestamp_dispossess(ball, CANT_HOLD_BALL_SHOT);
    ball.disable_interaction(Ball::CANT_INTERACT_SHOT);
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
    return !timer.timed_out(TIME_OF_LAST_SLIDE);
  }

  bool is_sliding_fast() const {
    return timer.elapsed(TIME_OF_LAST_SLIDE) < slide_duration;
  }

  bool is_sliding_slowndown() const {
    return is_sliding() && !is_sliding_fast();
  }

  bool is_slown_down() const {
    return !timer.timed_out(TIME_LAST_SLOWN_DOWN);
  }

  void timestamp_slide() {
    ASSERT(!has_ball);
    if(!can_slide())return;
    timer.set_event(TIME_OF_LAST_SLIDE);
  }
};
