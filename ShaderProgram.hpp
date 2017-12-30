#pragma once

#include <cassert>
#include <string>
#include <vector>
#include <tuple>

#include "incgraphics.h"
#include "Tuple.hpp"
#include "Shader.hpp"
#include "ShaderAttrib.hpp"

namespace gl {
template <typename... ShaderTs>
class ShaderProgram {
  GLuint programId = 0;
  std::tuple<ShaderTs...> shaders;

public:
  static void compile_program(ShaderProgram<ShaderTs...> &program) {
    program.compile_program();
  }

  void compile_program() {
    Tuple::for_each(shaders, [&](auto &s) mutable {
      s.init();
    });
    programId = glCreateProgram(); GLERROR
    ASSERT(programId != 0);
    Tuple::for_each(shaders, [&](auto &s) mutable {
      glAttachShader(programId, s.id()); GLERROR
    });
    glLinkProgram(programId); GLERROR
    Tuple::for_each(shaders, [&](auto &s) mutable {
      s.clear();
    });
  }

  void bind_attrib(const std::vector <std::string> &locations) {
    for(size_t i = 0; i < locations.size(); ++i) {
      glBindAttribLocation(programId, i, locations[i].c_str()); GLERROR
    }
  }

  template <typename... STRINGs>
  ShaderProgram(STRINGs... shader_filenames):
    shaders(shader_filenames...)
  {}

  GLuint id() const {
    return programId;
  }

  static void init(ShaderProgram<ShaderTs...> &program, gl::VertexArray &vao, const std::vector<std::string> &&locations) {
    program.init(vao, locations);
  }

  void init(gl::VertexArray &vao, const std::vector <std::string> &&locations) {
    init(vao, locations);
  }

  void init(gl::VertexArray &vao, const std::vector <std::string> &locations) {
    vao.bind();
    compile_program();
    bind_attrib(locations);
    ASSERT(is_valid());
    gl::VertexArray::unbind();
  }

  static void use(GLuint progId) {
    glUseProgram(progId); GLERROR
  }

  static void use(ShaderProgram<ShaderTs...> &program) {
    program.use();
  }

  void use() {
    use(id());
  }

  static void unuse() {
    glUseProgram(0); GLERROR
  }

  static void clear(ShaderProgram<ShaderTs...> &program) {
    program.clear();
  }

  void clear() {
    Tuple::for_each(shaders, [&](const auto &s) {
      glDetachShader(programId, s.id()); GLERROR
    });
    glDeleteProgram(programId); GLERROR
  }

  bool is_valid() {
    glValidateProgram(programId);
    int params = -1;
    glGetProgramiv(programId, GL_VALIDATE_STATUS, &params);
    Logger::Info("programId %d GL_VALIDATE_STATUS = %d\n", programId, params);
    print_info_log();
    print_all();
    if (GL_TRUE != params) {
      return false;
    }
    return true;
  }

  static const char* GL_type_to_string(GLenum type) {
    switch(type) {
      case GL_BOOL: return "bool";
      case GL_INT: return "int";
      case GL_FLOAT: return "float";
      case GL_FLOAT_VEC2: return "vec2";
      case GL_FLOAT_VEC3: return "vec3";
      case GL_FLOAT_VEC4: return "vec4";
      case GL_FLOAT_MAT2: return "mat2";
      case GL_FLOAT_MAT3: return "mat3";
      case GL_FLOAT_MAT4: return "mat4";
      case GL_SAMPLER_2D: return "sampler2D";
      case GL_SAMPLER_3D: return "sampler3D";
      case GL_SAMPLER_CUBE: return "samplerCube";
      case GL_SAMPLER_2D_SHADOW: return "sampler2DShadow";
      default: break;
    }
    return "other";
  }

  void print_info_log() {
    int max_length = 2048;
    int actual_length = 0;
    char programId_log[2048];
    glGetProgramInfoLog(programId, max_length, &actual_length, programId_log); GLERROR
    Logger::Info("programId info log for GL index %u:\n%s", programId, programId_log);
  }

  void print_all() {
    Logger::Info("--------------------\n");
    Logger::Info("shader programId %d info:\n", programId);
    int params = -1;
    glGetProgramiv(programId, GL_LINK_STATUS, &params);
    Logger::Info("GL_LINK_STATUS = %d\n", params);

    glGetProgramiv(programId, GL_ATTACHED_SHADERS, &params);
    Logger::Info("GL_ATTACHED_SHADERS = %d\n", params);

    glGetProgramiv(programId, GL_ACTIVE_ATTRIBUTES, &params);
    Logger::Info("GL_ACTIVE_ATTRIBUTES = %d\n", params);
    for (int i = 0; i < params; i++) {
      char name[64];
      int max_length = 64;
      int actual_length = 0;
      int size = 0;
      GLenum type;
      glGetActiveAttrib (
        programId,
        i,
        max_length,
        &actual_length,
        &size,
        &type,
        name
      );
      if(size > 1) {
        for(int j = 0; j < size; j++) {
          char long_name[64];
          sprintf(long_name, "%s[%d]", name, j);
          int location = glGetAttribLocation(programId, long_name);
          Logger::Info("  %d) type:%s name:%s location:%d\n",
                 i, GL_type_to_string(type), long_name, location);
        }
      } else {
        int location = glGetAttribLocation(programId, name);
        Logger::Info("  %d) type:%s name:%s location:%d\n",
               i, GL_type_to_string(type), name, location);
      }
    }

    glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &params);
    Logger::Info("GL_ACTIVE_UNIFORMS = %d\n", params);
    for(int i = 0; i < params; i++) {
      char name[64];
      int max_length = 64;
      int actual_length = 0;
      int size = 0;
      GLenum type;
      glGetActiveUniform(
      programId,
      i,
      max_length,
      &actual_length,
      &size,
      &type,
      name
      );
      if(size > 1) {
      for(int j = 0; j < size; j++) {
        char long_name[64];
        sprintf(long_name, "%s[%d]", name, j);
        int location = glGetUniformLocation(programId, long_name);
        Logger::Info("  %d) type:%s name:%s location:%d\n",
           i, GL_type_to_string(type), long_name, location);
      }
      } else {
      int location = glGetUniformLocation(programId, name);
      Logger::Info("  %d) type:%s name:%s location:%d\n",
           i, GL_type_to_string(type), name, location);
      }
    }
  }
};
}
