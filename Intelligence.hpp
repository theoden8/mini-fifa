#pragma once

#include <thread>
#include <mutex>

#include "Soccer.hpp"
#include "Network.hpp"

namespace RemoteType {
class Server;
class Client;
}

struct Intelligence {
  virtual void z_action() = 0;
  virtual void x_action(float) = 0;
  virtual void c_action(glm::vec3) = 0;
  virtual void v_action() = 0;
  virtual void f_action(float) = 0;
  virtual void s_action() = 0;
  virtual void m_action(glm::vec3) = 0;
};

namespace pkg {
  enum class PKGType {
    ACTION,
    SYNC
  };
  enum class Action { Z,X,C,V,F,S,M };
  struct action_struct {
    PKGType pkgtype = PKGType::ACTION;
    Action a;
    int id;
    float dir;
    glm::vec3 dest;
  };
};

template <typename RemoteT> struct Remote;

template <>
struct Remote<RemoteType::Server> : public Intelligence {
  int id_;
  Soccer &soccer;
  net::Socket socket;
  std::thread listen_thread;
  std::mutex soccer_mtx;
  std::mutex finalize_mtx;

  Remote(int id, Soccer &soccer, net::port_t port):
    id_(id), soccer(soccer), socket(port)
  {
    listen_thread = std::thread([&](auto &server) mutable {
      Remote<RemoteType::Server>::run(server);
    }, *this);
  }

  static void run(Remote<RemoteType::Server> &server) {
    while(!server.should_stop()) {
      net::Package<pkg::action_struct> package;
      if(server.socket.receive(package)) {
        pkg::action_struct action = package.data;
        std::lock_guard<std::mutex> guard(server.soccer_mtx);
        switch(action.a) {
          case pkg::Action::Z: server.soccer.z_action(action.id); break;
          case pkg::Action::X: server.soccer.x_action(action.id, action.dir); break;
          case pkg::Action::C: server.soccer.c_action(action.id, action.dest); break;
          case pkg::Action::V: server.soccer.v_action(action.id); break;
          case pkg::Action::F: server.soccer.f_action(action.id, action.dir); break;
          case pkg::Action::S: server.soccer.s_action(action.id); break;
          case pkg::Action::M: server.soccer.m_action(action.id, action.dest); break;
        }
      }
    }
  }

  ~Remote() {
    {
      std::lock_guard<std::mutex> guard(finalize_mtx);
      finished = true;
    }
    listen_thread.join();
  }

  int id() const {
    return id_;
  }

  bool finished = false;

  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finished;
  }

  void z_action() {
    std::lock_guard<std::mutex> guard(soccer_mtx);
    soccer.z_action(id_);
  }

  void x_action(float dir) {
    std::lock_guard<std::mutex> guard(soccer_mtx);
    soccer.x_action(id_, dir);
  }

  void c_action(glm::vec3 dest) {
    std::lock_guard<std::mutex> guard(soccer_mtx);
    soccer.c_action(id_, dest);
  }

  void v_action() {
    std::lock_guard<std::mutex> guard(soccer_mtx);
    soccer.v_action(id_);
  }

  void f_action(float dir) {
    std::lock_guard<std::mutex> guard(soccer_mtx);
    soccer.f_action(id_, dir);
  }

  void s_action() {
    std::lock_guard<std::mutex> guard(soccer_mtx);
    soccer.s_action(id_);
  }

  void m_action(glm::vec3 dest) {
    std::lock_guard<std::mutex> guard(soccer_mtx);
    soccer.m_action(id_, dest);
  }
};

template <>
struct Remote<RemoteType::Client> : public Intelligence {
  int id_;
  net::Addr server_addr;
  net::Socket socket;

  Remote(int id, net::Addr server_addr):
    id_(id),
    server_addr(server_addr),
    socket(server_addr.port)
  {}

  template <typename T>
  void send_action(const T &data) {
    net::Package<T> packet(server_addr, data);
    socket.send(packet);
  }

  void z_action() {
    pkg::action_struct d = { .a=pkg::Action::Z, .id=id_ };
    send_action(d);
  }

  void x_action(float dir) {
    pkg::action_struct d = { .a=pkg::Action::X, .id=id_, .dir=dir };
    send_action(d);
  }

  void c_action(glm::vec3 dest) {
    pkg::action_struct d = { .a=pkg::Action::C, .id=id_, .dest=dest };
    send_action(d);
  }

  void v_action() {
    pkg::action_struct d = { .a=pkg::Action::V, .id=id_ };
    send_action(d);
  }

  void f_action(float dir) {
    pkg::action_struct d = { .a=pkg::Action::F, .id=id_, .dir=dir };
    send_action(d);
  }

  void s_action() {
    pkg::action_struct d = { .a=pkg::Action::S, .id=id_ };
    send_action(d);
  }

  void m_action(glm::vec3 dest) {
    pkg::action_struct d = { .a=pkg::Action::M, .id=id_, .dest=dest };
    send_action(d);
  }
};
