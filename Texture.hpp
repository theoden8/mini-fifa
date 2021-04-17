#pragma once

#include <string>

#include "incgraphics.h"
#include "incfreetype.h"

#include "ShaderProgram.hpp"
#include "ShaderUniform.hpp"

#include "ImageLoader.hpp"

namespace gl {
struct Texture {
  GLuint tex;
  gl::Uniform<gl::UniformType::SAMPLER2D> uSampler;

  Texture(std::string uniform_name):
    uSampler(uniform_name.c_str())
  {}

  static constexpr GLenum get_gl_pixel_format(img::Image::Format format) {
    switch(format) {
      case img::Image::Format::ALPHA8: return GL_ALPHA8;
      case img::Image::Format::RGB: return GL_RGB;
      case img::Image::Format::RGBA: return GL_RGBA;
      case img::Image::Format::BGR: return GL_BGR;
      case img::Image::Format::BGRA: return GL_BGRA;
    }
    return UINT_MAX;
  }

  GLuint id() const { return tex; }

  void init_white_square() {
    // 100 x 100 RBGA (1., 1., 1., 1.) image
    uint32_t *image = new uint32_t[100*100];
    for(int i = 0; i < 100*100; ++i) {
      image[i] = UINT32_MAX;
    }
    glGenTextures(1, &tex); GLERROR
    gl::Texture::bind(tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 100, 100, 0, GL_RGBA, GL_UNSIGNED_BYTE, image); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GLERROR
    glGenerateMipmap(GL_TEXTURE_2D); GLERROR
    gl::Texture::unbind();
    delete [] image;
  }

  void init(const std::string &filename) {
    /* init(); return; */
    /* long c=clock(); */
    Logger::Info("Loading texture '%s'\n", filename.c_str());
    sys::File file(filename.c_str());
    /* c=clock(); */
    img::Image *image = img::load_image(file);
    /* Logger::Info("load image '%s': %ld\n", filename.c_str(), clock()-c); */
    /* std::cout << "load image '" << filename << "': " << clock()-c << std::endl; */
    /* c=clock(); */
    glGenTextures(1, &tex); GLERROR
    gl::Texture::bind(tex);
    glTexImage2D(GL_TEXTURE_2D, 0, get_gl_pixel_format(image->format), image->width, image->height, 0, get_gl_pixel_format(image->format), GL_UNSIGNED_BYTE, image->data); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); GLERROR
    glGenerateMipmap(GL_TEXTURE_2D); GLERROR
    gl::Texture::unbind();
    /* Logger::Info("create texture '%s': %ld\n", filename.c_str(), clock()-c); */
    /* std::cout << "create texture '" << filename << "': " << clock()-c << std::endl; */
    delete image;
    Logger::Info("Finished loading texture '%s'.\n", filename.c_str());
  }

  void init(FT_GlyphSlot *glyph) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); GLERROR
    glGenTextures(1, &tex); GLERROR
    gl::Texture::bind(tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, (*glyph)->bitmap.width, (*glyph)->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, (*glyph)->bitmap.buffer); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GLERROR
    gl::Texture::unbind();
  }

  static void set_active(int index=0) {
    glActiveTexture(GL_TEXTURE0 + index); GLERROR
  }

  static GLint get_active_texture() {
    GLint active_tex;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_tex); GLERROR
    return active_tex;
  }

  static void bind(GLuint texId) {
    glBindTexture(GL_TEXTURE_2D, texId); GLERROR
  }

  static void bind(gl::Texture &tx) {
    tx.bind();
  }

  void bind() {
    bind(tex);
  }

  void set_data(int index) {
    uSampler.set_data(index);
  }

  static void unbind() {
    glBindTexture(GL_TEXTURE_2D, 0); GLERROR
  }

  static void clear(GLuint &texId) {
    glDeleteTextures(1, &texId); GLERROR
  }

  static void clear(gl::Texture &tx) {
    tx.clear();
  }

  void clear() {
    clear(tex);
  }
};
}
