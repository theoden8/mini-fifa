#pragma once

#include <cctype>
#include <string>
#include <glm/glm.hpp>

#include "incgraphics.h"
#include "incfreetype.h"

#include "Transformation.hpp"
#include "ShaderProgram.hpp"
#include "Debug.hpp"
#include "Font.hpp"

namespace ui {
struct Text {
  Font &font;
  std::string str = "";

  glm::vec2 pos = {0, 0};
  glm::vec3 color = {1.f,1.f,1.f};
  glm::mat4 matrix;
  Transformation transform;

  gl::Uniform<gl::UniformType::VEC3> uTextColor;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::Buffer<GL_ARRAY_BUFFER, gl::BufferElementType::VEC4> buf;
  gl::Attrib<decltype(buf)> attr;
  gl::VertexArray<decltype(attr)> vao;
  float width_ = 0, height_ = 0;

  using ShaderBuffer = decltype(buf);
  using ShaderAttrib = decltype(attr);
  using VertexArray = decltype(vao);

  Text(Font &font):
    uTransform("transform"),
    uTextColor("textcolor"),
    attr("vertex", buf),
    vao(attr),
    font(font)
  {
    transform.Scale(1.f);
    transform.SetPosition(0, 0, 0);
    transform.SetRotation(1, 0, 0, 0);
  }

  float width() const { return width_ * transform.GetScale().x; }
  float height() const { return height_ * transform.GetScale().y; }

  float yscale = 1./1024;
  float xscale = yscale;

  void set_text(std::string s) {
    str = s;
    width_ = height_ = 0;
    for(int i = 0; i < s.length(); ++i) {
      char c = s[i];
      ASSERT(isascii(c));
      ASSERT(font.alphabet.find(c) != font.alphabet.end());
      Font::Character &ch = font.alphabet.at(c);
      width_ += ch.size.x + ch.bearing.x,
      height_ += ch.size.y - ch.bearing.y;
      if(i < s.length() - 1) {
        width_ += ch.advance >> 6;
      }
    }
  }

  template <typename... ShaderTs>
  void init(gl::ShaderProgram<ShaderTs...> &program) {
    using ShaderProgram = gl::ShaderProgram<ShaderTs...>;

    ShaderBuffer::init(buf);
    buf.allocate_with_overlap<GL_DYNAMIC_DRAW>(std::vector<float>(6*4, 0));

    VertexArray::init(vao);
    attr.select_buffer(buf);
    vao.enable(attr);
    vao.set_access(attr, 0);

    ShaderProgram::init(program, vao);
  }

  enum class Positioning {
    LEFT, CENTER
  };
  template <typename... ShaderTs>
  void display(gl::ShaderProgram<ShaderTs...> &program, Positioning pst=Positioning::CENTER, float scale=1.) {
    using ShaderProgram = gl::ShaderProgram<ShaderTs...>;

    ShaderProgram::use(program);

    glEnable(GL_BLEND); GLERROR
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLERROR
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); GLERROR

    uTextColor.set_id(program.id());
    uTextColor.set_data(color);

    auto init_scale = transform.GetScale();
    auto init_pos = transform.GetPosition();
    transform.SetScale(scale * xscale, scale * yscale, 1);
    transform.SetPosition(pos.x, -pos.y, 0);
    matrix = transform.get_matrix();
    uTransform.set_id(program.id());
    uTransform.set_data(matrix);

    gl::Texture::set_active(0);
    VertexArray::bind(vao);
    float x = 0, y = 0;
    /* if(pst == Positioning::CENTER) { */
    /*   transform.MovePosition(-.5*width(), .5*height(), 0); */
    /* } */
    for(char c : str) {
      ASSERT(isascii(c));
      ASSERT(font.alphabet.find(c) != font.alphabet.end());
      Font::Character &ch = font.alphabet.at(c);
      GLfloat
        posx = x + ch.bearing.x,
        posy = y - (ch.size.y - ch.bearing.y);
      GLfloat
        w = ch.size.x,
        h = ch.size.y;
      std::vector<GLfloat> vertices = {
        posx,     posy + h,   0.0, 0.0,
        posx,     posy,       0.0, 1.0,
        posx + w, posy,       1.0, 1.0,

        posx,     posy + h,   0.0, 0.0,
        posx + w, posy,       1.0, 1.0,
        posx + w, posy + h,   1.0, 0.0,
      };
      ch.tex.uSampler.set_id(program.id());
      gl::Texture::bind(ch.tex);
      ch.tex.set_data(0);
      buf.set_subdata(vertices);
      VertexArray::draw<GL_TRIANGLES>(vao, 0, 6);
      x += (ch.advance >> 6);
    }
    VertexArray::unbind();
    gl::Texture::unbind();

    /* transform.MovePosition(.5*width(), -.5*height(), 0); */
    /* transform.Scale(1./xscale, 1./yscale, 1); */
    transform.SetScale(init_scale);
    /* transform.SetPosition(init_pos); */

    glDisable(GL_BLEND); GLERROR

    ShaderProgram::unuse();
  }

  void clear() {
    ShaderAttrib::clear(attr);
    ShaderBuffer::clear(buf);
    VertexArray::clear(vao);
  }
};
} // namespace ui
