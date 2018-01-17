#pragma once

#include "Button.hpp"
#include "Lobby.hpp"
#include "StrConst.hpp"

struct LobbyObject {
  LobbyActor *lobbyActor = nullptr;

  C_STRING(btn_texture, "assets/button.png");
  C_STRING(btn_font, "assets/Verdana.ttf");
  ui::Button<btn_texture, btn_font> exit_button;

  C_STRING(infobar_texture, "assets/grass.png");
  C_STRING(infobar_font, "assets/Verdana.ttf");
  ui::Button<infobar_texture, infobar_font> infobar;

  LobbyObject()
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
    infobar.init();
  }

  glm::vec2 cursorPosition = {0.f, 0.f};
  bool clicked = false;
  int click_button;
  int click_action;
  void mouse(float m_x, float m_y) {
    cursorPosition.x = m_x;
    cursorPosition.y = m_y;
  }
  void mouse_click(int button, int action) {
    clicked = true;
    click_button = button;
    click_action = action;
  }

  void display() {
    if(!is_active())return;
    exit_button.setx(.8, .1);
    exit_button.sety(-1, -.8);
    exit_button.label.set_text("EXIT");
    if(clicked && (exit_button.region.contains(cursorPosition) || exit_button.state != decltype(exit_button)::DEFAULT_STATE)) {
      exit_button.mouse_click(click_button, click_action);
      clicked = false;
    }
    exit_button.action_on_click([&]() mutable {
      lobbyActor->stop();
      unset_actor();
    });
    if(!is_active())return;
    exit_button.display();

    infobar.setx(.0, .8);
    infobar.sety(-1, -.8);
    infobar.region.ys += .05;
    for(auto &p : lobbyActor->lobby.players) {
      infobar.label.set_text(std::to_string(p.second.ind) + ": " + p.first.to_str());
      infobar.display();
    }
  }

  void clear() {
    exit_button.clear();
    infobar.clear();
  }
};
