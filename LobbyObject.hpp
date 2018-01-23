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
  ui::Button<infobarR_texture, btn_font> infobarR;
  ui::Button<infobarB_texture, btn_font> infobarB;

  LobbyObject()
  {}

  bool is_active() {
    return !(lobbyActor == nullptr || lobbyActor->has_quit() || lobbyActor->has_started());
  }

  void init() {
    exit_button.setx(-1, -.7);
    exit_button.sety(.9, 1);
    exit_button.init();
    exit_button.label.set_text("Exit");
    start_button.setx(.7, 1);
    start_button.sety(.9, 1);
    start_button.init();
    start_button.label.set_text("Start");
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

  template <typename ButtonT, typename F>
  void button_display(ButtonT &btn, F &&func) {
    btn.mouse(cursorPosition.x, cursorPosition.y);
    if(clicked && (btn.region.contains(cursorPosition) || btn.state != ButtonT::DEFAULT_STATE)) {
      btn.mouse_click(click_button, click_action);
      clicked = false;
    }
    btn.action_on_click(func);
    btn.display();
  }

  void display() {
    if(!is_active())return;
    button_display(exit_button, [&]() mutable {
      lobbyActor->action_leave();
    });
    if(!is_active())return;
    button_display(start_button, [&]() mutable {
      lobbyActor->action_start();
    });
    if(!is_active())return;

    glm::vec2 xs(-.8, .0);
    glm::vec2 ys(-1., -.9);
    ys += .05;
    lobbyActor->lobby.iterate([&](auto &p) mutable {
      if(p.second.team == Soccer::Team::RED_TEAM) {
        infobarR.setx(xs.x, xs.y);
        infobarR.sety(ys.x, ys.y);
        infobarR.label.set_text(std::to_string(p.second.ind) + ": " + p.first.to_str());
        button_display(infobarR, [&]() mutable {});
      } else {
        infobarB.setx(xs.x, xs.y);
        infobarB.sety(ys.x, ys.y);
        infobarB.label.set_text(std::to_string(p.second.ind) + ": " + p.first.to_str());
        button_display(infobarB, [&]() mutable {});
      }
      ys += .12;
      return true;
    });
  }

  void clear() {
    exit_button.clear();
    start_button.clear();
    infobarR.clear();
    infobarB.clear();
  }
};
