#pragma once

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Shadow.hpp"
#include "Sprite.hpp"

#include "Player.hpp"

#include "File.hpp"

struct PlayerObject {
  glm::mat4 matrix;
  glm::mat4 extra_rotate;
  Transformation transform;

	Sprite<Model, class RedPlayer> *playerModelRed;
	Sprite<Model, class BluePlayer> *playerModelBlue;
  gl::Uniform<gl::UniformType::MAT4> uTransform;
  gl::ShaderProgram<
    gl::VertexShader,
    gl::FragmentShader
  > program;
  Shadow shadow;

  using ShaderProgram = decltype(program);

  Unit::loc_t initial_position;
  PlayerObject(std::pair<float, float> pos={0, 0}):
    initial_position(pos.first, pos.second, 0),
    uTransform("transform"),
    playerModelRed(Sprite<Model, class RedPlayer>::create("assets/ninja/ninja.3ds")),
    playerModelBlue(Sprite<Model, class BluePlayer>::create("assets/ninja/ninja.3ds")),
    program({"shaders/player.vert", "shaders/player.frag"})
  {
    transform.SetScale(.01);
    transform.SetPosition(initial_position.x, initial_position.y, initial_position.z);
    extra_rotate =
      glm::rotate(glm::radians(-90.f), glm::vec3(0, 0, 1))
      * glm::rotate(glm::radians(90.f), glm::vec3(1, 0, 0));
    transform.rotation = extra_rotate;
    shadow.transform.SetScale(.025);
  }

  void init() {
    program.compile_program();
    if(!playerModelRed->initialized) {
      HACK::swap_files("assets/ninja/nskinbl.jpg", "assets/ninja/nskinrd.jpg");
    }
    playerModelRed->init();
    if(!playerModelBlue->initialized) {
      HACK::swap_files("assets/ninja/nskinbl.jpg", "assets/ninja/nskinrd.jpg");
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
