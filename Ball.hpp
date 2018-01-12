#pragma once

#include <cmath>

#include "Timer.hpp"
#include "Unit.hpp"

struct Player;

struct Ball {
  Unit unit;
  Timer timer;
  static constexpr int
    TIME_LOOSE_BALL_BEGINS = 0,
    TIME_ABLE_TO_INTERACT = 1;
  const float loose_ball_cooldown = 0.1;
  static constexpr Timer::time_t CANT_INTERACT_SHOT = .7;
  static constexpr Timer::time_t CANT_INTERACT_SLIDE = .45;
  /* Unit::vec_t speed{0, 0, 0}; */
  const float default_height = Unit::GAUGE * 10;
  static constexpr float GROUND_FRICTION = .05;
  static constexpr float GROUND_HIT_SLOWDOWN = .02;
  const float G = Unit::GAUGE * 2.3;
  /* const float gravity = 2.3; */
  /* const float mass = 1.0f; */
  const float min_speed = Unit::GAUGE;
  float vertical_speed = .0;
  bool is_in_air = false;
  static constexpr int NO_OWNER = -1;
  int current_owner = NO_OWNER;
  int last_touched = NO_OWNER;

  Ball():
    unit(Unit::loc_t(0, 0, 0), M_PI * 4)
  {
    reset_height();
  }

  void set_timer() {
    reset_height();
    timer.set_timeout(TIME_LOOSE_BALL_BEGINS, loose_ball_cooldown);
  }

  Unit::vec_t &position() { return unit.pos; }
  /* Unit::vec_t &velocity() { return speed; } */
  void reset_height() {
    unit.height() = default_height;
  }

  void idle(double curtime) {
    /* printf("ball pos: %f %f %f\n", unit.pos.x,unit.pos.y,unit.pos.z); */
    /* printf("ball speed: %f %f\n", unit.moving_speed, vertical_speed); */
    timer.set_time(curtime);
    Timer::time_t timediff = timer.elapsed();
    if(owner() == NO_OWNER || is_loose()) {
      if(unit.moving_speed < min_speed) {
        unit.stop();
        unit.moving_speed = min_speed;
      } else {
        unit.move(unit.point_offset(1., unit.facing_dest));
      }

      if(is_in_air) {
        if(vertical_speed < 0. && unit.height() <= default_height) {
          // ball hits the ground
          unit.moving_speed -= GROUND_HIT_SLOWDOWN;
          reset_height();
          if(std::abs(vertical_speed) < min_speed) {
            is_in_air = false;
            vertical_speed = 0.;
          } else {
            vertical_speed -= .3 * vertical_speed;
            vertical_speed = std::abs(vertical_speed);
          }
        } else {
          // update height
          unit.height() += 8. * vertical_speed * timediff;
          vertical_speed -= 8. * G * timediff;
        }
      } else {
        unit.moving_speed -= GROUND_FRICTION * timediff;
        reset_height();
      }
    }
    unit.idle(timer);
  }

  void face(float angle) {
    unit.facing_dest = angle;
  }

  void face(Unit::loc_t point) {
    unit.facing_dest = atan2(point.y - unit.pos.y, point.x - unit.pos.x);
  }

  void timestamp_set_owner(int new_owner) {
    if(current_owner == new_owner)return;
    current_owner = new_owner;
    if(current_owner != -1) {
      timer.set_event(TIME_LOOSE_BALL_BEGINS);
      last_touched = current_owner;
    }
  }

  int owner() const {
    return current_owner;
  }

  bool is_loose() const {
    return owner() != NO_OWNER && !timer.timed_out(TIME_LOOSE_BALL_BEGINS);
  }

  void disable_interaction(Timer::time_t lock_for=CANT_INTERACT_SHOT) {
    timer.set_event(TIME_ABLE_TO_INTERACT);
    timer.set_timeout(TIME_ABLE_TO_INTERACT, lock_for);
  }

  bool can_interact() const {
    return timer.timed_out(TIME_ABLE_TO_INTERACT);
  }
};
