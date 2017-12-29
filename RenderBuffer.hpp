#pragma once

#include "Debug.hpp"
#include "Logger.hpp"
#include "Framebuffer.hpp"

namespace gl {
template <GLenum StorageT>
struct Renderbuffer {
  GLuint rbo = 0;

  Renderbuffer()
  {}

  auto id() const { return rbo; }

  void init(size_t width, size_t height) {
    glGenRenderbuffers(1, &rbo); GLERROR
    bind();

    glRenderbufferStorage(GL_RENDERBUFFER, StorageT, GL_RENDERBUFFER, rbo); GLERROR
  }

  void bind() {
    glBindRenderbuffer(GL_RENDERBUFFER, rbo); GLERROR
  }

  static void unbind() {
    glBindRenderbuffer(GL_RENDERBUFFER, 0); GLERROR
  }

  void clear() {
    glDeleteRenderbuffers(1, &rbo); GLERROR
  }
};
}
