#pragma once

#include <cstdint>

#include <queue>
#include <thread>
#include <mutex>
#include <chrono>

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
  virtual void idle(Timer::time_t) = 0;
};

namespace pkg {
  // listen/send action
  enum class Action { NO_ACTION,Z,X,C,V,F,S,M };
  struct action_struct {
    Action a;
    int id;
    float dir;
    glm::vec3 dest;
  };

  struct action_struct_response {
  };

  // send/listen to unit sync
  struct sync_struct {
    int8_t id; // -1 for ball
    int8_t ball_owner;
    glm::vec3 pos;
    glm::vec2 dest;
    float movement_speed;
    float vertical_speed;
    float angle;
    float angle_dest;

    Timer::time_t frame;

    int no_actions;
    action_struct action = { .a=Action::NO_ACTION };

    constexpr bool has_action() const {
      return action.a != Action::NO_ACTION;
    }

    constexpr bool operator==(const sync_struct &other) const {
      return frame == other.frame;
    }
    constexpr bool operator!=(const sync_struct &other) const {
      return frame != other.frame;
    }
    constexpr bool operator<(const sync_struct &other) const {
      return frame < other.frame;
    }
    constexpr bool operator>(const sync_struct &other) const {
      return frame > other.frame;
    }
  };
};

template <typename RemoteT> struct Remote : public Intelligence {};

template <>
struct Remote<RemoteType::Server> : public Intelligence {
  int id_;
  int no_actions = 0;
  Soccer &soccer;
  net::Socket socket;
  std::thread server_thread;
  std::mutex q_actions_mtx;
  std::mutex finalize_mtx;

  std::vector<net::Addr> clients;

  Remote(int id, Soccer &soccer, net::port_t port, std::vector<net::Addr> clients):
    id_(id), soccer(soccer), socket(port), clients(clients)
  {
    server_thread = std::thread(
      Remote<RemoteType::Server>::run,
      this
    );
  }

