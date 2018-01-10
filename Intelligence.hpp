#pragma once

#include <queue>
#include <thread>
#include <mutex>

#include "Soccer.hpp"
#include "Network.hpp"
#include "Logger.hpp"

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
  // listen/send action
  enum class Action { NO_ACTION,Z,X,C,V,F,S,M };
  struct action_struct {
    PKGType pkgtype = PKGType::ACTION;
    Action a;
    int id;
    float dir;
    glm::vec3 dest;
  };

  // send/listen to unit sync
  struct unit_sync_struct {
    unsigned char id; // -1 for ball
    unsigned char ball_owner;
    glm::vec3 pos;
    glm::vec2 dest;
    float movement_speed;
    float vertical_speed;
    float angle;
    float angle_dest;
    Timer::time_t frame;

    constexpr bool operator==(const unit_sync_struct &other) const {
      return frame == other.frame;
    }
    constexpr bool operator<(const unit_sync_struct &other) const {
      return frame < other.frame;
    }
  };
};

template <typename RemoteT> struct Remote;

template <>
struct Remote<RemoteType::Server> : public Intelligence {
  int id_;
  Soccer &soccer;
  net::Socket socket;
  std::thread listen_thread;
  std::thread sender_thread;
  std::mutex finalize_mtx;
  std::mutex socket_mtx;

  std::vector<net::Addr> clients;

  Remote(int id, Soccer &soccer, net::port_t port, std::vector<net::Addr> clients):
    id_(id), soccer(soccer), socket(port), clients(clients)
  {
    listen_thread = std::thread(
      Remote<RemoteType::Server>::listener_func,
      this
    );
    sender_thread = std::thread(
      Remote<RemoteType::Server>::sender_func,
      this
    );
  }

  static void listener_func(Remote<RemoteType::Server> *server) {
    while(!server->should_stop()) {
      net::Package<pkg::action_struct> package;
      package.data.a = pkg::Action::NO_ACTION;
      std::lock_guard<std::mutex> guard(server->socket_mtx);
      if(server->socket.receive(package)) {
        pkg::action_struct action = package.data;
        if(action.a == pkg::Action::NO_ACTION)continue;
        printf("Received action %d\n", action.a);
        std::lock_guard<std::mutex> guard(server->soccer.mtx);
        switch(action.a) {
          case pkg::Action::Z: server->soccer.z_action(action.id); break;
          case pkg::Action::X: server->soccer.x_action(action.id, action.dir); break;
          case pkg::Action::C: server->soccer.c_action(action.id, action.dest); break;
          case pkg::Action::V: server->soccer.v_action(action.id); break;
          case pkg::Action::F: server->soccer.f_action(action.id, action.dir); break;
          case pkg::Action::S: server->soccer.s_action(action.id); break;
          case pkg::Action::M: server->soccer.m_action(action.id, action.dest); break;
          case pkg::Action::NO_ACTION:break;
        }
      }
    }
  }

  static void sender_func(Remote<RemoteType::Server> *server) {
    while(!server->should_stop()) {
      usleep(1e6 / 5.);
      int no_ids = server->soccer.team1.size() + server->soccer.team2.size() + 1;
      pkg::unit_sync_struct data = server->get_unit_sync_data(rand() % no_ids);
      for(auto &addr : server->clients) {
        /* printf("sending sync data for unit %d\n", data.id); */
        net::Package<pkg::unit_sync_struct> package(addr, data);
        std::lock_guard<std::mutex> guard(server->socket_mtx);
        server->socket.send(package);
      }
    }
  }

  pkg::unit_sync_struct get_unit_sync_data(int unit_id) {
    pkg::unit_sync_struct usd;
    usd.id = unit_id;
    std::lock_guard<std::mutex> guard(soccer.mtx);
    usd.frame = soccer.timer.current_time;
    if(unit_id == Ball::NO_OWNER) {
      usd.vertical_speed = soccer.ball.vertical_speed;
    } else {
      usd.vertical_speed = soccer.get_player(unit_id).vertical_speed;
    }
    auto &unit = (unit_id == Ball::NO_OWNER) ? soccer.ball.unit : soccer.get_player(unit_id).unit;
    usd.ball_owner = soccer.ball.owner();
    usd.pos = unit.pos;
    usd.dest = unit.dest;
    usd.movement_speed = unit.moving_speed;
    usd.angle = unit.facing;
    usd.angle_dest = unit.facing_dest;
    return usd;
  }

