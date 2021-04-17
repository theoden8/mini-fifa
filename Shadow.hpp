#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Shader.hpp"
#include "ShaderUniform.hpp"
#include "ShaderBuffer.hpp"
#include "ShaderAttrib.hpp"
#include "VertexArray.hpp"
#include "ShaderProgram.hpp"

struct Shadow {
  glm::mat4 matrix;
  Transformation transform;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::Buffer<GL_ARRAY_BUFFER, gl::BufferElementType::VEC2> buf;
  gl::Attrib<decltype(buf)> attrVertex;
  gl::VertexArray<decltype(attrVertex)> vao;

  using ShaderBuffer = decltype(buf);
  using ShaderAttrib = decltype(attrVertex);
  using ShaderProgram = decltype(program);
  using VertexArray = decltype(vao);

  Shadow(const std::string &dir):
    transform(),
    program({
      sys::Path(dir) / sys::Path("shaders"s) / sys::Path("shadow.vert"s),
      sys::Path(dir) / sys::Path("shaders"s) / sys::Path("shadow.frag"s)
    }),
    uTransform("transform"),
    attrVertex("vertex"s, buf),
    vao(attrVertex)
  {
  }

  void init() {
    ShaderBuffer::init(buf);
    buf.allocate<GL_STREAM_DRAW>(std::vector<float>{
      -1,-1, 1,-1, 1,1,
      1,1, -1,1, -1,-1,
    });

    VertexArray::init(vao);
    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0);

    ShaderProgram::init(program, vao);
    uTransform.set_id(program.id());
  }

  void display(Camera &cam) {
    ShaderProgram::use(program);

    glEnable(GL_BLEND); GLERROR
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLERROR
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); GLERROR

    if(cam.has_changed || transform.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    VertexArray::draw<GL_TRIANGLES>(vao);

    glDisable(GL_BLEND); GLERROR

    ShaderProgram::unuse();
  }

  void clear() {
    ShaderAttrib::clear(attrVertex);
    ShaderBuffer::clear(buf);
    VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }
};
