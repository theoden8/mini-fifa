#pragma once

#include "Debug.hpp"
#include "Logger.hpp"
#include "File.hpp"

#ifdef COMPILE_IMGPNG
  #include "PNGImage.hpp"
#endif
#ifdef COMPILE_IMGJPEG
  #include "JPEGImage.hpp"
#endif
#ifdef COMPILE_IMGTIFF
  #include "TIFFImage.hpp"
#endif
#include "BMPImage.hpp"
#include "TGAImage.hpp"

namespace img {

Image *load_image(File &file) {
  Logger::Info("Loading image file '%s'\n", file.name().c_str());
  Image *image = nullptr;
  if(!file.exists()) {
    TERMINATE("file '%s' not found", file.name().c_str());
  } else if(file.is_ext(".png")) {
// png
#ifdef COMPILE_IMGPNG
    image = new img::PNGImage(file.name().c_str());
#else
    Logger::Error("unable to open file %s: compiled without PNG support\n", file.name().c_str());
#endif
  } else if(file.is_ext(".jpg") || file.is_ext(".jpeg")) {
// jpeg
#ifdef COMPILE_IMGJPEG
    image = new img::JPEGImage(file.name().c_str());
#else
    Logger::Error("unable to open file %s: compiled without JPEG support\n", file.name().c_str());
#endif
  } else if(file.is_ext(".tiff")) {
// tiff
#ifdef COMPILE_IMGTIFF
    image = new img::TIFFImage(file.name().c_str());
#else
    Logger::Error("unable to open file %s: compiled without TIFF support\n", file.name().c_str());
#endif
  } else if(file.is_ext(".bmp")) {
// bmp
    image = new img::BMPImage(file.name().c_str());
  } else if(file.is_ext(".tga")) {
// tga
    image = new img::TGAImage(file.name().c_str());
  }
  if(image == nullptr) {
    TERMINATE("unsupported file format for %s\n", file.name().c_str());
  }
  return image;
}

}
