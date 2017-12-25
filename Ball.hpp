#pragma once

#include <cmath>

#include "incgraphics.h"
#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"

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
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC3> attrVertices;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrTexcoords;
  gl::Texture ballTx;

  Ball():
    uTransform("transform"),
    program({"ball.vert", "ball.frag"}),
    ballTx("ballTx"),
    attrVertices("vpos"),
    attrTexcoords("vtex"),
    unit(Unit::vec_t(0, 0, 0), M_PI * 4)
  {
    transform.SetScale(.01, .01, .01);
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
    transform.SetRotation(1, 0, 0, 180.f);
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
    gl::VertexArray::unbind();

    set_timer();
  }

  void keyboard(int key) {
    if(key == GLFW_KEY_X) {
      /* unit.pos = Unit::loc_t(0, 0, .2); */
    } else if(key == GLFW_KEY_T) {
      speed = Unit::vec_t(1.25, 0, 1);
    } else if(key == GLFW_KEY_Y) {
      speed = Unit::vec_t(-1.25, 0, 1);
    }
  }

  void display(Camera &cam) {
    program.use();

    if(transform.has_changed || cam.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
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
    TIME_CHANGED_OWNER = 0;
  const float loose_ball_cooldown = .16;
  Unit::vec_t speed{0, 0, 0};
  const float default_height = .0001;
  const float air_friction = .1;
  const float ground_friction = .3;
  const Unit::vec_t gravity{0, 0, -2};
  const float mass = 1.0f;
  static constexpr int NO_OWNER = -1;
  int current_owner = NO_OWNER;
  int last_touched = NO_OWNER;

  void set_timer() {
    timer.set_timeout(TIME_CHANGED_OWNER, loose_ball_cooldown);
  }

  Unit::vec_t &position() { return unit.pos; }
  Unit::vec_t &velocity() { return speed; }
  void reset_height() {
    unit.height() = default_height;
  }

  void idle(double curtime) {
    printf("ball pos: %f %f %f\n", unit.pos.x,unit.pos.y,unit.pos.z);
    printf("ball speed: %f %f %f\n", speed.x,speed.y,speed.z);
    timer.set_time(curtime);
    /* printf("ball: %f %f %f\n", unit.pos.x, unit.pos.y, unit.pos.z); */
    unit.idle(curtime);
    /* printf("ball: %f %f %f\n", unit.pos.x, unit.pos.y, unit.pos.z); */
    if(is_loose() || owner() == NO_OWNER) {
      idle_physics();
    }
    transform.SetPosition(unit.pos.x, unit.pos.y, unit.pos.z);
  }

  void idle_physics() {
    Timer::time_t timediff = timer.elapsed();
    unit.pos += speed * (1.f - air_friction) * float(timediff);
    /* printf("ball: %f %f %f\n", unit.pos.x, unit.pos.y, unit.pos.z); */
    if(unit.height() < 0) {
      unit.height() = -unit.height();
      speed.z = -speed.z;
      speed *= (1.f - ground_friction);
    } else if(unit.height() >= .001) {
      speed += (gravity * float(timediff))/float(mass);
    } else if(unit.height() < .001) {
      reset_height();
      speed *= (1.f - ground_friction);
    }
    /* printf("ball: %f %f %f\n", unit.pos.x, unit.pos.y, unit.pos.z); */
    /* transform.SetRotation(0, 0, 1, timer.current_time * 10); */
    unit.facing_dest = unit.facing_angle(unit.pos + speed);
    auto speedlen = glm::length(glm::vec2(speed.x, speed.y));
    if(speedlen > .001) {
      float spin_angle = unit.facing + 90;
      glm::vec3 norm(
        std::cos(spin_angle),
        std::sin(spin_angle),
        0
      );
      transform.Rotate(norm.x, norm.y, norm.z, glm::length(speed) * 50);
    }
  }

  void timestamp_set_owner(int new_owner) {
    if(current_owner == new_owner)return;
    current_owner = new_owner;
    if(current_owner != -1) {
      timer.set_event(TIME_CHANGED_OWNER);
      last_touched = current_owner;
    }
  }

  int owner() const {
    return current_owner;
  }

  bool is_loose() const {
    return !timer.timed_out(TIME_CHANGED_OWNER);
  }
};
