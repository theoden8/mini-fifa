#pragma once

#include <string>
#include "incgraphics.h"

namespace img {
struct Image {
  enum class Format {
    ALPHA8,
    RGB,
    RGBA,
    BGR,
    BGRA
  };

  size_t width=0, height=0, depth=1;
  Format format = Format::RGBA;
  unsigned char *data = NULL;
  std::string filename;

  Image(const char *filename):
    filename(filename)
  {}

  constexpr auto bpp() {
    switch(format) {
      case Format::ALPHA8:
      return 1;
      case Format::RGB:
      case Format::BGR:
      return 3;
      case Format::RGBA:
      case Format::BGRA:
      return 4;
    }
  }

  virtual ~Image() {
    width=height=depth=0;
    delete [] data;
    data = NULL;
  }
};
}
