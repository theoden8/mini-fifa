#pragma once

#include <glm/glm.hpp>

#include "Debug.hpp"
#include "Timer.hpp"

struct Unit {
  static constexpr float GAUGE = .0005;
  using vec_t = glm::vec3;
  using loc_t = vec_t;
  using real_t = float;
  using time_t = Timer::time_t;

  static real_t length(vec_t vec) { return glm::length(vec); }

  Timer timer;

  loc_t pos;
  loc_t dest;
  Unit *dest_unit=nullptr;
  real_t moving_speed = 0.;
  real_t facing = .0;
  real_t facing_dest = .0;
  const real_t facing_speed;
  static constexpr int TIME_LOCKED_MOVE = 0;

  Unit(vec_t pos={0, 0, 0}, real_t facing_speed=4*M_PI):
    pos(pos), dest(pos), facing_speed(facing_speed)
  {
    timer.set_event(TIME_LOCKED_MOVE);
  }

  real_t height() const { return pos.z; }
  real_t &height() { return pos.z; }
  vec_t velocity() const {
    vec_t dir(
      dest.x - pos.x,
      dest.y - pos.y,
      0
    );
    /* printf("dir: %f %f %f\n", dir.x, dir.y, dir.z); */
    if(length(dir) < .0001) return vec_t(0, 0, 0);
    return moving_speed * dir / length(dir);
  }

  void idle(Timer &t) {
    timer.set_time(t.current_time);
    if(dest_unit)dest=dest_unit->pos;
    idle_facing();
    idle_moving();
  }

  void move(loc_t location) {
    if(!timer.timed_out(TIME_LOCKED_MOVE)) {
      facing_dest = facing_angle(location);
      return;
    }
    face(location);
    dest = location;
  }

  void slide(loc_t location, time_t lock_dur) {
    if(!timer.timed_out(TIME_LOCKED_MOVE))return;
    stop();
    dest = location;
    timer.set_event(TIME_LOCKED_MOVE);
    timer.set_timeout(TIME_LOCKED_MOVE, lock_dur);
  }

  void move(Unit &unit) {
    dest_unit = &unit;
  }

  void face(float angle) {
    stop();
    facing_dest = angle;
  }

  float facing_angle(loc_t location) {
    return atan2(location.y - pos.y, location.x - pos.x);
  }

  void face(loc_t location) {
    face(facing_angle(location));
  }

  void stop() {
    /* printf("stop dest: %f %f %f\n", dest.x, dest.y, dest.z); */
    dest = pos;
    if(dest_unit)dest_unit=nullptr;
  }

  void idle_facing() {
    while(facing < -M_PI)facing += 2*M_PI;
    while(facing > M_PI)facing -= 2*M_PI;
    real_t face_diff = std::abs(facing - facing_dest);
    while(face_diff >= 2*M_PI)face_diff -= 2*M_PI;
    real_t timediff = timer.elapsed();
    if(face_diff < .001)return;
    real_t delta = facing_speed * timediff;
    /* printf("facing: %f \\tendsto %f\n", facing, facing_dest); */
    if(face_diff < M_PI) {
      if(face_diff < std::abs(delta))facing=facing_dest;
      else if(facing > facing_dest)facing -= delta;
      else facing += delta;
    } else {
      if(2*M_PI - face_diff < std::abs(delta))facing=facing_dest;
      else if(facing > facing_dest)facing += delta;
      else facing -= delta;
    }
  }

  vec_t point_offset(real_t offset, float angle) const {
    return pos + offset * vec_t(
      std::cos(angle),
      std::sin(angle),
      height()
    );
  }

  vec_t point_offset(real_t offset) const {
    return point_offset(offset, facing);
  }

  bool is_moving() const {
    return length(dest - pos) > .0001;
  }

  void idle_moving() {
    if(!is_moving())return;
    /* printf("pos: %f %f %f\n", pos.x, pos.y, pos.z); */
    real_t timediff = timer.elapsed();
    auto dir = vec_t(dest.x - pos.x, dest.y - pos.y, 0);
    /* printf("pos: %f %f %f\n", pos.x, pos.y, pos.z); */
    /* printf("dest: %f %f %f\n", dest.x, dest.y, dest.z); */
    /* printf("dir: %f %f %f\n", dir.x, dir.y, dir.z); */
    /* printf("timediff: %f\n", timediff); */
    /* printf("moving_speed: %f\n", moving_speed); */
    real_t step = timediff * moving_speed;
    if(length(dir) <= step) {
      pos.x = dest.x;
      pos.y = dest.y;
    } else {
      dir *= step / length(dir);
      pos += dir;
      /* printf("facing angle: %f\n", facing_dest); */
    }
    /* printf("pos: %f %f %f\n", pos.x, pos.y, pos.z); */
  }
};
