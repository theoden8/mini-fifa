#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"

struct Post {
  glm::mat4 matrix;
  glm::mat4 extra_rotate;
  Transformation transform;

  Model postModel;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;

  bool team;
  Post(bool team):
    team(team),
    uTransform("transform"),
    postModel("assets/woodswing/Models and Textures/woodswing.obj"),
    program({"post.vert", "post.frag"})
  {
    transform.SetScale(.033);
    transform.SetPosition(team ? -1.82 : 1.82, .0, 0);
    transform.rotation = extra_rotate =
      glm::rotate(glm::radians(90.f), glm::vec3(0, 0, 1))
      * glm::rotate(glm::radians(90.f), glm::vec3(1, 0, 0));
    transform.Rotate(0, 0, 1, team ? -90.f : 90.f);
  }

  void init() {
    program.compile_program();
    postModel.init();
    uTransform.set_id(program.id());
  }

  void display(Camera &cam) {
    program.use();

    if(cam.has_changed || transform.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    postModel.display(program);

    decltype(program)::unuse();
  }

  void clear() {
    postModel.clear();
    program.clear();
  }
};
