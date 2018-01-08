#pragma once

#include <string>
#include "incgraphics.h"

namespace img {
struct Image {
  size_t width=0, height=0, depth=1;
  GLenum format = GL_RGBA;
  unsigned char *data = NULL;
  std::string filename;

  Image(const char *filename):
    filename(filename)
  {}

  virtual ~Image() {
    width=height=depth=0;
    delete [] data;
    data = NULL;
  }
};
}
