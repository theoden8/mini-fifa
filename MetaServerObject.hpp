#pragma once

#include "Transformation.hpp"
#include "ShaderUniform.hpp"
#include "ShaderProgram.hpp"
#include "MetaServer.hpp"
#include "Button.hpp"
#include "StrConst.hpp"

struct MetaServerObject {
  MetaServerClient client;

  C_STRING(font_name, "assets/Verdana.ttf");
  C_STRING(texture_name, "assets/grass.png");
  Button<texture_name, font_name> button;

  template <typename... Ts>
  MetaServerObject(Ts... args):
    client(args...)
  {}

  bool is_active() {
    return client.should_stop();
  }

  void init() {
    button.init();
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
    glm::vec2 init_pos(button.region.x1(), button.region.y1());
    button_display("HOST", [&]() mutable {
      client.action_host("gamename");
    });
    button.region.ys += button.label.height();
    for(auto &game : client.gamelist.games) {
      button_display(game.second.c_str(), [&]() {
        // join the game
      });
      button.region.ys += button.label.height();
    }
    button.region.xs -= button.region.x1() - init_pos.x;
    button.region.ys -= button.region.y1() - init_pos.y;
  }

  void clear() {
    button.clear();
  }
};