  static void run(Remote<RemoteType::Server> *server) {
    using sys_time_t = std::chrono::nanoseconds;
    Timer::time_t last_sent = Timer::time_start();
    const Timer::time_t send_diff = .1;
    auto server_time_start = std::chrono::system_clock::now();
    while(!server->should_stop()) {
      auto server_time_now = std::chrono::system_clock::now();
      Timer::time_t server_time = 1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(server_time_now - server_time_start).count();
      if(last_sent == 0 || server_time - last_sent > send_diff) {
        server->send_sync();
        last_sent = server_time;
      }
      net::Package<pkg::action_struct> package;
      package.data.a = pkg::Action::NO_ACTION;
      if(server->socket.receive(package)) {
        pkg::action_struct action = package.data;
        if(action.a == pkg::Action::NO_ACTION)continue;
        printf("Received action %d\n", action.a);
        // lock soccer
        {
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
        // lock no_actions
        {
          std::lock_guard<std::mutex> guard(server->q_actions_mtx);
          ++server->no_actions;
        }
        // lock soccer again
        pkg::sync_struct sync = server->get_sync_data(action.id);
        sync.action = action;
        for(const auto &addr : server->clients) {
          net::Package<pkg::sync_struct> package(addr, sync);
          server->socket.send(package);
        }
      }
    }
  }

  void send_sync() {
    int unit_id;
    {
      std::lock_guard<std::mutex> guard(soccer.mtx);
      int no_ids = soccer.team1.size() + soccer.team2.size() + 1;
      unit_id = (rand() % no_ids) - 1;
    }
    pkg::sync_struct data = get_sync_data(unit_id);
    for(const auto &addr : clients) {
      /* printf("sending sync data for unit %f\n", data.frame); */
      net::Package<pkg::sync_struct> package(addr, data);
      socket.send(package);
    }
  }

  pkg::sync_struct get_sync_data(int unit_id=Ball::NO_OWNER) {
    pkg::sync_struct usd;
    std::lock_guard<std::mutex> guard(soccer.mtx);
    usd.id = unit_id;
    usd.frame = soccer.timer.current_time;
    if(unit_id == Ball::NO_OWNER) {
      usd.vertical_speed = soccer.ball.vertical_speed;
    } else {
      usd.vertical_speed = soccer.get_player(unit_id).vertical_speed;
    }
    auto &unit = soccer.get_unit(unit_id);
    usd.ball_owner = soccer.ball.owner();
    usd.pos = unit.pos;
    usd.dest = unit.dest;
    usd.movement_speed = unit.moving_speed;
    usd.angle = unit.facing;
    usd.angle_dest = unit.facing_dest;
    {
      std::lock_guard<std::mutex> guard(q_actions_mtx);
      usd.no_actions = no_actions;
    }
    return usd;
  }

  void idle(Timer::time_t curtime) {
    soccer.idle(curtime);
  }

  ~Remote() {
    stop();
    server_thread.join();
  }

  int id() const {
    return id_;
  }

  bool finalize = false;
  void stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    finalize = true;
  }
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
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
  int no_actions = 0;
  Timer::time_t last_frame = Timer::time_start();
  std::thread listen_thread;
  std::mutex frame_schedule_mtx;
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
      net::Package<pkg::sync_struct> packet;
      packet.data.no_actions = -1;
      std::lock_guard<std::mutex> guard(client->socket_mtx);
      if(client->socket.receive(packet)) {
        pkg::sync_struct sync = packet.data;
        if(sync.no_actions < 0)continue;
        /* printf("Received frame sync: %f\n", sync.frame); */
        client->frame_schedule.push(sync);
        {
          std::lock_guard<std::mutex> guard(client->soccer.mtx);
          Timer::time_t current_time = client->soccer.timer.current_time;
          delay = current_time - sync.frame;
        }
      }
    }
  }

  std::priority_queue<
    pkg::sync_struct,
    std::vector<pkg::sync_struct>,
    std::greater<pkg::sync_struct>
  > frame_schedule;

  std::queue<Timer::time_t> frames;
  static constexpr size_t FRAMERATE = 48;

  void process_frames(Timer::time_t max_frame) {
    /* printf("  processing frames up to %f\n", max_frame); */
    /* printf("  processing frame count: at most %lu\n", frames.size()); */
    int no_frames = 0;
    while(!frames.empty() && frames.front() < max_frame) {
      Timer::time_t new_frame = frames.front();
      /* printf("    processed: frame %f\n", new_frame); */
      soccer.idle(new_frame);
      last_frame = new_frame;
      frames.pop();
      ++no_frames;
    }
  }

  void unpack_sync_unit(const pkg::sync_struct &sync) {
    std::lock_guard<std::mutex> guard(soccer.mtx);
    if(sync.id == Ball::NO_OWNER) {
      soccer.ball.vertical_speed = sync.vertical_speed;
    } else {
      soccer.get_player(sync.id).vertical_speed = sync.vertical_speed;
    }
    auto &unit = soccer.get_unit(sync.id);
    unit.pos = sync.pos;
    unit.facing = sync.angle;
    unit.facing_dest = sync.angle_dest;
    unit.moving_speed = sync.movement_speed;
    unit.dest = glm::vec3(sync.dest, 0);

    /* soccer.timer.set_time(sync.frame); */
    /* soccer.set_control_player(sync.ball_owner); */
  }

  void unpack_sync_action(const pkg::sync_struct &event) {
    std::lock_guard<std::mutex> guard(soccer.mtx);
    if(!event.has_action())return;
    switch(event.action.a) {
      case pkg::Action::Z:soccer.z_action(event.action.id);break;
      case pkg::Action::X:soccer.x_action(event.action.id, event.action.dir);break;
      case pkg::Action::C:soccer.c_action(event.action.id, event.action.dest);break;
      case pkg::Action::V:soccer.v_action(event.action.id);break;
      case pkg::Action::F:soccer.f_action(event.action.id, event.action.dir);break;
      case pkg::Action::S:soccer.s_action(event.action.id);break;
      case pkg::Action::M:soccer.m_action(event.action.id, event.action.dest);break;
      case pkg::Action::NO_ACTION:break;
    }
    ++no_actions;
  }

  void unpack_sync(const pkg::sync_struct &sync) {
    unpack_sync_unit(sync);
    unpack_sync_action(sync);
  }

  void idle(Timer::time_t curtime) {
    if(curtime <= Timer::time_start())return;

    constexpr Timer::time_t max_framediff = 1. / FRAMERATE;
    Timer::time_t max_new_frame = std::fmin(last_frame + max_framediff, curtime);
    if(!frames.empty()) {
      max_new_frame = std::fmax(max_new_frame, frames.front() + .0001);
    }

    std::lock_guard<std::mutex> guard_frames(frame_schedule_mtx);
    pkg::sync_struct next_event;
    /* printf("frame: %f\n", last_frame); */
    /* printf("curtime: %f\n", curtime); */
    /* printf("max_frame: %f\n", max_new_frame); */
    while(!frame_schedule.empty()) {
      next_event = frame_schedule.top();
      /* printf("next event: %f\n", next_event.frame); */
      int diff_action = next_event.no_actions - no_actions;
      // pursue "no action" if no action can possibly be happening within framediff
      if(max_new_frame < next_event.frame && (
        diff_action == 0
        || (diff_action == 1 && next_event.has_action())))
      {
        process_frames(max_new_frame);
        break;
      } else if(diff_action == 0) {
      // we are within reach of "no action" frame sync. nothing has happened up to that frame
        process_frames(next_event.frame);
        unpack_sync(next_event);
        frame_schedule.pop();
      } else if(diff_action == 1 && next_event.has_action()) {
      // we are within reach of a frame sync which contains an action
        /* printf("Processing action: %f\n", next_event.frame); */
        process_frames(next_event.frame);
        if(next_event.frame < max_new_frame) {
          unpack_sync(next_event);
          frame_schedule.pop();
        }
        process_frames(max_new_frame);
      } else {
      // there is a mismatch because some frame syncs havent arrived
        break;
      }
    }
    // push current frame to the end.
    frames.push(curtime);
  }

  bool finalize = false;
  void stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    finalize = true;
  }
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  ~Remote() {
    stop();
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
