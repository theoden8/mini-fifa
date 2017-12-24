#pragma once

#include <cmath>

#include "incgraphics.h"
#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"

struct Ball {
  static constexpr size_t DIM = 10;
  static constexpr size_t SIZE = DIM*DIM*4;

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
    attrTexcoords("vtex")
  {
    transform.SetScale(.01, .01, .01);
    transform.SetPosition(0, 0, 0);
    transform.SetRotation(1, 0, 0, 180.f);
  }

  glm::vec3 point_on_sphere(float dyx, float dzx) {
    return glm::vec3(
      std::sin(dyx) * std::cos(dzx),
      std::cos(dyx),
      std::sin(dyx) * std::sin(dzx)
    );
  }

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
  }

  void Keyboard(GLFWwindow *w) {
    /* if(glfwGetKey(w, GLFW_KEY_X)) { */
    /*   posit = glm::vec3(0, 0, .4); */
    /* } else if(glfwGetKey(w, GLFW_KEY_Z)) { */
    /*   speed += glm::vec3(.001, 0, 0); */
    /* } else if(glfwGetKey(w, GLFW_KEY_C)) { */
    /*   speed += glm::vec3(-.005, 0, .003); */
    /* } */
  }

// gameplay
  glm::vec3 posit{0, 0, 0};
  glm::vec3 speed{0, 0, 0};
  float mass = 1.0f;
  glm::vec3 gravity{0, 0, -.0007};
  double current_time = 0.;

  void idle(double curtime = NAN) {
    double prev_time = current_time;
    current_time = std::isnan(curtime) ? glfwGetTime() : curtime;
    double timediff = current_time - prev_time;
    float air_resistance = 0.9;
    float collision_resistance = 0.8;
    posit += speed * air_resistance;
    if(posit.z < 0) {
      posit.z = -posit.z;
      speed.z = -speed.z;
      speed *= collision_resistance;
    } else if(posit.z > .001) {
      speed += gravity/mass;
    } else if(posit.z < 0.001) {
      speed *= collision_resistance;
    }
    transform.SetPosition(posit.x, posit.y, posit.z);
    /* transform.SetRotation(0, 0, 1, glfwGetTime() * 10); */
    auto speedlen = glm::length(glm::vec2(speed.x, speed.y));
    if(speedlen) {
      glm::vec3 norm = glm::vec3(
        speed.x,
        speed.y,
        0
      );
      transform.Rotate(norm.x, norm.y, norm.z, glm::length(speed) * 100);
    }
  }

  glm::vec3 &position() { return posit; }
  glm::vec3 &velocity() { return speed; }

// graphics again
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
};