  ~Remote() {
    {
      std::lock_guard<std::mutex> guard(finalize_mtx);
      finished = true;
    }
    listen_thread.join();
    sender_thread.join();
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
    std::lock_guard<std::mutex> guard(soccer.mtx);
    soccer.z_action(id_);
  }

  void x_action(float dir) {
    std::lock_guard<std::mutex> guard(soccer.mtx);
    soccer.x_action(id_, dir);
  }

  void c_action(glm::vec3 dest) {
    std::lock_guard<std::mutex> guard(soccer.mtx);
    soccer.c_action(id_, dest);
  }

  void v_action() {
    std::lock_guard<std::mutex> guard(soccer.mtx);
    soccer.v_action(id_);
  }

  void f_action(float dir) {
    std::lock_guard<std::mutex> guard(soccer.mtx);
    soccer.f_action(id_, dir);
  }

  void s_action() {
    std::lock_guard<std::mutex> guard(soccer.mtx);
    soccer.s_action(id_);
  }

  void m_action(glm::vec3 dest) {
    std::lock_guard<std::mutex> guard(soccer.mtx);
    soccer.m_action(id_, dest);
  }
};

template <>
struct Remote<RemoteType::Client> : public Intelligence {
  int id_;
  Soccer &soccer;
  net::Addr server_addr;
  net::Socket socket;
  std::thread listen_thread;
  std::mutex finalize_mtx;
  std::mutex socket_mtx;

  Remote(int id, Soccer &soccer, net::port_t client_port, net::Addr server_addr):
    id_(id),
    soccer(soccer),
    server_addr(server_addr),
    socket(client_port)
  {
    listen_thread = std::thread(
      Remote<RemoteType::Client>::listener_func,
      this
    );
  }

  static void listener_func(Remote<RemoteType::Client> *client) {
    Timer::time_t delay = 1.;
    while(!client->should_stop()) {
      net::Package<pkg::unit_sync_struct> packet;
      packet.data.id = 100;
      std::lock_guard<std::mutex> guard(client->socket_mtx);
      if(client->socket.receive(packet)) {
        pkg::unit_sync_struct unit_sync = packet.data;
        {
          std::lock_guard<std::mutex> guard(client->soccer.mtx);
          Timer::time_t current_time = client->soccer.timer.current_time;
          Timer::time_t next_time = current_time + 1./60;
          {
            if(!client->frame_patches.empty())printf("client: next frame %f, current time %f\n", client->frame_patches.top().frame, current_time);
            while(!client->frame_patches.empty() && client->frame_patches.top().frame + delay < current_time) {
              client->frame_patches.pop();
            }
            while(!client->frame_patches.empty() && client->frame_patches.top().frame + delay <= next_time) {
            /* while(!client->frame_patches.empty()) { */
              client->unpack_frame(client->frame_patches.top());
              client->frame_patches.pop();
            }
          }
        }
        if(unit_sync.id == 100)continue;
        /* printf("received data for unit %d and frame %f\n", unit_sync.id, unit_sync.frame); */
        client->frame_patches.push(unit_sync);
        {
          std::lock_guard<std::mutex> guard(client->soccer.mtx);
          Timer::time_t current_time = client->soccer.timer.current_time;
          delay = current_time - unit_sync.frame;
        }
      }
    }
  }

  std::priority_queue<pkg::unit_sync_struct> frame_patches;

  void unpack_frame(const pkg::unit_sync_struct &unit_sync) {
    int id = unit_sync.id;
    if(id == Ball::NO_OWNER) {
      soccer.ball.vertical_speed = unit_sync.vertical_speed;
    } else {
      soccer.get_player(id).vertical_speed = unit_sync.vertical_speed;
    }
    auto &unit = (id == Ball::NO_OWNER) ? soccer.ball.unit : soccer.get_player(id).unit;
    unit.pos = unit_sync.pos;
    unit.facing = unit_sync.angle;
    unit.facing_dest = unit_sync.angle_dest;
    unit.moving_speed = unit_sync.movement_speed;
    unit.dest = glm::vec3(unit_sync.dest, 0);

    /* soccer.timer.set_time(unit_sync.frame); */
    /* soccer.set_control_player(unit_sync.ball_owner); */
  }

  bool finalize = false;

  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  ~Remote() {
    {
      std::lock_guard<std::mutex> guard(finalize_mtx);
      finalize = true;
    }
    listen_thread.join();
  }

  template <typename T>
  void send_action(const T &data) {
    printf("Client: sending action %d\n", data.a);
    net::Package<T> packet(server_addr, data);
    std::lock_guard<std::mutex> guard(socket_mtx);
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
