#pragma once

#include <cstdarg>
#include <cstdlib>
#include <string>
#include <vector>
#include <mutex>

#include <File.hpp>
#include <Debug.hpp>

class Logger {
  std::vector<FILE *> files;

  static char *log_file;
  static FILE *log_file_ptr;
  std::mutex mtx;

  Logger()
  {}
  ~Logger() {
    for(FILE *fp : files) {
      fclose(fp);
    }
  }
  void AddOutputFilename(const char *filename) {
    #ifndef NDEBUG
      sys::File::truncate(filename);
      FILE *fp = fopen(filename, "w");
      AddOutputFile(fp);
    #endif
  }

  void AddOutputFile(FILE *fp) {
    #ifndef NDEBUG
      ASSERT(fp != nullptr);
      files.push_back(fp);
    #endif
  }

  void Write(const char *fmt, va_list args) {
    #ifndef NDEBUG
      std::lock_guard<std::mutex> guard(mtx);
      for(FILE *fp : files) {
        vfprintf(fp, fmt, args);
        fflush(fp);
      }
    #endif
  }
  void WriteFmt(const char *prefix, const char *fmt, va_list args) {
    #ifndef NDEBUG
      std::lock_guard<std::mutex> guard(mtx);
      for(FILE *fp : files) {
        if(fp == nullptr)continue;
        std::string s;
        s += prefix;
        s += fmt;
        va_list argptr;
        va_copy(argptr, args);
        vfprintf(fp, s.c_str(), argptr);
        fflush(fp);
      }
    #endif
  }
  static Logger *instance;
public:
  static void Setup() {
    if(instance == nullptr) {
      instance = new Logger();
    }
  }
  static void Say(const char *fmt, ...) {
    ASSERT(instance != nullptr);
    std::va_list argptr;
    va_start(argptr, fmt);
    instance->Write(fmt, argptr);
    va_end(argptr);
  }
  static void Info(const char *fmt, ...) {
    ASSERT(instance != nullptr);
    std::va_list argptr;
    va_start(argptr, fmt);
    instance->WriteFmt("INFO: ", fmt, argptr);
    va_end(argptr);
  }
  static void Warning(const char *fmt, ...) {
    ASSERT(instance != nullptr);
    std::va_list argptr;
    va_start(argptr, fmt);
    instance->WriteFmt("WARN: ", fmt, argptr);
    va_end(argptr);
  }
  static void Error(const char *fmt, ...) {
    ASSERT(instance != nullptr);
    std::va_list argptr;
    va_start(argptr, fmt);
    instance->WriteFmt("ERROR: ", fmt, argptr);
    va_end(argptr);
  }
  static void SetLogOutput(const std::string &filename) {
    ASSERT(instance != nullptr);
    instance->AddOutputFilename(filename.c_str());
    Logger::Info("added output file '%s'\n", filename.c_str());
    if(instance->files.size() == 1) {
      Logger::Info("Started log %s\n", filename.c_str());
    }
  }
  static void MirrorLog(FILE *redir) {
    ASSERT(instance != nullptr);
    instance->AddOutputFile(redir);
    Logger::Info("mirror log\n");
    if(instance->files.size() == 1) {
      Logger::Info("Started log\n");
    }
  }
  static void Close() {
    Logger::Info("Closing log\n");
    ASSERT(instance != nullptr);
    delete instance;
    instance = nullptr;
  }
};

char *Logger::log_file  = nullptr;
FILE *Logger::log_file_ptr  = nullptr;
Logger *Logger::instance = nullptr;
