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
  gl::Attrib<GL_ARRAY_BUFFER, gl::AttribType::VEC2> attrVertices;

  Shadow():
    program({"shadow.vert", "shadow.frag"}),
    uTransform("transform"),
    attrVertices("vertex")
  {}

  void init() {
    attrVertices.init();
    attrVertices.allocate<GL_STREAM_DRAW>(6, std::vector<float>{
      -1,-1, 1,-1, 1,1,
      1,1, -1,1, -1,-1,
    });

    vao.init();
    vao.bind();
    vao.enable(attrVertices);
    vao.set_access(attrVertices, 0, 0);
    gl::VertexArray::unbind();
    program.init(vao, {"attrVertices"});
    uTransform.set_id(program.id());
  }

  void display(Camera &cam) {
    glEnable(GL_BLEND); GLERROR
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GLERROR
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO); GLERROR

    program.use();

    if(cam.has_changed || transform.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6); GLERROR
    gl::VertexArray::unbind();

    decltype(program)::unuse();

    glDisable(GL_BLEND); GLERROR
  }

  void clear() {
    attrVertices.clear();
    vao.clear();
    program.clear();
  }
};
