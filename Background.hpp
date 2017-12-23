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
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC3> attrTriangle;
  gl::Texture skyTx;

  Background():
    program({"bg.vert", "bg.frag"}),
    attrTriangle("triangle"),
    skyTx("sky")
  {}

  void init() {
    attrTriangle.init();
    attrTriangle.allocate<GL_STREAM_DRAW>(6, std::vector<float>{
      -1,-1,0, 1,-1,0, 1,1,0,
      1,1,0, -1,1,0, -1,-1,0,
    });

    vao.init();
    vao.bind();
    vao.enable(attrTriangle);
    vao.set_access(attrTriangle, 0, 0);
    gl::VertexArray::unbind();
    program.init(vao, {"attrTriangle"});
  }

  void display(Camera &cam) {
    program.use();

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    gl::VertexArray::unbind();

    gl::Texture::unbind();
    decltype(program)::unuse();
  }

  void clear() {
    attrTriangle.clear();
    vao.clear();
    program.clear();
  }
};
