#pragma once

#include "Ball.hpp"

#include "incgraphics.h"
#include "Optimizations.hpp"
#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Shadow.hpp"

struct BallObject {
  static constexpr size_t DIM = 10; // number of steps per dimension to perform for tessellation
  static constexpr size_t SIZE = DIM*DIM*4; // number of triangles used in tessellation
  float deg = 0;

  Transformation transform;
  glm::mat4 matrix;

  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::Uniform<gl::UniformType::VEC3> uColor;
  gl::Buffer<GL_ARRAY_BUFFER, gl::BufferElementType::VEC3> bufVertex;
  gl::Buffer<GL_ARRAY_BUFFER, gl::BufferElementType::VEC2> bufTexcoord;
  gl::Attrib<decltype(bufVertex)> attrVertex;
  gl::Attrib<decltype(bufTexcoord)> attrTexcoord;
  gl::VertexArray<decltype(attrVertex), decltype(attrTexcoord)> vao;
  gl::Texture ballTx;
  Shadow shadow;

  using ShaderBufferVEC3 = decltype(bufVertex);
  using ShaderBufferVEC2 = decltype(bufTexcoord);
  using ShaderAttribVEC3 = decltype(attrVertex);
  using ShaderAttribVEC2 = decltype(attrTexcoord);
  using VertexArray = decltype(vao);
  using ShaderProgram = decltype(program);

  BallObject():
    uTransform("transform"),
    uColor("color"),
    program({"shaders/ball.vert"s, "shaders/ball.frag"s}),
    ballTx("ballTx"),
    attrVertex("vpos", bufVertex),
    attrTexcoord("vtex", bufTexcoord),
    vao(attrVertex, attrTexcoord)
  {
    transform.SetScale(.01, .01, .01);
    transform.SetPosition(0, 0, 0);
    transform.SetRotation(1, 0, 0, 180.f);
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
  void set_vertices(std::vector<float> &vertices) {
    ASSERT(vertices.size() == SIZE * 9);
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
      float *v = &vertices[offset];
      v[0]=a.x,v[1]=a.y,v[2]=a.z;
      v[3]=b.x,v[4]=b.y,v[5]=b.z;
      v[6]=c.x,v[7]=c.y,v[8]=c.z;
    }
  }

  // set texture coordinates for triangles used to tessellate
  void set_texcoords(std::vector<float> &coords) {
    ASSERT(coords.size() == SIZE * 6);
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
    {
      std::vector<float> vertices(SIZE * 9);
      set_vertices(vertices);
      ShaderBufferVEC3::init(bufVertex);
      bufVertex.allocate<GL_STREAM_DRAW>(vertices);
    }

    {
      std::vector<float> txcoords(SIZE * 6);
      set_texcoords(txcoords);
      ShaderBufferVEC2::init(bufTexcoord);
      bufTexcoord.allocate<GL_STREAM_DRAW>(txcoords);
    }

    VertexArray::init(vao);

    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0);
    vao.set_divisor(attrVertex, 0);

    vao.enable(attrTexcoord);
    vao.set_access(attrTexcoord, 0);
    vao.set_divisor(attrTexcoord, 0);

    ShaderProgram::init(program, vao);
    ballTx.init("assets/ball.png");

    ballTx.uSampler.set_id(program.id());
    uTransform.set_id(program.id());
    uColor.set_id(program.id());

    shadow.init();
  }

  void display(const Ball &ball, Camera &cam) {
    transform.SetPosition(ball.unit.pos.x, ball.unit.pos.y, ball.unit.pos.z);
    float angle = ball.unit.facing_dest;
    glm::vec2 dir(std::cos(angle), std::sin(angle));
    glm::vec2 nrm = glm::normalize(dir);
    Timer::time_t timediff = ball.timer.elapsed();
    deg += 5*360.f*ball.unit.moving_speed*timediff;
    transform.SetRotation(0, 0, M_PI/2, deg);

    shadow.transform.SetPosition(ball.unit.pos.x, ball.unit.pos.y, .001);
    shadow.display(cam);

    ShaderProgram::use(program);

    if(transform.has_changed || cam.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    {
      glm::vec3 color(1, 1, 1);
      if(ball.owner() != Ball::NO_OWNER) {
        color.x = .8;
      }
      if(ball.is_in_air) {
        color.y = .8;
      }
      uColor.set_data(color);
    }

    gl::Texture::set_active(0);
    ballTx.bind();
    ballTx.set_data(0);

    VertexArray::bind(vao);
    VertexArray::draw<GL_TRIANGLES>(vao, 0, SIZE * 3);
    VertexArray::unbind();

    gl::Texture::unbind();
    ShaderProgram::unuse();
  }

  void clear() {
    shadow.clear();
    ballTx.clear();
    ShaderAttribVEC3::clear(attrVertex);
    ShaderAttribVEC2::clear(attrTexcoord);
    ShaderBufferVEC3::clear(bufVertex);
    ShaderBufferVEC2::clear(bufTexcoord);
    VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }
};
