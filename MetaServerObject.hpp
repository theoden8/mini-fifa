#pragma once

#include "Transformation.hpp"
#include "ShaderUniform.hpp"
#include "ShaderProgram.hpp"
#include "MetaServer.hpp"
#include "Button.hpp"
#include "StrConst.hpp"

struct MetaServerObject {
  MetaServerClient &mclient;

  C_STRING(font_name, "assets/Verdana.ttf");
  C_STRING(texture_name, "assets/button.png");
  Button<texture_name, font_name> button;

  MetaServerObject(MetaServerClient &mclient):
    mclient(mclient)
  {}

  bool is_active() {
    return !mclient.should_stop();
  }

  void init() {
    button.init();
    mclient.start();
  }

  void mouse(float m_x, float m_y) {
    cursorPosition.x = m_x;
    cursorPosition.y = m_y;
  }

  glm::vec2 cursorPosition = {0.f, 0.f};
  bool clicked = true;
  int click_button;
  int click_action;
  void mouse_click(int button, int action) {
    clicked = true;
    click_button = button;
    click_action = action;
  }

  template <typename F>
  void button_display(const char *button_name, F func) {
    button.label.set_text(button_name);
    button.mouse(cursorPosition.x, cursorPosition.y);
    if(clicked && (button.region.contains(cursorPosition) || button.state != decltype(button)::DEFAULT_STATE)) {
      button.mouse_click(click_button, click_action);
      clicked = false;
    }
    button.action_on_click(func);
    button.display();
  }

  void display() {
    if(!is_active())return;
    button.setx(-1, -.7);
    button.sety(-1, -1+.1);
    glm::vec2 init_pos(button.region.x1(), button.region.y1());
    button_display("HOST", [&]() mutable {
      mclient.action_host("the game");
    });
    button.region.ys += button.label.height() * 2;
    /* for(auto &game : mclient.gamelist.games) { */
    /*   button_display(game.first.to_str() + " : " + game.second.c_str(), [&]() { */
    /*     // join the game */
    /*   }); */
    /*   button.region.ys += button.label.height(); */
    /* } */
    button.region.xs -= button.region.x1() - init_pos.x;
    button.region.ys -= button.region.y1() - init_pos.y;
  }

  void clear() {
    button.clear();
  }
};
