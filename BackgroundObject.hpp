#pragma once

#include "Camera.hpp"
#include "Shader.hpp"
#include "ShaderProgram.hpp"
#include "VertexArray.hpp"
#include "ShaderBuffer.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"
#include "Tuple.hpp"

struct BackgroundObject {
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::Buffer<GL_ARRAY_BUFFER, gl::BufferElementType::VEC2> buf;
  gl::Attrib<decltype(buf)> attrVertex;
  gl::VertexArray<decltype(attrVertex)> vao;

  using ShaderProgram = decltype(program);
  using ShaderBuffer = decltype(buf);
  using ShaderAttrib = decltype(attrVertex);
  using VertexArray = decltype(vao);

  BackgroundObject(const std::string &dir):
    program({
      sys::Path(dir) / sys::Path("shaders"s) / sys::Path("bg.vert"s),
      sys::Path(dir) / sys::Path("shaders"s) / sys::Path("bg.frag"s)
    }),
    attrVertex("vertex"s, buf),
    vao(attrVertex)
  {}

  void init() {
    ShaderBuffer::init(buf);
    buf.allocate<GL_STREAM_DRAW>(std::vector<float>{
      -1,-1, 1,-1, 1,1,
      1,1, -1,1, -1,-1,
    });

    VertexArray::init(vao);
    VertexArray::bind(vao);

    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0);

    VertexArray::unbind();

    ShaderProgram::init(program, vao);
  }

  void display(Camera &cam) {
    ShaderProgram::use(program);

    VertexArray::bind(vao);
    VertexArray::draw<GL_TRIANGLES>(vao);
    VertexArray::unbind();

    ShaderProgram::unuse();
  }

  void clear() {
    ShaderAttrib::clear(attrVertex);
    ShaderBuffer::clear(buf);
    VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }
};
