#pragma once

#include "Camera.hpp"
#include "Shader.hpp"
#include "ShaderProgram.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"
#include "Tuple.hpp"

struct BackgroundObject {
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrVertex;

  using ShaderAttrib = decltype(attrVertex);
  using ShaderProgram = decltype(program);

  BackgroundObject():
    program({"shaders/bg.vert", "shaders/bg.frag"}),
    attrVertex("vertex")
  {}

  void init() {
    ShaderAttrib::init(attrVertex);
    attrVertex.allocate<GL_STREAM_DRAW>(6, std::vector<float>{
      -1,-1, 1,-1, 1,1,
      1,1, -1,1, -1,-1,
    });

    gl::VertexArray::init(vao);
    gl::VertexArray::bind(vao);
    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0, 0);
    gl::VertexArray::unbind();
    ShaderProgram::init(program, vao, {"attrVertex"});
  }

  void display(Camera &cam) {
    ShaderProgram::use(program);

    gl::VertexArray::bind(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    gl::VertexArray::unbind();

    ShaderProgram::unuse();
  }

  void clear() {
    ShaderAttrib::clear(attrVertex);
    gl::VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }
};
