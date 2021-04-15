#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Shadow.hpp"
#include "Sprite.hpp"
#include "StrConst.hpp"
#include "Player.hpp"

#include "File.hpp"

struct PlayerObject {
  glm::mat4 matrix;
  glm::mat4 extra_rotate;
  Transformation transform;

  class RedPlayer; class BluePlayer;

  Sprite<Model, RedPlayer> *playerModelRed;
  Sprite<Model, BluePlayer> *playerModelBlue;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  Shadow shadow;

  using ShaderProgram = decltype(program);

  PlayerObject():
    uTransform("transform"),
    playerModelRed(Sprite<Model, RedPlayer>::create("assets/ninja/ninja.3ds"s)),
    playerModelBlue(Sprite<Model, BluePlayer>::create("assets/ninja/ninja.3ds"s)),
    program({"shaders/player.vert"s, "shaders/player.frag"s})
  {
    transform.SetScale(.01, .006, .01);
    transform.SetPosition(0, 0, 0);
    extra_rotate =
      glm::rotate(glm::radians(-90.f), glm::vec3(0, 0, 1))
      * glm::rotate(glm::radians(90.f), glm::vec3(1, 0, 0));
    transform.rotation = extra_rotate;
    shadow.transform.SetScale(.025);
  }

  void init() {
    program.compile_program();
    if(!playerModelRed->initialized) {
      sys::HACK::swap_files("assets/ninja/nskinbl.jpg"s, "assets/ninja/nskinrd.jpg"s);
    }
    playerModelRed->init();
    if(!playerModelBlue->initialized) {
      sys::HACK::swap_files("assets/ninja/nskinbl.jpg"s, "assets/ninja/nskinrd.jpg"s);
    }
    playerModelBlue->init();
    uTransform.set_id(program.id());
    shadow.init();
  }

  void display(const Player &player, Camera &cam) {
    transform.rotation = extra_rotate;
    transform.Rotate(0, 0, 1, player.unit.facing / M_PI * 180.f);
    transform.SetPosition(player.unit.pos.x, player.unit.pos.y, player.unit.pos.z);

    shadow.transform.SetPosition(player.unit.pos.x, player.unit.pos.y, .001);
    shadow.display(cam);

    ShaderProgram::use(program);

    if(cam.has_changed || transform.has_changed) {
      matrix = cam.get_matrix() * transform.get_matrix();
      uTransform.set_data(matrix);

      transform.has_changed = false;
    }

    if(!player.team) {
      playerModelRed->display(program);
    } else {
      playerModelBlue->display(program);
    }

    ShaderProgram::unuse();
  }

  void clear() {
    playerModelRed->clear();
    playerModelBlue->clear();
    ShaderProgram::clear(program);
  }
};
