#pragma once

#include "Debug.hpp"
#include "Logger.hpp"

#include <string>
#include <cstdio>
#include <cassert>
#include <unistd.h>
#include <sys/stat.h>

namespace HACK {
  void rename_file(const char *a, const char *b) {
    int err = rename(a, b);
    if(err)perror( "Error renaming file");
  }

  static void swap_files(std::string a, std::string b) {
    ASSERT(a != b);
    const char *TMP = "dahfsjkgdhjsfgshjkgfdhjgfwfghjfhdgjsvfh";
    rename_file(a.c_str(), TMP);
    rename_file(b.c_str(), a.c_str());
    rename_file(TMP, b.c_str());
  }
}

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
    char *text = (char *)malloc(size * sizeof(char));
    if(text == nullptr) {
      TERMINATE("unable to allocate text\n");
    }

    FILE *file = fopen(filename.c_str(), "r");
    if(file == nullptr) {
      TERMINATE("unable to open file '%s' for reading\n", filename.c_str());
    }

    char *t = text;
    while((*t = fgetc(file)) != EOF)
      ++t;
    *t = '\0';

    fclose(file);
    return text;
  }
};
