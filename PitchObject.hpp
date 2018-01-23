#pragma once

#include <array>

#include "Transformation.hpp"
#include "Region.hpp"
#include "Camera.hpp"
#include "ShaderProgram.hpp"
#include "Shader.hpp"
#include "ShaderUniform.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"

struct PitchObject {
  Transformation transform;
  glm::mat4 matrix;

  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrVertex;
  gl::Texture grassTx;

  using ShaderAttrib = decltype(attrVertex);
  using ShaderProgram = decltype(program);

  PitchObject():
    program({"shaders/pitch.vert", "shaders/pitch.frag"}),
    uTransform("transform"),
    attrVertex("vertex"),
    grassTx("grass")
  {
    transform.SetScale(2, 1, 1);
    transform.SetPosition(0, 0, 0);
    transform.SetRotation(0, 1, 0, 0.f);
  }

  void init() {
    ShaderAttrib::init(attrVertex);
    attrVertex.allocate<GL_STREAM_DRAW>(6, std::vector<float>{
      1,1, -1,1, -1,-1,
      -1,-1, 1,-1, 1,1,
    });

    gl::VertexArray::init(vao);
    gl::VertexArray::bind(vao);
    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0, 0);
    gl::VertexArray::unbind();
    ShaderProgram::init(program, vao, {"attrVertex"});
    grassTx.init("assets/grass.png");

    grassTx.uSampler.set_id(program.id());
    uTransform.set_id(program.id());
  }

  Region playable_area() {
    return Region(glm::vec2(-1.80, 1.80), glm::vec2(-.95, .95));
  }

  Region left_half() {
    auto playable = playable_area();
    return Region(glm::vec2(playable.xs.x, 0), glm::vec2(playable.ys.x, playable.ys.y));
  }

  Region right_half() {
    auto playable = playable_area();
    return Region(glm::vec2(0, playable.xs.y), glm::vec2(playable.ys.x, playable.ys.y));
  }

  void display(Camera &cam) {
    ShaderProgram::use(program);

    if(transform.has_changed || cam.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    gl::Texture::set_active(0);
    grassTx.bind();
    grassTx.set_data(0);

    gl::VertexArray::bind(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    gl::VertexArray::unbind();

    gl::Texture::unbind();
    ShaderProgram::unuse();
  }

  void clear() {
    grassTx.clear();
    ShaderAttrib::clear(attrVertex);
    gl::VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }
};
