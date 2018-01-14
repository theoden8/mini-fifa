#pragma once

#include <vector>

#include "Transformation.hpp"
#include "ShaderUniform.hpp"
#include "ShaderProgram.hpp"
#include "Texture.hpp"
#include "Timer.hpp"
#include "Pitch.hpp"

struct Cursor {
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
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrVertex;

  using ShaderAttrib = decltype(attrVertex);
  using ShaderProgram = decltype(program);

  State state = State::POINTER;

  Cursor():
    uTransform("transform"),
    pointerTx("cursorTx"),
    selectorTx("cursorTx"),
    program({"shaders/cursor.vert", "shaders/cursor.frag"})
  {
    transform.SetScale(.02f);
    transform.SetPosition(0, 0, 0);
    transform.SetRotation(0, 0, 1, 270.f);
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
    return transform.GetPosition();
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

    gl::VertexArray::bind(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    gl::VertexArray::unbind();

    gl::Texture::unbind();
    decltype(program)::unuse();
    glDisable(GL_BLEND); GLERROR
  }

  void clear() {
    pointerTx.clear();
    selectorTx.clear();
    ShaderAttrib::clear(attrVertex);
    gl::VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }
};
