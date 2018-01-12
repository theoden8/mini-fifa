#pragma once

#include <stdexcept>
#include <iostream>
#include <string>

#include "incgraphics.h"
#include "incaudio.h"

#define STR(x) #x
#define TOSTR(x) STR(x)
#define ERROR(format) fprintf(stderr, "error: " format "\n");

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CODE_LOCATION "\033[1m" __FILE__ "\033[0m::\033[93m" TOSTRING(__LINE__) "\033[0m"
#define CONDITION_TOSTR(CONDITION) " ( \033[1;4m" STRINGIFY(CONDITION) "\033[0m )"

#define ASSERT(CONDITION) \
  if(!(CONDITION)) { \
    throw std::runtime_error("\033[1;91merror\033[0m at " CODE_LOCATION CONDITION_TOSTR(CONDITION)); \
  }
#define TERMINATE(...) \
  Logger::Error(__VA_ARGS__);  \
  throw std::runtime_error("\033[1;91mterminated\033[0m at " CODE_LOCATION);

#ifndef NDEBUG

#define GLERROR { GLenum ret = glGetError(); if(ret != GL_NO_ERROR) { explain_glerror(ret); std::cerr << ret << std::endl; ASSERT(ret == GL_NO_ERROR); } };
#define ALERROR { GLenum ret = alGetError(); if(ret != GL_NO_ERROR) { explain_alerror(ret); std::cerr << ret << std::endl; ASSERT(ret == GL_NO_ERROR); } };

#else

#define GLERROR
#define ALERROR

#endif

void explain_glerror(GLenum code) {
  std::string err;
  switch(code) {
    case GL_NO_ERROR:
      err = "no error";
    break;
    case GL_INVALID_ENUM:
      err = "invalid enum";
    break;
    case GL_INVALID_VALUE:
      err = "invalid value";
    break;
    case GL_INVALID_OPERATION:
      err = "invalid operation";
    break;
    case GL_STACK_OVERFLOW:
      err = "stack overflow";
    break;
    case GL_STACK_UNDERFLOW:
      err = "stack underflow";
    break;
    case GL_OUT_OF_MEMORY:
      err = "out of memory";
    break;
    case GL_TABLE_TOO_LARGE:
      err = "table too large";
    break;
    default:
      err = "unknown_error " + std::to_string(code);
    break;
  }
  std::cerr << err << std::endl;
}

void explain_alerror(ALenum code) {
  std::string err;
  switch(code) {
    case 40962: err = "invalid enum"; break;
    case 40963: err = "invalid format"; break;
  }
  std::cerr << "alerror: " << err << " (" << code << ")" << std::endl;
}
