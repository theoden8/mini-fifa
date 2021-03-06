#pragma once

#include "Debug.hpp"
#include "File.hpp"
#include "Logger.hpp"
#include "Image.hpp"

#include <jpeglib.h>
#include <jerror.h>

namespace img {
struct JPEGImage : public Image {
  JPEGImage(const char *filename):
    Image(filename)
  {
    init();
  }

  void init() {
    unsigned char *rowptr[1];    // pointer to an array
    struct jpeg_decompress_struct info; //for our jpeg info
    struct jpeg_error_mgr err;          //the error handler

    FILE *file = fopen(filename.c_str(), "rb");  //open the file
    if(file == nullptr) {
      TERMINATE("jpeg: unable to open file '%s'\n", filename.c_str());
    }

    sys::File::Lock fl(file);

    info.err = jpeg_std_error(& err);
    jpeg_create_decompress(& info);   //fills info structure

    jpeg_stdio_src(&info, file);
    jpeg_read_header(&info, TRUE);   // read jpeg file header

    jpeg_start_decompress(&info);    // decompress the file

    //set width and height
    size_t bpp = info.num_components;
    width = info.output_width;
    height = info.output_height;
    if(bpp == 3) {
      format = Image::Format::RGB;
    } else if(bpp == 4) {
      format = Image::Format::RGBA;
    } else {
      TERMINATE("jpeg: unknown pixel format\n");
    }

    data = new unsigned char[width * height * bpp];
    while(info.output_scanline < info.output_height) {
      // Enable jpeg_read_scanlines() to fill our jdata array
      rowptr[0] = (unsigned char *)data + /* secret to method */ bpp * width * (height - 1 - info.output_scanline);
      jpeg_read_scanlines(&info, rowptr, 1);
    }
    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);
    fl.drop();
    fclose(file);
  }
};
}
