#pragma once

#include <cstdint>

#include <algorithm>
#include <set>
#include <queue>
#include <thread>
#include <mutex>
#include <chrono>

#include "Soccer.hpp"
#include "Network.hpp"
#include "Logger.hpp"

enum class IntelligenceType {
  ABSTRACT,
  SERVER,
  REMOTE,
  COMPUTER
};

template <IntelligenceType IntelligenceT> struct Intelligence;

template <>
struct Intelligence <IntelligenceType::ABSTRACT> {
  virtual void z_action() = 0;
  virtual void x_action(float) = 0;
  virtual void c_action(glm::vec3) = 0;
  virtual void v_action() = 0;
  virtual void f_action(float) = 0;
  virtual void s_action() = 0;
  virtual void m_action(glm::vec3) = 0;
  virtual void idle(Timer::time_t) = 0;
  virtual void start() = 0;
  virtual bool should_stop() = 0;
  virtual void stop() = 0;
  virtual ~Intelligence()
  {}

  enum class State {
    DEFAULT, QUIT
  };
  State state_ = State::DEFAULT;
  std::recursive_mutex state_mtx;
  void set_state(State state) {
    std::lock_guard<std::recursive_mutex> guard(state_mtx);
    state_ = state;
  }
  State state() {
    std::lock_guard<std::recursive_mutex> guard(state_mtx);
    return state_;
  }
  void leave() {
    set_state(State::QUIT);
  }
  bool has_quit() {
    return state() == State::QUIT;
  }
};

using SoccerServer = Intelligence<IntelligenceType::SERVER>;
using SoccerRemote = Intelligence<IntelligenceType::REMOTE>;
using SoccerComputer = Intelligence<IntelligenceType::COMPUTER>;

namespace pkg {
  // listen/send action
  enum class Action : uint8_t { NO_ACTION,Z,X,C,V,F,S,M };
  struct action_struct {
    Action a;
    int8_t id;
    float dir;
    glm::vec3 dest;
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

    uint16_t no_actions;
    action_struct action = {
      .a=Action::NO_ACTION
    };

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

template <>
struct Intelligence<IntelligenceType::SERVER> : public Intelligence<IntelligenceType::ABSTRACT> {
  int8_t id_;
  int no_actions = 0;
  Soccer &soccer;
  net::Socket<net::SocketType::UDP> &socket;
  std::thread server_thread;
  std::recursive_mutex no_actions_mtx;
  std::recursive_mutex finalize_mtx;

  std::set<net::Addr> clients;

  Intelligence(int id, Soccer &soccer, net::Socket<net::SocketType::UDP> &socket, std::set<net::Addr> clients):
    id_(id), soccer(soccer),
    socket(socket), clients(clients)
  {}

  static void run(SoccerServer *server) {
    constexpr int EVENT_SYNC = 1;
    Timer timer;
    timer.set_time(Timer::system_time());
    timer.set_timeout(EVENT_SYNC, .1);
    server->socket.listen(
      [&]() mutable {
        if(server->has_quit()) {
          return !server->should_stop();
        }
        // send sync data for random unit showing that no action occured until a
        // certain time point
        Timer::time_t server_time = Timer::system_time();
        timer.set_time(server_time);
        if(timer.timed_out(EVENT_SYNC)) {
          timer.set_event(EVENT_SYNC);
          std::lock_guard<std::recursive_mutex> guard(server->soccer.mtx);
          int no_ids = server->soccer.team1.size() + server->soccer.team2.size() + 1;
          int8_t unit_id = (rand() % no_ids) - 1;
          for(const auto &addr : server->clients) {
            server->socket.send(net::make_package(addr, server->get_sync_data(unit_id)));
          }
        }
        return !server->should_stop();
      },
      [&](const net::Blob &blob) {
        // discard packages not belonging to current players
        if(server->has_quit() || server->clients.find(blob.addr) == std::end(server->clients)) {
          return !server->should_stop();
        }
        // if this package seems to be action, perform action and send responses
        blob.try_visit_as<pkg::action_struct>([&](const auto action) mutable {
          server->perform_action(action);
          {
            std::lock_guard<std::recursive_mutex> guard(server->no_actions_mtx);
            ++server->no_actions;
          }
          {
            std::lock_guard<std::recursive_mutex> guard(server->soccer.mtx);
            for(const auto &addr : server->clients) {
              server->socket.send(net::make_package(addr, server->get_sync_data(action.id)));
            }
          }
        });
        return !server->should_stop();
      }
    );
  }

  pkg::sync_struct get_sync_data(int unit_id=Ball::NO_OWNER) {
    pkg::sync_struct usd;
    std::lock_guard<std::recursive_mutex> guard(soccer.mtx);
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
      std::lock_guard<std::recursive_mutex> guard(no_actions_mtx);
      usd.no_actions = no_actions;
    }
    return usd;
  }

  void perform_action(pkg::action_struct action) {
    std::lock_guard<std::recursive_mutex> guard(soccer.mtx);
    switch(action.a) {
      case pkg::Action::Z: soccer.z_action(action.id); break;
      case pkg::Action::X: soccer.x_action(action.id, action.dir); break;
      case pkg::Action::C: soccer.c_action(action.id, action.dest); break;
      case pkg::Action::V: soccer.v_action(action.id); break;
      case pkg::Action::F: soccer.f_action(action.id, action.dir); break;
      case pkg::Action::S: soccer.s_action(action.id); break;
      case pkg::Action::M: soccer.m_action(action.id, action.dest); break;
    }
  }

