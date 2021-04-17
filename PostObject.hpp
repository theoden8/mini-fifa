#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"

struct PostObject {
  glm::mat4 matrix;
  glm::mat4 extra_rotate;
  Transformation transform;

  class PostModel;
  Sprite<Model, PostModel> *postModelSprite;
  Model &postModel;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;

  using ShaderProgram = decltype(program);

  bool team;
  PostObject(bool team, const std::string &dir):
    team(team),
    uTransform("transform"),
    postModelSprite(Sprite<Model, PostModel>::create(sys::Path(dir) / sys::Path("assets"s) / sys::Path("woodswing/woodswing.obj"s))),
    postModel(postModelSprite->object),
    program({
      sys::Path(dir) / sys::Path("shaders"s) / sys::Path("post.vert"s),
      sys::Path(dir) / sys::Path("shaders"s) / sys::Path("post.frag"s)
    })
  {
    transform.SetScale(.033);
    transform.SetPosition(team ? -1.82 : 1.82, .0, 0);
    transform.rotation = extra_rotate =
      glm::rotate(glm::radians(90.f), glm::vec3(0, 0, 1))
      * glm::rotate(glm::radians(90.f), glm::vec3(1, 0, 0));
    transform.Rotate(0, 0, 1, team ? -90.f : 90.f);
  }

  void init() {
    ShaderProgram::compile_program(program);
    postModelSprite->init();
    uTransform.set_id(program.id());
  }

  void display(Camera &cam) {
    ShaderProgram::use(program);

    if(cam.has_changed || transform.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    postModel.display(program);

    ShaderProgram::unuse();
  }

  void clear() {
    postModelSprite->clear();
    program.clear();
  }
};
