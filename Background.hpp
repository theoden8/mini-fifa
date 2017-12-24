#pragma once

#include "Camera.hpp"
#include "Shader.hpp"
#include "ShaderProgram.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"
#include "Tuple.hpp"

struct Background {
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrVertices;

  Background():
    program({"bg.vert", "bg.frag"}),
    attrVertices("vertex")
  {}

  void init() {
    attrVertices.init();
    attrVertices.allocate<GL_STREAM_DRAW>(6, std::vector<float>{
      -1,-1, 1,-1, 1,1,
      1,1, -1,1, -1,-1,
    });

    vao.init();
    vao.bind();
    vao.enable(attrVertices);
    vao.set_access(attrVertices, 0, 0);
    gl::VertexArray::unbind();
    program.init(vao, {"attrVertices"});
  }

  void display(Camera &cam) {
    program.use();

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    gl::VertexArray::unbind();

    decltype(program)::unuse();
  }

  void clear() {
    attrVertices.clear();
    vao.clear();
    program.clear();
  }
};
