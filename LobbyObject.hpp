#pragma once

#include "Button.hpp"
#include "Lobby.hpp"
#include "StrConst.hpp"

struct LobbyObject {
  LobbyActor *lobbyActor = nullptr;

  C_STRING(btn_texture, "assets/button.png");
  C_STRING(btn_font, "assets/Verdana.ttf");
  ui::Button<btn_texture, btn_font> exit_button;
  ui::Button<btn_texture, btn_font> start_button;

  C_STRING(infobarR_texture, "assets/infobar_red.png");
  C_STRING(infobarB_texture, "assets/infobar_blue.png");
  C_STRING(infobar_font, "assets/Verdana.ttf");
  ui::Button<infobarR_texture, infobar_font> infobarR;
  ui::Button<infobarB_texture, infobar_font> infobarB;

  LobbyObject()
  {}

  bool is_active() {
    return !(lobbyActor == nullptr || lobbyActor->has_quit() || lobbyActor->has_started());
  }

  void init() {
    exit_button.init();
    start_button.init();
    infobarR.init();
    infobarB.init();
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

  template <typename F>
  void exit_button_update(F &&func) {
    exit_button.setx(-1., -.8);
    exit_button.sety(.9, .1);
    exit_button.label.set_text("Exit");
    exit_button.mouse(cursorPosition.x, cursorPosition.y);
    if(clicked && (exit_button.region.contains(cursorPosition) || exit_button.state != decltype(exit_button)::DEFAULT_STATE)) {
      exit_button.mouse_click(click_button, click_action);
      clicked = false;
    }
    exit_button.action_on_click(func);
  }

  template <typename F>
  void start_button_update(F &&func) {
    start_button.setx(-.2, .0);
    start_button.sety(.9, .1);
    start_button.label.set_text("Start");
    start_button.mouse(cursorPosition.x, cursorPosition.y);
    if(clicked && (start_button.region.contains(cursorPosition) || start_button.state != decltype(start_button)::DEFAULT_STATE)) {
      start_button.mouse_click(click_button, click_action);
      clicked = false;
    }
    start_button.action_on_click(func);
  }

  void update() {
    if(!is_active())return;
    exit_button_update([&]() mutable {
      lobbyActor->action_leave();
    });
    start_button_update([&]() mutable {
      lobbyActor->action_start();
    });
  }

  void display() {
    if(!is_active())return;
    update();
    if(!is_active())return;
    exit_button.display();
    start_button.display();

    glm::vec2 xs(-.8, .0);
    glm::vec2 ys(-1., -.8);
    ys += .05;
    std::lock_guard<std::mutex> guard(lobbyActor->lobby.mtx);
    for(auto &p : lobbyActor->lobby.players) {
      if(p.second.team == Soccer::Team::RED_TEAM) {
        infobarR.setx(xs.x, xs.y);
        infobarR.sety(ys.x, ys.y);
        infobarR.label.set_text(std::to_string(p.second.ind) + ": " + p.first.to_str());
        infobarR.display();
      } else {
        infobarB.setx(xs.x, xs.y);
        infobarB.sety(ys.x, ys.y);
        infobarB.label.set_text(std::to_string(p.second.ind) + ": " + p.first.to_str());
        infobarB.display();
      }
    }
  }

  void clear() {
    exit_button.clear();
    start_button.clear();
    infobarR.clear();
    infobarB.clear();
  }
};
