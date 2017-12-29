#pragma once

#include "incgraphics.h"
#include "Debug.hpp"
#include "Logger.hpp"

#include "ShaderProgram.hpp"

namespace gl {
struct Pipeline {
  GLuint pipeline;

  Pipeline()
  {}

  auto id() const { return pipeline; }

  void init() {
    glGenProgramPipelines(1, &pipeline); GLERROR
  }

  void bind() {
    glBindProgramPipeline(pipeline); GLERROR
  }

  template <typename... ShaderTs>
  void use(gl::ShaderProgram<ShaderTs...> &program) {
    glUseProgram(program.id()); GLERROR
  }

  void clear() {
    glDeleteProgramPipelines(1, &pipeline); GLERROR
  }
};
}
