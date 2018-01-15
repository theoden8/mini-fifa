#pragma once

#include <cctype>
#include <string>
#include <glm/glm.hpp>

#include "incgraphics.h"
#include "freetype_config.h"

#include "Transformation.hpp"
#include "ShaderProgram.hpp"
#include "ShaderUniform.hpp"
#include "ShaderAttrib.hpp"
#include "Debug.hpp"
#include "Font.hpp"

struct Text {
  Font &font;
  std::string str = "";

  glm::vec2 pos = {0, 0};
  glm::vec3 color = {1.f,1.f,1.f};
  glm::mat4 matrix;
  Transformation transform;

  gl::Uniform<gl::UniformType::VEC3> uTextColor;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC4> vbo;
  float width_ = 0, height_ = 0;

  using ShaderAttrib = decltype(vbo);

  Text(Font &font):
    uTransform("transform"),
    uTextColor("textcolor"),
    font(font)
  {
    transform.Scale(1.f);
    transform.SetPosition(0, 0, 0);
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

  void init() {
    gl::VertexArray::init(vao);
    gl::VertexArray::bind(vao);
    ShaderAttrib::init(vbo);
    ShaderAttrib::bind(vbo);
    vbo.allocate<GL_DYNAMIC_DRAW>(6, nullptr);
    vao.enable(vbo);
    vao.set_access(vbo);
    ShaderAttrib::unbind();
    gl::VertexArray::unbind();
  }

  template <typename... ShaderTs>
  void display(gl::ShaderProgram<ShaderTs...> &program) {
    glEnable(GL_BLEND); GLERROR
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLERROR
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); GLERROR

    gl::ShaderProgram<ShaderTs...>::use(program);

    uTextColor.set_id(program.id());
    uTextColor.set_data(color);

    transform.Scale(xscale, yscale, 1);
    transform.SetPosition(pos.x, -pos.y, 0);
    matrix = transform.get_matrix();
    uTransform.set_id(program.id());
    uTransform.set_data(matrix);

    gl::Texture::set_active(0);
    gl::VertexArray::bind(vao);
    float x = 0, y = 0;
    transform.MovePosition(-.5*width(), -.5*height(), 0);
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
      GLfloat vertices[6][4] = {
        { posx,     posy + h,   0.0, 0.0 },
        { posx,     posy,       0.0, 1.0 },
        { posx + w, posy,       1.0, 1.0 },

        { posx,     posy + h,   0.0, 0.0 },
        { posx + w, posy,       1.0, 1.0 },
        { posx + w, posy + h,   1.0, 0.0 },
      };
      ch.tex.uSampler.set_id(program.id());
      gl::Texture::bind(ch.tex);
      ch.tex.set_data(0);
      ShaderAttrib::bind(vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); GLERROR
      ShaderAttrib::unbind();
      glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
      x += (ch.advance >> 6);
    }
    gl::VertexArray::unbind();
    gl::Texture::unbind();

    transform.MovePosition(.5*width(), .5*height(), 0);
    transform.Scale(1./xscale, 1./yscale, 1);

    gl::ShaderProgram<ShaderTs...>::unuse();

    glDisable(GL_BLEND); GLERROR
  }

  void clear() {
    ShaderAttrib::clear(vbo);
    gl::VertexArray::clear(vao);
  }
};
