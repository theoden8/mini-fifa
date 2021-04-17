#pragma once

#include <map>
#include <glm/glm.hpp>

#include "incfreetype.h"
#include "Debug.hpp"
#include "Logger.hpp"
#include "Texture.hpp"

namespace ui {
struct Font {
  static FT_Library ft;
  static bool initialized;
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
  std::string filename;

  Font(const std::string &filename):
    filename(filename)
  {}

  static void setup() {
    int rc = FT_Init_FreeType(&ft);
    ASSERT(!rc);
    initialized = true;
  }

  void init() {
    ASSERT(initialized);
    int rc = FT_New_Face(ft, filename.c_str(), 0, &face);
    ASSERT(!rc);
    FT_Set_Pixel_Sizes(face, 0, 48);

    for(GLubyte c = 0; c < 128; ++c) {
      int rc = FT_Load_Char(face, c, FT_LOAD_RENDER);
      ASSERT(!rc);
      alphabet.insert({c, Character(
        gl::Texture("font_texture"),
        glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
        glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
        GLuint(face->glyph->advance.x)
      )});
      ASSERT(alphabet.find(c) != std::end(alphabet));
      alphabet.at(c).tex.init(&face->glyph);
    }
    Logger::Info("Initialized font from file %s\n", filename.c_str());
  }

  void clear() {
    ASSERT(initialized);
    FT_Done_Face(face);
    for(auto &it : alphabet) {
      it.second.tex.clear();
    }
  }

  static void cleanup() {
    FT_Done_FreeType(ft);
    initialized = false;
  }
};
FT_Library Font::ft;
bool Font::initialized = false;
}
