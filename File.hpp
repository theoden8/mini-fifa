#pragma once

#include "Debug.hpp"

#include <string>
#include <cassert>
#include <unistd.h>
#include <sys/stat.h>

class File {
  std::string filename;
public:
  File(const char *filename):
    filename(filename)
  {}

  size_t length() {
    struct stat st;
    stat(filename.c_str(), &st);
    return st.st_size;
  }

  const std::string &name() const {
    return filename;
  }

  std::string name() {
    return filename;
  }

  bool is_ext(const std::string &&ext) {
    if(ext.length() > filename.length())
      return false;
    size_t f=filename.length(),e=ext.length();
    return filename.substr(f-e, e) == ext;
  }

  bool exists() {
    return access(filename.c_str(), F_OK) != -1;
  }

  char *load_text() {
    size_t size = length() + 1;
    char *text = (char *)malloc(size * sizeof(char)); ASSERT(text != NULL);

    FILE *file = fopen(filename.c_str(), "r"); ASSERT(file != NULL);

    char *t = text;
    while((*t = fgetc(file)) != EOF)
      ++t;
    *t = '\0';

    fclose(file);
    return text;
  }
};
