#pragma once

#include <cstdarg>
#include <string>
#include <cstdlib>
#include <unistd.h>

#include "incaudio.h"
#include "incgraphics.h"
#include "Debug.hpp"

class Logger {
  std::string filename;
  FILE *file = nullptr;

  static char *log_file;
  static FILE *log_file_ptr;

  Logger(const char *filename):
    filename(filename)
  {
    #ifndef NDEBUG
      truncate(this->filename.c_str(), 0);
      file = fopen(this->filename.c_str(), "a");
      ASSERT(file != nullptr);
    #endif
  }
  ~Logger() {
    if(file != nullptr) {
      fclose(file);
    }
  }
  void Write(const char *fmt, va_list args) {
    #ifndef NDEBUG
      if(file == nullptr)return;
      ASSERT(file != nullptr);
      vfprintf(file, fmt, args);
      fflush(file);
    #endif
  }
  void WriteFmt(const char *prefix, const char *fmt, va_list args) {
    #ifndef NDEBUG
      if(file == nullptr)return;
      ASSERT(file != nullptr);
      vfprintf(file, (std::string() + prefix + fmt).c_str(), args);
    #endif
  }
  static Logger *instance;
public:
  static void Setup(const char *filename) {
    if(instance == nullptr) {
      instance = new Logger(filename);
    }
    Logger::Info("Started log %s\n", filename);
  }
  static void Say(const char *fmt, ...) {
    ASSERT(instance != nullptr);
    va_list argptr;
    va_start(argptr, fmt);
    instance->Write(fmt, argptr);
    va_end(argptr);
  }
  static void Info(const char *fmt, ...) {
    ASSERT(instance != nullptr);
    va_list argptr;
    va_start(argptr, fmt);
    instance->WriteFmt("INFO: ", fmt, argptr);
    va_end(argptr);
  }
  static void Warning(const char *fmt, ...) {
    ASSERT(instance != nullptr);
    va_list argptr;
    va_start(argptr, fmt);
    instance->WriteFmt("WARN: ", fmt, argptr);
    va_end(argptr);
  }
  static void Error(const char *fmt, ...) {
    ASSERT(instance != nullptr);
    va_list argptr;
    va_start(argptr, fmt);
    instance->WriteFmt("ERROR: ", fmt, argptr);
    va_end(argptr);
  }
  static void MirrorLog(FILE *redir) {
    ASSERT(instance != nullptr);
    if(instance->file == nullptr)
      return;
    dup2(fileno(redir), fileno(instance->file));
    if(errno) {
      perror("error");
      errno=0;
    }
  }
  static void Close() {
    Logger::Info("Closing log %s", instance->filename.c_str());
    ASSERT(instance != nullptr);
    delete instance;
    instance = nullptr;
  }
};

char *Logger::log_file  = nullptr;
FILE *Logger::log_file_ptr  = nullptr;
Logger *Logger::instance = nullptr;

void log_gl_params() {
#ifndef NDEBUG
  GLenum params[] = {
    GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,
    GL_MAX_CUBE_MAP_TEXTURE_SIZE,
    GL_MAX_DRAW_BUFFERS,
    GL_MAX_FRAGMENT_UNIFORM_COMPONENTS,
    GL_MAX_TEXTURE_IMAGE_UNITS,
    GL_MAX_TEXTURE_SIZE,
    GL_MAX_VARYING_FLOATS,
    GL_MAX_VERTEX_ATTRIBS,
    GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
    GL_MAX_VERTEX_UNIFORM_COMPONENTS,
    GL_MAX_VIEWPORT_DIMS,
    GL_STEREO,
  };
  const char* names[] = {
    "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS",
    "GL_MAX_CUBE_MAP_TEXTURE_SIZE",
    "GL_MAX_DRAW_BUFFERS",
    "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS",
    "GL_MAX_TEXTURE_IMAGE_UNITS",
    "GL_MAX_TEXTURE_SIZE",
    "GL_MAX_VARYING_FLOATS",
    "GL_MAX_VERTEX_ATTRIBS",
    "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS",
    "GL_MAX_VERTEX_UNIFORM_COMPONENTS",
    "GL_MAX_VIEWPORT_DIMS",
    "GL_STEREO",
  };
  Logger::Info("GL Context Params:\n");
  char msg[256];
  // integers - only works if the order is 0-10 integer return types
  for (int i = 0; i < 10; i++) {
    int v = 0;
    glGetIntegerv(params[i], &v);
    Logger::Info("%s %i\n", names[i], v);
  }
  // others
  int v[2];
  v[0] = v[1] = 0;
  glGetIntegerv(params[10], v);
  Logger::Info("%s %i %i\n", names[10], v[0], v[1]);
  unsigned char s = 0;
  glGetBooleanv(params[11], &s);
  Logger::Info("%s %u\n", names[11], (unsigned int)s);
  Logger::Info("-----------------------------\n");
#endif
}
