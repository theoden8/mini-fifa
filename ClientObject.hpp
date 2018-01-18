#pragma once

#include "GameObject.hpp"
#include "LobbyObject.hpp"
#include "MetaServerObject.hpp"
#include "Cursor.hpp"
#include "Client.hpp"

struct ClientObject {
  Cursor cursor;
  Client client;

  GameObject *gObject = nullptr;
  LobbyObject lObject;
  MetaServerObject mObject;

  ClientObject(net::Addr metaserver):
    client(metaserver),
    mObject(client.mclient),
    lObject()
  {}

  void init() {
    mObject.init();
    lObject.init();
    gObject = nullptr; // need to set/unset actors!
    cursor.init();
  }

  void keypress(int key) {
    if(gObject != nullptr) {
      gObject->keypress(key);
    }
  }

  void mouse(float m_x, float m_y) {
    if(mObject.is_active()) {
      mObject.mouse(m_x, m_y);
    } else if(lObject.is_active()) {
      lObject.mouse(m_x, m_y);
    } else if(gObject != nullptr) {
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
    if(client.is_active_mclient()) {
      if(lObject.is_active()) {
        lObject.unset_actor();
      }
      if(gObject != nullptr) {
        gObject->clear();
        delete gObject;
      }
    } else if(client.is_active_lobby()) {
      if(!lObject.is_active()) {
        lObject.set_actor(*client.l_actor);
      }
      if(gObject != nullptr) {
        gObject->clear();
        delete gObject;
      }
    } else if(client.is_active_game()) {
      if(lObject.is_active()) {
        lObject.unset_actor();
      } else if(gObject == nullptr) {
        gObject = new GameObject(*client.soccer, *client.intelligence, cursor);
        gObject->init();
      }
    }
  }

  void display(GLFWwindow *window, size_t width, size_t height) {
    update_states();
    if(client.is_active_mclient()) {
      mObject.display();
    } else if(client.is_active_lobby()) {
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
    mObject.clear();
    lObject.clear();
    if(gObject) {
      gObject->clear();
      delete gObject;
    }
    cursor.display();
  }
};
