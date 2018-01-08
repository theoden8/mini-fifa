#pragma once

#include "Image.hpp"
#include "Debug.hpp"
#include "Logger.hpp"

#include <tiffio.h>

namespace img {
struct TIFFImage : public Image {
  TIFFImage(const char *filename):
    Image(filename)
  {
    init();
  }

  void init() {
    TIFFRGBAImage img;
    char error[1024];
    TIFF *tif = TIFFOpen(filename.c_str(), "r");
    ASSERT(tif != NULL);
    if(TIFFRGBAImageBegin(&img, tif, 0, error)) {
      size_t bpp = 4;
      format = GL_RGBA;
      width = img.width, height = img.height;
      data = new unsigned char[width * height * bpp];
      TIFFRGBAImageEnd(&img);
      int ret = TIFFRGBAImageGet(&img, (uint32_t *)data, width, height);
      if(ret == 0) {
        TIFFError("error: filename '%s', err=%d\n", filename.c_str(), error);
        exit(1);
      }
      Logger::Info("read tiff image %s (%d x %d)\n", filename.c_str(), img.width, img.height);
    } else {
      TIFFError("error: filename '%s', err=%d\n", filename.c_str(), error);
      exit(1);
    }
    TIFFClose(tif);
  }
};
}
