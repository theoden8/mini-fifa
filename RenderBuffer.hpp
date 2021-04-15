#pragma once

#include "Debug.hpp"
#include "Logger.hpp"

#include "incgraphics.h"

namespace gl {
template <GLenum StorageT>
struct Renderbuffer {
  GLuint rbo = 0;

  Renderbuffer()
  {}

  auto id() const { return rbo; }

  static void init(GLuint &rbo) {
    glGenRenderbuffers(1, &rbo); GLERROR
    gl::Renderbuffer<StorageT>::bind(rbo);

    glRenderbufferStorage(GL_RENDERBUFFER, StorageT, GL_RENDERBUFFER, rbo); GLERROR
  }

  static void init(gl::Renderbuffer<StorageT> &rb, size_t width, size_t height) {
    rb.init(width, height);
  }

  void init(size_t width, size_t height) {
    gl::Renderbuffer<StorageT>::init(rbo, width, height);
  }

  static void bind(GLuint rbo) {
    glBindRenderbuffer(GL_RENDERBUFFER, rbo); GLERROR
  }

  static void bind(gl::Renderbuffer<StorageT> &rb) {
    rb.bind();
  }

  void bind() {
    gl::Renderbuffer<StorageT>::bind(rbo);
  }

  static void unbind() {
    glBindRenderbuffer(GL_RENDERBUFFER, 0); GLERROR
  }

  static void clear(GLuint &rbo) {
    glDeleteRenderbuffers(1, &rbo); GLERROR
  }

  static void clear(gl::Renderbuffer<StorageT> &rbo) {
    rbo.clear();
  }

  void clear() {
    clear(rbo);
  }
};
}
