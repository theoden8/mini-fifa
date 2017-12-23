#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"

struct Player {
  glm::mat4 matrix;
  Transformation transform;

	Model playerModel;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;

  Player():
    uTransform("transform"),
    playerModel("assets/nanosuit/nanosuit.obj"),
    program({"player.vert", "player.frag"})
  {}

  void init() {
    program.compile_program();
    playerModel.init();
    uTransform.set_id(program.id());
  }

  void display(Camera &cam) {
    program.use();

    if(cam.has_changed || transform.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    playerModel.display(program);

    decltype(program)::unuse();
  }

  void clear() {
    playerModel.clear();
    program.clear();
  }
};
