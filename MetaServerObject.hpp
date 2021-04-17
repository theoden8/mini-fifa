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

  C_STRING(hosttex_name, "assets/button.png");
  C_STRING(exittex_name, "assets/button.png");
  ui::Button<exittex_name, font_name> host_button;
  ui::Button<hosttex_name, font_name> exit_button;

  C_STRING(texture_name, "assets/button.png");
  ui::Button<texture_name, font_name> button;

  MetaServerObject(MetaServerClient &mclient, const std::string &dir):
    mclient(mclient),
    host_button(dir),
    exit_button(dir),
    button(dir)
  {}

  bool is_active() {
    return !mclient.should_stop() && !mclient.has_hosted() && !mclient.has_joined() && !mclient.has_quit();
  }

  void init() {
    button.init();
    exit_button.setx(-1, -.7);
    exit_button.sety(.9, 1);
    exit_button.init();
    exit_button.label.set_text("Exit");
    host_button.setx(.7, 1);
    host_button.sety(.9, 1);
    host_button.init();
    host_button.label.set_text("Host");
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
    button_display(host_button, [&]() mutable {
      mclient.action_host("the game");
    });
    button_display(exit_button, [&]() mutable {
      mclient.set_state(MetaServerClient::State::QUIT);
    });
    GameList glist;
    {
      // remove duplicates from multiple metaservers
      std::lock_guard<std::recursive_mutex> guard(mclient.mservers_mtx);
      for(auto &m : mclient.metaservers) {
        for(auto &game : mclient.gamelists[m].games) {
          auto &host = game.first;
          if(glist.games.find(host) == std::end(glist.games)) {
            glist.games[host] = game.second;
          }
        }
      }
    }
    button.setx(-.9, .9);
    button.sety(-1, -1+.1);
    for(auto &g : glist.games) {
      auto &host = g.first;
      auto &name = g.second;
      button.label.set_text(host.to_str() + " : " + name);
      button_display(button, [&]() mutable {
        mclient.action_join(g.first);
      });
      button.region.ys += .1;
    }
  }

  void clear() {
    button.clear();
    exit_button.clear();
    host_button.clear();
  }
};
