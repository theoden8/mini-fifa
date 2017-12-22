#pragma once

#include "incgraphics.h"

#include "File.hpp"
#include "Shader.hpp"
#include "Debug.hpp"
#include "Logger.hpp"

#include <string>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <sys/stat.h>

namespace gl {
enum class ShaderType {
  VERTEX, TESS_CNTRL, TESS_EVAL,
  GEOMETRY, FRAGMENT, COMPUTE
};

constexpr GLenum get_gl_shader_constant(ShaderType sT) {
  switch(sT) {
    case ShaderType::VERTEX: return GL_VERTEX_SHADER;
    case ShaderType::TESS_CNTRL: return GL_TESS_CONTROL_SHADER;
    case ShaderType::TESS_EVAL: return GL_TESS_EVALUATION_SHADER;
    case ShaderType::GEOMETRY: return GL_GEOMETRY_SHADER;
    case ShaderType::FRAGMENT: return GL_FRAGMENT_SHADER;
    case ShaderType::COMPUTE: return GL_COMPUTE_SHADER;
  }
}

template <ShaderType ShaderT>
struct Shader {
  File file;
  GLuint shaderId = 0;

  Shader(std::string filename):
    file(filename.c_str())
  {
    if(file.is_ext(".vert")) { ASSERT(ShaderT == ShaderType::VERTEX); } else
    if(file.is_ext(".tesc")) { ASSERT(ShaderT == ShaderType::TESS_CNTRL); } else
    if(file.is_ext(".tese")) { ASSERT(ShaderT == ShaderType::TESS_EVAL); } else
    if(file.is_ext(".geom")) { ASSERT(ShaderT == ShaderType::GEOMETRY); } else
    if(file.is_ext(".frag")) { ASSERT(ShaderT == ShaderType::FRAGMENT); } else
    if(file.is_ext(".comp")) { ASSERT(ShaderT == ShaderType::COMPUTE); } else {
      throw std::runtime_error("unknown shader type");
    }
  }
  ~Shader()
  {}
  int id() const {
    return shaderId;
  }
  void init() {
    shaderId = glCreateShader(get_gl_shader_constant(ShaderT)); GLERROR
    char *source_code = file.load_text();
    ASSERT(source_code != NULL);
    glShaderSource(shaderId, 1, &source_code, NULL); GLERROR
    glCompileShader(shaderId); GLERROR
    free(source_code);
  }
  template <typename = std::enable_if<ShaderT == ShaderType::COMPUTE>>
  void dispatch(size_t x, size_t y, size_t z) {
    glDispatchCompute(x,y,z); GLERROR
  }
  void clear() {
    glDeleteShader(shaderId); GLERROR
  }
};

using VertexShader = Shader<ShaderType::VERTEX>;
using FragmentShader = Shader<ShaderType::FRAGMENT>;
using ComputeShader = Shader<ShaderType::COMPUTE>;

}
