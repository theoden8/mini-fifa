#pragma once

#include <cmath>

#include "incgraphics.h"
#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Shadow.hpp"

#include "Timer.hpp"
#include "Unit.hpp"

struct Player;

struct Ball {
  static constexpr size_t DIM = 10; // number of steps per dimension to perform for tessellation
  static constexpr size_t SIZE = DIM*DIM*4; // number of triangles used in tessellation

  Transformation transform;
  glm::mat4 matrix;

  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::Uniform<gl::UniformType::VEC3> uColor;
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC3> attrVertices;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrTexcoords;
  gl::Texture ballTx;
  Shadow shadow;

  Ball():
    uTransform("transform"),
    uColor("color"),
    program({"ball.vert", "ball.frag"}),
    ballTx("ballTx"),
    attrVertices("vpos"),
    attrTexcoords("vtex"),
    unit(Unit::vec_t(0, 0, 0), M_PI * 4)
  {
    transform.SetScale(.01, .01, .01);
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    transform.SetRotation(1, 0, 0, 180.f);
    reset_height();
    shadow.transform.SetScale(.01);
  }

  // tesselate a sphere with triangles
  // this function takes two angles and constructs a triangle with them
  glm::vec3 point_on_sphere(float dyx, float dzx) {
    return glm::vec3(
      std::sin(dyx) * std::cos(dzx),
      std::cos(dyx),
      std::sin(dyx) * std::sin(dzx)
    );
  }

  // set vertices of the tessellation
  void set_vertices(float *verts) {
    float dimf = DIM;
    for(int index = 0; index < SIZE; ++index) {
      int IF = index & 1;

      const float step = M_PI / dimf;
      const float dyx = (float)((index/2) / (DIM*2)) * step;
      const float dzx = (float)((index/2) % (DIM*2)) * step;

      const int offset = index * 9;
      glm::vec3
        a = point_on_sphere(dyx,dzx),
        b = point_on_sphere(dyx + (IF?0:step), dzx+step),
        c = point_on_sphere(dyx + step, dzx + (IF?step:0));
      float *v = &verts[offset];
      v[0]=a.x,v[1]=a.y,v[2]=a.z;
      v[3]=b.x,v[4]=b.y,v[5]=b.z;
      v[6]=c.x,v[7]=c.y,v[8]=c.z;
    }
  }

  // set texture coordinates for triangles used to tessellate
  void set_texcoords(float *coords) {
    float dimf = DIM;
    for(int index = 0; index < SIZE; ++index) {
      int IF = index & 1;

      const int offset = index * 6;
      const float tx0 = (index/2) % (2*DIM) / (2.*dimf);
      const float txstep = .5/dimf;
      const float ty0 = (DIM - 1 - (index/2) / (2*DIM)) / dimf;
      const float tystep = 1./dimf;
      coords[offset+0] = tx0;
      coords[offset+1] = ty0 + tystep;
      coords[offset+2] = tx0 + txstep;
      coords[offset+3] = ty0 + tystep*(IF?1:0);
      coords[offset+4] = tx0 + txstep*(IF?1:0);
      coords[offset+5] = ty0;
    }
  }

  void init() {
    float *vertices = new float[SIZE*9];
    set_vertices(vertices);
    attrVertices.init();
    attrVertices.allocate<GL_STREAM_DRAW>(SIZE*3, vertices);
    delete [] vertices;
    float *txcoords = new float[SIZE*6];
    set_texcoords(txcoords);
    attrTexcoords.init();
    attrTexcoords.allocate<GL_STREAM_DRAW>(SIZE*3, txcoords);
    delete [] txcoords;
    vao.init();
    vao.bind();
    vao.enable(attrVertices);
    vao.set_access(attrVertices, 0, 0);
    vao.enable(attrTexcoords);
    vao.set_access(attrTexcoords, 1, 0);
    glVertexAttribDivisor(0, 0); GLERROR
    glVertexAttribDivisor(1, 0); GLERROR
    gl::VertexArray::unbind();
    program.init(vao, {"vpos", "vtex"});
    ballTx.init("assets/ball.png");

    ballTx.uSampler.set_id(program.id());
    uTransform.set_id(program.id());
    uColor.set_id(program.id());
    gl::VertexArray::unbind();

    shadow.init();
    set_timer();
  }

  void keyboard(int key) {
    if(key == GLFW_KEY_X) {
      /* unit.pos = Unit::loc_t(0, 0, .2); */
    } else if(key == GLFW_KEY_T) {
      /* speed = Unit::vec_t(1.25, 0, 1); */
    } else if(key == GLFW_KEY_Y) {
      /* speed = Unit::vec_t(-1.25, 0, 1); */
    }
  }

