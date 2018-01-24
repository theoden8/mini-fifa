#pragma once

#include "GameObject.hpp"
#include "LobbyObject.hpp"
#include "MetaServerObject.hpp"
#include "CursorObject.hpp"
#include "Client.hpp"

struct ClientObject {
  CursorObject cursor;
  Client &client;

  GameObject *gObject = nullptr;
  LobbyObject lObject;
  MetaServerObject mObject;

  ClientObject(Client &client):
    client(client),
    mObject(client.mclient),
    lObject()
  {}

  void init() {
    Logger::Info("cobject: initialized\n");
    mObject.init();
    lObject.init();
    gObject = nullptr; // need to set/unset actors!
    cursor.init();
  }

  void keypress(int key, int mods) {
    if(gObject != nullptr) {
      gObject->keypress(key, mods);
    }
  }

  bool is_active() {
    return client.state == Client::State::DEFAULT;
  }

  void mouse(float m_x, float m_y) {
    if(mObject.is_active()) {
      mObject.mouse(m_x, m_y);
    }
    if(lObject.is_active()) {
      lObject.mouse(m_x, m_y);
    }
    if(gObject != nullptr) {
      gObject->mouse(m_x, m_y);
    }
    cursor.mouse(m_x, m_y);
  }

  void mouse_click(int button, int action) {
    if(mObject.is_active()) {
      mObject.mouse_click(button, action);
    } else if(lObject.is_active()) {
      lObject.mouse_click(button, action);
    } else if(gObject != nullptr) {
      gObject->mouse_click(button, action);
    }
  }

  void update_states() {
    // perform actions
    if(client.mclient.has_quit()) {
      client.action_quit();
    }
    if(client.is_active_mclient() && !client.is_active_lobby() && !client.is_active_game()) {
      if(client.mclient.has_hosted()) {
        client.action_host_game();
      } else if(client.mclient.has_joined()) {
        client.action_join_game();
      }
    } else if(client.is_active_lobby()) {
      if(client.l_actor->has_quit()) {
        client.action_quit_lobby();
      } else if(client.l_actor->has_started()) {
        client.action_start_game();
      }
    }
    if(client.is_active_game() && client.intelligence->has_quit()) {
      client.action_quit_game();
    }
    // set lObject
    lObject.lobbyActor = client.l_actor;
    // set gObject
    if(gObject == nullptr && client.is_active_game()) {
      gObject = new GameObject(*client.soccer, *client.intelligence, cursor);
      gObject->init();
    } else if(gObject != nullptr && !client.is_active_game()) {
      gObject->clear();
      delete gObject;
      gObject = nullptr;
    }
    ASSERT(!(lObject.is_active() && gObject != nullptr));
  }

  void display(GLFWwindow *window, size_t width, size_t height) {
    update_states();
    if(mObject.is_active()) {
      mObject.display();
    } else if(lObject.is_active()) {
      lObject.display();
    } else if(client.is_active_game()) {
      gObject->set_winsize(width, height);
      gObject->keyboard(window);
      gObject->idle();
      gObject->current_time += 1./60;
      gObject->display();
    }
    cursor.display();
  }

  void clear() {
    Logger::Info("cobject: clearance\n");
    mObject.clear();
    lObject.clear();
    if(gObject != nullptr) {
      gObject->clear();
      delete gObject;
      gObject = nullptr;
    }
    cursor.display();
  }
};
