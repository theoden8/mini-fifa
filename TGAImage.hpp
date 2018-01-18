#pragma once

#include <fstream>

#include "Image.hpp"
#include "Debug.hpp"
#include "Logger.hpp"

// https://stackoverflow.com/a/20596072/4811285
namespace img {
struct TGAImage : public Image {
  TGAImage(const char *filename):
    Image(filename)
  {
    init();
  }

  void init() {
    FILE *file = fopen(filename.c_str(), "r");
    if(file == nullptr) {
      TERMINATE("bmp: unable to open file '%s'\n", filename.c_str());
    }

    uint8_t header[18] = {0};
    constexpr static uint8_t is_decompressed_mask[12] = {0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    constexpr static uint8_t is_compressed_mask[12] = {0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

    fread(&header, 1, sizeof(header), file);

    bool is_compressed = false;

    if(!memcmp(is_decompressed_mask, &header, sizeof(is_decompressed_mask))) {
    // decompressed
      int bpp8 = header[16];
      int bpp = (bpp8 / 8);
      width  = header[13] * 256 + header[12];
      height = header[15] * 256 + header[14];
      size_t size  = width * height * bpp;

      if ((bpp8 != 24) && (bpp8 != 32)) {
        fclose(file);
        TERMINATE("tga: invalid file format: required 24 (RGB) or 32 (RGBA) bpp image\n");
      }

      is_compressed = false;
      data = new unsigned char[size];

      if(bpp == 4) {
        format = GL_RGBA;
      } else if(bpp == 3) {
        format = GL_RGB;
      } else if(bpp == 1) {
        format = GL_ALPHA8;
      }

      unsigned char *dpixel = data;
      unsigned char pixel[bpp];
      for(int i = 0; i < width * height; ++i) {
        fread(pixel, 1, bpp, file);

        if(bpp >= 3) {
          dpixel[0] = pixel[2];
          dpixel[1] = pixel[1];
          dpixel[2] = pixel[0];
        }
        if(bpp == 4) {
          dpixel[3] = pixel[3];
        }
        dpixel += bpp;
      }
    } else if(!memcmp(is_compressed_mask, &header, sizeof(is_compressed_mask))) {
    // compressed
      int bpp8 = header[16];
      int bpp = (bpp8 / 8);
      width  = header[13] * 256 + header[12];
      height = header[15] * 256 + header[14];
      size_t size  = ((width * bpp8 + 31) / 32) * 4 * height;

      if ((bpp8 != 24) && (bpp8 != 32)) {
        fclose(file);
        TERMINATE("tga: invalid file format: required 24 (RGB) or 32 (RGBA) bpp image\n");
      }

      is_compressed = true;
      data = new unsigned char[width * height * bpp];
      if(bpp == 4) {
        format = GL_RGBA;
      } else if(bpp == 3) {
        format = GL_RGB;
      } else if(bpp == 1) {
        format = GL_ALPHA8;
      }

      unsigned char *dpixel = data;

      while(dpixel < data + width * height*bpp) {
        uint8_t chunk_header;
        fread(&chunk_header, 1, sizeof(chunk_header), file);

        uint8_t pixel[bpp];
        if(chunk_header < 128) {
          ++chunk_header;
          for(int i = 0; i < chunk_header; ++i) {
            fread(pixel, 1, bpp, file);
            if(bpp >= 3) {
              dpixel[0] = pixel[2];
              dpixel[1] = pixel[1];
              dpixel[2] = pixel[0];
            }
            if(bpp == 4) {
              dpixel[3] = pixel[3];
            }
            dpixel += bpp;
          }
        } else {
          chunk_header -= 127;
          fread(pixel, 1, bpp, file);

          for(int i = 0; i < chunk_header; ++i) {
            if(bpp >= 3) {
              dpixel[0] = pixel[2];
              dpixel[1] = pixel[1];
              dpixel[2] = pixel[0];
            }
            if(bpp == 4) {
              dpixel[3] = pixel[3];
            }
            dpixel += bpp;
          }
        }
      };
    } else {
      TERMINATE("tga: invalid file format: required 24 (RGB) or 32 (RGBA) bpp image\n");
    }

    fclose(file);
  }
};
}
