#pragma once

#include <vector>

#include "Transformation.hpp"
#include "ShaderProgram.hpp"
#include "Texture.hpp"
#include "Timer.hpp"
#include "Region.hpp"

namespace ui {
struct CursorObject {
  Transformation transform;

  enum class State {
    POINTER,
    SELECTOR
  };

  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::Texture pointerTx;
  gl::Texture selectorTx;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::Buffer<GL_ARRAY_BUFFER, gl::BufferElementType::VEC2> buf;
  gl::Attrib<decltype(buf)> attrVertex;
  gl::VertexArray<decltype(attrVertex)> vao;

  using ShaderBuffer = decltype(buf);
  using ShaderAttrib = decltype(attrVertex);
  using ShaderProgram = decltype(program);
  using VertexArray = decltype(vao);

  State state = State::POINTER;

  CursorObject():
    uTransform("transform"),
    pointerTx("cursorTx"),
    selectorTx("cursorTx"),
    program({"shaders/cursor.vert", "shaders/cursor.frag"}),
    attrVertex("vertex", buf),
    vao(attrVertex)
  {
    transform.SetScale(.02f);
    transform.SetPosition(0, 0, 0);
    transform.SetRotation(0, 0, 1, 270.f);
  }

  void init() {
    ShaderBuffer::init(buf);
    buf.allocate<GL_STREAM_DRAW>(std::vector<float>{
      1,1, -1,1, -1,-1,
      -1,-1, 1,-1, 1,1,
    });

    VertexArray::init(vao);
    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0, 0);

    ShaderProgram::init(program, vao);
    pointerTx.init("assets/pointer.png");
    selectorTx.init("assets/selector.png");
    pointerTx.uSampler.set_id(program.id());
    selectorTx.uSampler.set_id(program.id());
    uTransform.set_id(program.id());
  }

  void mouse(float m_x, float m_y) {
    transform.SetPosition(m_x*2-1 + .02, -(m_y*2-1 + .02), .5);
  }

  glm::vec2 get_position() const {
    glm::vec4 pos = transform.GetPosition();
    return {pos.x, pos.y};
  }

  void display() {
    glEnable(GL_BLEND); GLERROR
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLERROR
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); GLERROR

    ShaderProgram::use(program);

    if(transform.has_changed) {
      glm::mat4 matrix = transform.get_matrix();
      uTransform.set_data(matrix);

      /* transform.has_changed = false; */
    }

    gl::Texture::set_active(0);
    if(state == State::POINTER) {
      pointerTx.bind();
      pointerTx.set_data(0);
    } else {
      selectorTx.bind();
      selectorTx.set_data(0);
    }

    VertexArray::draw<GL_TRIANGLES>(vao);

    gl::Texture::unbind();
    decltype(program)::unuse();
    glDisable(GL_BLEND); GLERROR
  }

  void clear() {
    pointerTx.clear();
    selectorTx.clear();
    ShaderAttrib::clear(attrVertex);
    ShaderBuffer::clear(buf);
    VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }
};
}
