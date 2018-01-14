#pragma once

#include <array>
#include <string>

#include "Transformation.hpp"
#include "Region.hpp"
#include "Camera.hpp"
#include "Sprite.hpp"
#include "ShaderProgram.hpp"
#include "Shader.hpp"
#include "ShaderUniform.hpp"
#include "ShaderAttrib.hpp"
#include "Texture.hpp"
#include "Text.hpp"

template <typename BUTTON_FILENAME, typename FONT_FILENAME>
struct Button {
  using self_t = Button<BUTTON_FILENAME, FONT_FILENAME>;
  Text label;
  Sprite<Font, self_t> *font;
  gl::Texture btnTx;
  gl::VertexArray vao;
  static constexpr int DEFAULT_STATE = 0;
  static constexpr int POINTED_STATE = 1;
  static constexpr int SELECTED_STATE = 1;
  int state = DEFAULT_STATE;
  gl::Uniform<gl::UniformType::INTEGER> uState;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > quadProgram, textProgram;
  /* gl::ShaderProgram< */
  /*   gl::VertexShader, */
  /*   gl::FragmentShader */
  /* > textProgram; */
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrVertex;

  Region region;

  using ShaderAttrib = decltype(attrVertex);
  using ShaderProgramQ = decltype(quadProgram);
  using ShaderProgramT = decltype(textProgram);

  Button(Region region=Region(glm::vec2(-1,1), glm::vec2(-1,1))):
    font(Sprite<Font, self_t>::create(FONT_FILENAME::c_str)),
    quadProgram({"shaders/btn_quad.vert", "shaders/btn_quad.frag"}),
    textProgram({"shaders/btn_text.vert", "shaders/btn_text.frag"}),
    label(font->object),
    region(region),
    attrVertex("vertex"),
    btnTx("btnTx"),
    uState("state")
  {}

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
    ShaderProgramQ::init(quadProgram, vao, {"attrVertex"});
    ShaderProgramT::compile_program(textProgram);
    font->init();
    label.init();
    btnTx.init(BUTTON_FILENAME::c_str);
    btnTx.uSampler.set_id(quadProgram.id());
  }

  void mouse(float m_x, float m_y) {
    if(state == DEFAULT_STATE && region.contains(m_x, m_y)) {
      state = POINTED_STATE;
    } else if(state == POINTED_STATE && !region.contains(m_x, m_y)) {
      state = DEFAULT_STATE;
    }
  }

  bool clicked = false;
  void mouse_click(int button, int action) {
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
      if(action == GLFW_PRESS) {
        if(state == POINTED_STATE) {
          state = SELECTED_STATE;
        }
      } else if(action == GLFW_RELEASE) {
        if(state == SELECTED_STATE) {
          state = POINTED_STATE;
          clicked = true;
        }
      }
    } else if(button == GLFW_MOUSE_BUTTON_RIGHT) {
      if(action == GLFW_PRESS && state == SELECTED_STATE) {
        state = POINTED_STATE;
      }
    }
  }

  template <typename F>
  void action_on_click(F func) {
    if(clicked) {
      func();
    }
  }

  void display() {
    label.transform.SetScale(region.x2()-region.x1(), region.y2()-region.y1(), 1.);
    label.position() = { region.x1(), region.y1() };
    label.display(textProgram);

    ShaderProgramQ::use(quadProgram);

    gl::Texture::set_active(0);
    btnTx.bind();
    btnTx.set_data(0);

    uState.set_data(state);

    gl::VertexArray::bind(vao);
    label.uTransform.set_id(quadProgram.id());
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    label.uTransform.unset_id();
    gl::VertexArray::unbind();

    gl::Texture::unbind();
    ShaderProgramQ::unuse();
  }

  void clear() {
    ShaderAttrib::clear(attrVertex);
    gl::VertexArray::clear(vao);
    btnTx.clear();
    label.clear();
    ShaderProgramQ::clear(quadProgram);
    ShaderProgramT::clear(textProgram);
    font->clear();
  }
};
