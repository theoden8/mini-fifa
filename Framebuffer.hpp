#pragma once

#include "incgraphics.h"
#include "Debug.hpp"
#include "Logger.hpp"
#include "Texture.hpp"
#include "Renderbuffer.hpp"

namespace gl {
struct Framebuffer {
  GLuint fbo = 0;
  std::vector<GLuint> textures;

  Framebuffer()
  {}

  auto id() const {
    return fbo;
  }

  void init(size_t width, size_t height) {
    glGenFramebuffers(1, &fbo); GLERROR
  }

  void bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); GLERROR
  }

  static void unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0); GLERROR
  }

  template <GLenum Attachment>
  void attach_texture(size_t w, size_t h, int mipmap_level=0) {
    gl::Framebuffer::bind();

    GLuint tex = 0;

    glGenTextures(1, &tex); GLERROR
    glBindTexture(GL_TEXTURE_2D, tex); GLERROR

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GLERROR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); GLERROR

    glFramebufferTexture2D(GL_FRAMEBUFFER, Attachment, GL_TEXTURE_2D, tex, mipmap_level); GLERROR

    textures.push_back(tex);

    gl::Framebuffer::unbind();
  }

  template <GLenum AttachmentT, GLenum StorageT>
  void attach_renderbuffer(gl::Renderbuffer<StorageT> &rb) {
    bind();

    rb.bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, AttachmentT, GL_RENDERBUFFER, rb.id()); GLERROR
    gl::Renderbuffer<StorageT>::unbind();

    gl::Framebuffer::unbind();
  }

  void clear() {
    glDeleteTextures(textures.size(), textures.data()); GLERROR
    glDeleteFramebuffers(1, &fbo); GLERROR
  }
};
}