  void display(Camera &cam) {
    shadow.transform.SetPosition(unit.pos.x, unit.pos.y, .001);
    shadow.display(cam);

    program.use();

    if(transform.has_changed || cam.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    {
      glm::vec3 color(1, 1, 1);
      if(owner() != NO_OWNER) {
        color.x = .8;
      }
      if(is_in_air) {
        color.y = .8;
      }
      uColor.set_data(color);
    }

    gl::Texture::set_active(0);
    ballTx.bind();
    ballTx.set_data(0);

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, SIZE*3); GLERROR
    gl::VertexArray::unbind();

    gl::Texture::unbind();
    decltype(program)::unuse();
  }

  void clear() {
    shadow.clear();
    ballTx.clear();
    attrVertices.clear();
    attrTexcoords.clear();
    vao.clear();
    program.clear();
  }

// gameplay
  Unit unit;
  Timer timer;
  static constexpr int
    TIME_LOOSE_BALL_BEGINS = 0,
    TIME_ABLE_TO_INTERACT = 1;
  const float loose_ball_cooldown = .16;
  static constexpr Timer::time_t CANT_INTERACT_SHOT = .7;
  static constexpr Timer::time_t CANT_INTERACT_SLIDE = .45;
  /* Unit::vec_t speed{0, 0, 0}; */
  const float default_height = .001;
  static constexpr float GROUND_FRICTION = .05;
  static constexpr float GROUND_HIT_SLOWDOWN = .05;
  /* const float G = 1.15; */
  /* const float gravity = 2.3; */
  /* const float mass = 1.0f; */
  const float min_speed = .002;
  float vertical_speed = 0.;
  bool is_in_air = false;
  static constexpr int NO_OWNER = -1;
  int current_owner = NO_OWNER;
  int last_touched = NO_OWNER;

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
    /* unit.moving_speed = std::max<float>(.01, glm::length(glm::vec2(speed.x, speed.y))); */
    /* if(is_loose() || owner() == NO_OWNER) { */
      /* idle_physics(); */
    /* } */
    Timer::time_t timediff = timer.elapsed();
    if(owner() == NO_OWNER) {
      idle_horizontal();
      if(is_in_air) {
        idle_vertical();
      } else {
        unit.moving_speed -= GROUND_FRICTION * timediff;
      }
    }
    unit.idle(timer);
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    float angle = unit.facing;
    transform.Rotate(std::cos(angle+90), std::sin(angle+90), 0, 5*360.f*unit.moving_speed*timediff);
  }

  void face(float angle) {
    unit.facing_dest = angle;
  }

  void face(Unit::loc_t point) {
    unit.facing_dest = atan2(point.y - unit.pos.y, point.x - unit.pos.x);
  }

  void idle_horizontal() {
    if(unit.moving_speed < min_speed) {
      unit.stop();
      unit.moving_speed = 0.;
    } else {
      unit.move(unit.point_offset(1., unit.facing_dest));
    }
  }

  void idle_vertical() {
    Timer::time_t timediff = timer.elapsed();
    if(vertical_speed < 0. && unit.height() <= default_height) {
      // ball hits the ground
      unit.moving_speed -= GROUND_HIT_SLOWDOWN;
      reset_height();
      if(std::abs(vertical_speed) < min_speed) {
        is_in_air = false;
        vertical_speed = 0.;
      } else {
        vertical_speed *= .6;
        vertical_speed = std::abs(vertical_speed);
      }
    } else {
      // update height
      unit.height() += 30 * vertical_speed * timediff;
      vertical_speed -= .0069 * timediff;
    }
  }

  void timestamp_set_owner(int new_owner) {
    if(current_owner == new_owner)return;
    current_owner = new_owner;
    if(current_owner != -1) {
      timer.set_event(TIME_LOOSE_BALL_BEGINS);
      last_touched = current_owner;
      unit.moving_speed = 0.;
    }
  }

  int owner() const {
    return current_owner;
  }

  bool is_loose() const {
    return !timer.timed_out(TIME_LOOSE_BALL_BEGINS);
  }

  void disable_interaction(Timer::time_t lock_for=CANT_INTERACT_SHOT) {
    timer.set_event(TIME_ABLE_TO_INTERACT);
    timer.set_timeout(TIME_ABLE_TO_INTERACT, lock_for);
  }

  bool can_interact() const {
    return timer.timed_out(TIME_ABLE_TO_INTERACT);
  }
};
