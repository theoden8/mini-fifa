#pragma once

#include <string>

#include "incgraphics.h"

#include "ShaderProgram.hpp"
#include "ShaderUniform.hpp"

#include "PNGImage.hpp"
#include "JPEGImage.hpp"
#include "BMPImage.hpp"

namespace gl {
struct Texture {
  GLuint tex;
  gl::Uniform<gl::UniformType::SAMPLER2D> uSampler;

  Texture(std::string uniform_name):
    uSampler(uniform_name.c_str())
  {}

  static img::Image *make_image(File &file) {
    Logger::Info("Loading image file '%s'\n", file.name().c_str());
    if(!file.exists()) {
      Logger::Info("file '%s' not found", file.name().c_str());
      ASSERT(file.exists());
    }
    if(file.is_ext(".png")) {
      return new img::PNGImage(file.name().c_str());
    } else if(file.is_ext(".jpg") || file.is_ext(".jpeg")) {
      return new img::JPEGImage(file.name().c_str());
    /* } else if(file.is_ext(".tiff")) { */
    /*   return new img::TIFFImage(file.name().c_str()); */
    } else if(file.is_ext(".bmp")) {
      return new img::BMPImage(file.name().c_str());
    }
    Logger::Error("unsupported file format for %s\n", file.name().c_str());
    throw std::domain_error("invalid image format for " + file.name());
    return nullptr;
  }

  void init_white_square() {
    uint32_t *image = new uint32_t[100*100];
    for(int i = 0; i < 100*100; ++i) {
      image[i] = UINT32_MAX;
    }
    glGenTextures(1, &tex); GLERROR
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 100, 100, 0, GL_RGBA, GL_UNSIGNED_BYTE, image); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GLERROR
    glGenerateMipmap(GL_TEXTURE_2D); GLERROR
    Texture::unbind();
    delete [] image;
  }

  void init(std::string filename) {
    /* init(); return; */
    /* long c=clock(); */
    Logger::Info("Loading texture '%s'\n", filename.c_str());
    File file(filename.c_str());
    /* c=clock(); */
    img::Image *image = make_image(file);
    image->init();
    /* Logger::Info("load image '%s': %ld\n", filename.c_str(), clock()-c); */
    /* std::cout << "load image '" << filename << "': " << clock()-c << std::endl; */
    /* c=clock(); */
    glGenTextures(1, &tex); GLERROR
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, image->format, image->width, image->height, 0, image->format, GL_UNSIGNED_BYTE, image->data); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); GLERROR
    glGenerateMipmap(GL_TEXTURE_2D); GLERROR
    Texture::unbind();
    /* Logger::Info("create texture '%s': %ld\n", filename.c_str(), clock()-c); */
    /* std::cout << "create texture '" << filename << "': " << clock()-c << std::endl; */
    image->clear();
    delete image;
    Logger::Info("Finished loading texture '%s'.\n", filename.c_str());
  }

  /* void init(FT_GlyphSlot *glyph) { */
  /*   glPixelStorei(GL_UNPACK_ALIGNMENT, 1); GLERROR */
  /*   glGenTextures(1, &tex); GLERROR */
  /*   glBindTexture(GL_TEXTURE_2D, tex); GLERROR */
  /*   glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (*glyph)->bitmap.width, (*glyph)->bitmap.rows, */
  /*                0, GL_RED, GL_UNSIGNED_BYTE, (*glyph)->bitmap.buffer); GLERROR */
  /*   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); GLERROR */
  /*   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); GLERROR */
  /*   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLERROR */
  /*   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GLERROR */
  /*   unbind(); */
  /* } */

  static void set_active(int index) {
    glActiveTexture(GL_TEXTURE0 + index); GLERROR
  }

  void bind() {
    glBindTexture(GL_TEXTURE_2D, tex); GLERROR
  }

  void set_data(int index) {
    uSampler.set_data(index);
  }

  static void unbind() {
    glBindTexture(GL_TEXTURE_2D, 0); GLERROR
  }

  void clear() {
    glDeleteTextures(1, &tex);
  }
};
}
