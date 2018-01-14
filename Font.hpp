#pragma once

#include <map>
#include <glm/glm.hpp>

#include "freetype_config.h"
#include "Debug.hpp"
#include "Logger.hpp"
#include "Texture.hpp"

struct Font {
  static FT_Library ft;
  FT_Face face;
  struct Character {
    gl::Texture tex;
    glm::ivec2 size;
    glm::ivec2 bearing;
    GLuint advance;

    Character(gl::Texture tex, glm::ivec2 size, glm::ivec2 bearing, GLuint advance):
      tex(tex), size(size), bearing(bearing), advance(advance)
    {}
  };
  std::map<GLchar, Character> alphabet;
  const char *filename;

  Font(const char *filename):
    filename(filename)
  {}

  static void setup() {
    int rc = FT_Init_FreeType(&ft);
    ASSERT(!rc);
  }

  void init() {
    int rc = FT_New_Face(ft, filename, 0, &face);
    ASSERT(!rc);
    FT_Set_Pixel_Sizes(face, 0, 48);

    for(GLubyte c = 0; c < 128; ++c) {
      int rc = FT_Load_Char(face, c, FT_LOAD_RENDER);
      ASSERT(!rc);
      alphabet.at(c) = Character(
        gl::Texture("font_texture"),
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        GLuint(face->glyph->advance.x)
      );
      alphabet.at(c).tex.init(&face->glyph);
    }
    Logger::Info("Initialized font from file %s\n", filename);
  }

  void clear() {
    FT_Done_Face(face);
    for(auto &it : alphabet) {
      it.second.tex.clear();
    }
  }

  static void cleanup() {
    FT_Done_FreeType(ft);
  }
};
FT_Library Font::ft;
