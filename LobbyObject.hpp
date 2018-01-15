#pragma once

#include "Button.hpp"
#include "Lobby.hpp"
#include "StrConst.hpp"

struct LobbyObject {
  LobbyActor *lobbyActor = nullptr;

  C_STRING(btn_texture, "assets/button.png");
  C_STRING(btn_font, "assets/Verdana.ttf");
  Button<btn_texture, btn_font> exit_button;

  LobbyObject():
    exit_button(Region(glm::vec2(.8, 1.), glm::vec2(-1, -.8)))
  {}

  void set_actor(LobbyActor &actor) {
    lobbyActor = &actor;
  }

  bool is_active() {
    return lobbyActor != nullptr;
  }

  void unset_actor() {
    lobbyActor = nullptr;
  }

  void init() {
    exit_button.init();
  }

  void display() {
    if(!is_active())return;
    exit_button.display();
  }

  void clear() {
    exit_button.clear();
  }
};
