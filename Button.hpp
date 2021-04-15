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
#include "StrConst.hpp"

namespace ui {
template <typename BUTTON_FILENAME, typename FONT_FILENAME>
struct Button {
  // workaround for "error: cannot compile this scalar expression yet" from clang
  const std::string btnquadf = "shaders/btn_quad.frag";
  const std::string btntextf = "shaders/btn_text.frag";

  using self_t = Button<BUTTON_FILENAME, FONT_FILENAME>;
  Sprite<ui::Font, self_t> *font;
  ui::Text label;
  gl::Texture btnTx;
  static constexpr int DEFAULT_STATE = 0;
  static constexpr int POINTED_STATE = 1;
  static constexpr int SELECTED_STATE = 2;
  int state = DEFAULT_STATE;
  gl::Uniform<gl::UniformType::INTEGER> uState;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > quadProgram;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > textProgram;
  gl::Buffer<GL_ARRAY_BUFFER, gl::BufferElementType::VEC2> buf;
  gl::Attrib<decltype(buf)> attrVertex;
  gl::VertexArray<decltype(attrVertex)> vao;

  Region region;

  using ShaderBuffer = decltype(buf);
  using ShaderAttrib = decltype(attrVertex);
  using ShaderProgramQuad = decltype(quadProgram);
  using ShaderProgramText = decltype(textProgram);
  using VertexArray = decltype(vao);

  Button(Region region=Region(glm::vec2(-1,1), glm::vec2(-1,1))):
    font(Sprite<ui::Font, self_t>::create(FONT_FILENAME::c_str)),
    label(font->object),
    btnTx("btn"),
    uState("state"),
    quadProgram({"shaders/btn_quad.vert"s, btnquadf}),
    textProgram({"shaders/btn_text.vert"s, btntextf}),
    attrVertex("vertex", buf),
    vao(attrVertex),
    region(region)
  {}

  void init() {
    ShaderBuffer::init(buf);
    buf.allocate<GL_STREAM_DRAW>(std::vector<float>{
      1,1, -1,1, -1,-1,
      -1,-1, 1,-1, 1,1,
    });

    VertexArray::init(vao);
    attrVertex.select_buffer(buf);
    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0);

    ShaderProgramQuad::init(quadProgram, vao);

    font->init();
    label.init(textProgram);
    btnTx.init(BUTTON_FILENAME::c_str);
    btnTx.uSampler.set_id(quadProgram.id());
    uState.set_id(quadProgram.id());
  }

  void setx(float l, float r) {
    if(l > r)std::swap(l, r);
    region.xs = glm::vec2(l, r);
  }
  void sety(float t, float b) {
    if(t > b)std::swap(t, b);
    region.ys = glm::vec2(t, b);
  }

  void mouse(float m_x, float m_y) {
    float s_x = 2 * m_x - 1;
    float s_y = 2 * m_y - 1;
    /* printf("button mouse: %f %f\n", m_x, m_y); */
    /* printf("xs: %f %f\n", region.x1(), region.x2()); */
    /* printf("ys: %f %f\n", region.y1(), region.y2()); */
    /* printf("contains: %d\n", region.contains(m_x, m_y)); */
    /* printf("contains x: %d\n", region.x1() <= m_x && m_x <= region.x2()); */
    /* printf("contains y: %d\n", region.y1() <= m_y && m_y <= region.y2()); */
    if(state == DEFAULT_STATE && region.contains(s_x, s_y)) {
      state = POINTED_STATE;
    } else if(state == POINTED_STATE && !region.contains(s_x, s_y)) {
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
      clicked = false;
    }
  }

  void display() {
    label.transform.SetScale((region.x2()-region.x1())/2, (region.y2()-region.y1())/2, 1.);
    label.pos = { region.center().x, region.center().y };

    // display quad
    ShaderProgramQuad::use(quadProgram);

    gl::Texture::set_active(0);
    btnTx.bind();
    btnTx.set_data(0);

    uState.set_data(state);

    VertexArray::bind(vao);
    auto m = label.transform.get_matrix();
    label.uTransform.set_id(quadProgram.id());
    label.uTransform.set_data(m);
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    label.uTransform.unset_id();
    VertexArray::unbind();

    gl::Texture::unbind();
    ShaderProgramQuad::unuse();

    // display text
    /* label.transform.SetScale(1, 1, 1); */
    label.color = glm::vec3(1, 1, 1);
    label.display(textProgram);
  }

  void clear() {
    ShaderAttrib::clear(attrVertex);
    ShaderBuffer::clear(buf);
    VertexArray::clear(vao);
    btnTx.clear();
    label.clear();
    ShaderProgramQuad::clear(quadProgram);
    ShaderProgramText::clear(textProgram);
    font->clear();
  }
};

} // namespace ui