  void idle(Timer::time_t curtime) {
    soccer.idle(curtime);
  }

  int id() const {
    return id_;
  }

  bool finalize = true;
  void start() {
    Logger::Info("iserver: started\n");
    ASSERT(should_stop());
    finalize = false;
    server_thread = std::thread(SoccerServer::run, this);
  }
  void stop() {
    ASSERT(!should_stop());
    {
      std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
      finalize = true;
    }
    server_thread.join();
    Logger::Info("iserver: finished\n");
  }
  bool should_stop() {
    std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
    return finalize;
  }

  void z_action() {
    soccer.z_action(id_);
  }

  void x_action(float dir) {
    soccer.x_action(id_, dir);
  }

  void c_action(glm::vec3 dest) {
    soccer.c_action(id_, dest);
  }

  void v_action() {
    soccer.v_action(id_);
  }

  void f_action(float dir) {
    soccer.f_action(id_, dir);
  }

  void s_action() {
    soccer.s_action(id_);
  }

  void m_action(glm::vec3 dest) {
    soccer.m_action(id_, dest);
  }
};

template <>
struct Intelligence<IntelligenceType::REMOTE> : public Intelligence<IntelligenceType::ABSTRACT> {
  int8_t id_;
  Soccer &soccer;
  net::Addr server_addr;
  net::Socket<net::SocketType::UDP> &socket;
  int no_actions = 0;
  Timer::time_t last_frame = Timer::time_start();
  std::thread client_thread;
  std::recursive_mutex frame_schedule_mtx;
  std::recursive_mutex finalize_mtx;

  Intelligence(int id, Soccer &soccer, net::Socket<net::SocketType::UDP> &socket, net::Addr server_addr):
    id_(id),
    soccer(soccer),
    server_addr(server_addr),
    socket(socket)
  {}

  static void run(SoccerRemote *client) {
    Timer::time_t delay = 1.;
    client->socket.listen(
      [&]() mutable {
        return !client->should_stop();
      },
      [&](const net::Blob &blob) mutable {
        if(client->has_quit() || blob.addr != client->server_addr) {
          return !client->should_stop();
        }
        blob.try_visit_as<pkg::sync_struct>([&](const auto &sync) mutable {
          std::lock_guard<std::recursive_mutex> guard(client->frame_schedule_mtx);
          client->frame_schedule.push(sync);
          std::lock_guard<std::recursive_mutex> sguard(client->soccer.mtx);
          Timer::time_t current_time = client->soccer.timer.current_time;
          delay = current_time - sync.frame;
        });
        return !client->should_stop();
      }
    );
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
    std::lock_guard<std::recursive_mutex> guard(soccer.mtx);
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
    std::lock_guard<std::recursive_mutex> guard(soccer.mtx);
    if(!event.has_action())return;
    switch(event.action.a) {
      case pkg::Action::Z:soccer.z_action(event.action.id);break;
      case pkg::Action::X:soccer.x_action(event.action.id, event.action.dir);break;
      case pkg::Action::C:soccer.c_action(event.action.id, event.action.dest);break;
      case pkg::Action::V:soccer.v_action(event.action.id);break;
      case pkg::Action::F:soccer.f_action(event.action.id, event.action.dir);break;
      case pkg::Action::S:soccer.s_action(event.action.id);break;
      case pkg::Action::M:soccer.m_action(event.action.id, event.action.dest);break;
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

    std::lock_guard<std::recursive_mutex> guard_frames(frame_schedule_mtx);
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

  bool finalize = true;
  void start() {
    Logger::Info("iclient: started\n");
    ASSERT(should_stop());
    finalize = false;
    client_thread = std::thread(SoccerRemote::run, this);
  }
  void stop() {
    ASSERT(!should_stop());
    {
      std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
      finalize = true;
    }
    client_thread.join();
    Logger::Info("iclient: finished\n");
  }
  bool should_stop() {
    std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
    return finalize;
  }

  template <typename T>
  void send_action(const T &data) {
    printf("iclient: sending action %d\n", data.a);
    socket.send(net::make_package(server_addr, data));
  }

  void z_action() {
    send_action((pkg::action_struct){
      .a = pkg::Action::Z,
      .id = id_
    });
  }

  void x_action(float dir) {
    pkg::action_struct d;// = { .a=pkg::Action::X, .id=id_, .dir=dir };
    d.a=pkg::Action::X,d.id=id_,d.dir=dir;
    send_action(d);
  }

  void c_action(glm::vec3 dest) {
    pkg::action_struct d;// = { .a=pkg::Action::C, .id=id_, .dest=dest };
    d.a=pkg::Action::C;d.id=id_;d.dest=dest;
    send_action(d);
  }

  void v_action() {
    send_action((pkg::action_struct){
      .a = pkg::Action::V,
      .id = id_
    });
  }

  void f_action(float dir) {
    send_action((pkg::action_struct){
      .a = pkg::Action::F,
      .id = id_,
      .dir = dir
    });
  }

  void s_action() {
    pkg::action_struct d = { .a=pkg::Action::S, .id=id_ };
    send_action(d);
  }

  void m_action(glm::vec3 dest) {
    pkg::action_struct d;// = { .a=pkg::Action::M, .id=id_, .dest=dest };
    d.a=pkg::Action::M,d.id=id_,d.dest=dest;
    send_action(d);
  }
};
