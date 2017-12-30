#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Shader.hpp"
#include "ShaderProgram.hpp"
#include "ShaderUniform.hpp"
#include "ShaderAttrib.hpp"

struct Shadow {
  glm::mat4 matrix;
  Transformation transform;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  gl::VertexArray vao;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrVertex;

  using ShaderAttrib = decltype(attrVertex);
  using ShaderProgram = decltype(program);

  Shadow():
    program({"shaders/shadow.vert", "shaders/shadow.frag"}),
    uTransform("transform"),
    attrVertex("vertex")
  {}

  void init() {
    ShaderAttrib::init(attrVertex);
    attrVertex.allocate<GL_STREAM_DRAW>(6, std::vector<float>{
      -1,-1, 1,-1, 1,1,
      1,1, -1,1, -1,-1,
    });

    gl::VertexArray::init(vao);
    gl::VertexArray::bind(vao);
    vao.enable(attrVertex);
    vao.set_access(attrVertex, 0, 0);
    gl::VertexArray::unbind();
    ShaderProgram::init(program, vao, {"attrVertex"});
    uTransform.set_id(program.id());
  }

  void display(Camera &cam) {
    glEnable(GL_BLEND); GLERROR
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLERROR
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); GLERROR

    ShaderProgram::use(program);

    if(cam.has_changed || transform.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    gl::VertexArray::bind(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    gl::VertexArray::unbind();

    ShaderProgram::unuse();

    glDisable(GL_BLEND); GLERROR
  }

  void clear() {
    ShaderAttrib::clear(attrVertex);
    gl::VertexArray::clear(vao);
    ShaderProgram::clear(program);
  }
};
