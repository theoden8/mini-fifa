#pragma once

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
  glm::vec3 clr = {1.f,1.f,1.f};
  glm::mat4 matrix;
  Transformation transform;

  gl::Uniform<gl::UniformType::VEC3> uTextColor;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::VertexArray vao;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC4> vbo;
  size_t width_ = 0, height_ = 0;

  Text(Font &font):
    uTransform("transform"),
    uTextColor("textcolor"),
    font(font)
  {
    transform.Scale(1.f);
    transform.SetRotation(0, 0, 1, 0.f);
    transform.SetPosition(0, 0, 0);
  }

  size_t width() const { return width_; }
  size_t height() const { return height_; }

  void set_text(std::string &&s) {
    str = s;
    width_ = height_ = 0;
    for(const char *c = str.c_str(); *c != '\0'; ++c) {
      Font::Character &ch = font.alphabet.at(*c);
      width_ += ch.size.x + ch.bearing.x,
      height_ += ch.size.y - ch.bearing.y;
    }
  }

  glm::vec2 &position() { return pos; }
  glm::vec3 &color() { return clr; }

  void init() {
    vao.init();
    vao.bind();
    vbo.init();
    vbo.bind();
    vbo.allocate<GL_DYNAMIC_DRAW>(6, nullptr);
    vao.enable(vbo);
    vao.set_access(vbo);
    decltype(vbo)::unbind();
    gl::VertexArray::unbind();
  }

  template <typename... ShaderTs>
  void display(gl::ShaderProgram<ShaderTs...> &program) {
    gl::ShaderProgram<ShaderTs...>::use(program);

    uTextColor.set_id(program.id());
    uTextColor.set_data(clr);

    matrix = transform.get_matrix();
    uTransform.set_id(program.id());
    uTransform.set_data(matrix);

    gl::Texture::set_active(0);
    gl::VertexArray::bind(vao);
    GLfloat x = pos.x, y = pos.y;
    for(char c : str) {
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
      ch.tex.bind();
      ch.tex.set_data(0);
      vbo.bind();
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); GLERROR
      decltype(vbo)::unbind();
      glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
      x += (ch.advance >> 6);
    }
    gl::VertexArray::unbind();
    gl::Texture::unbind();

    gl::ShaderProgram<ShaderTs...>::unuse();
  }

  void clear() {
    vbo.clear();
    vao.clear();
  }
};
