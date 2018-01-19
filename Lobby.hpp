#pragma once

#include "Debug.hpp"
#include "Logger.hpp"

#include <cstdint>

#include <map>
#include <set>
#include <thread>
#include <mutex>

#include "Network.hpp"
#include "Soccer.hpp"
#include "Intelligence.hpp"

namespace pkg {
  enum class HelloAction : int8_t {
    CONNECT, DISCONNECT, NOTHING
  };

  struct lobby_hello_struct {
    HelloAction a;
  };

  struct lobby_sync_struct {
    HelloAction a = HelloAction::NOTHING;
    Timer::time_t time;
    net::Addr address;

    constexpr bool operator<(const lobby_sync_struct &other) const {
      return time < other.time;
    }

    constexpr bool operator>(const lobby_sync_struct &other) const {
      return time > other.time;
    }
  };
}

struct Lobby {
  struct Participant {
    int8_t ind;
    IntelligenceType itype;
    int8_t team;
  };
  std::map<net::Addr, Participant> players;
  std::mutex mtx;

  Lobby()
  {}

  int team1=0, team2=0;

  void add_participant(net::Addr addr, IntelligenceType itype=IntelligenceType::REMOTE) {
    std::lock_guard<std::mutex> guard(mtx);
    bool team = (team1 >= team2) ? Soccer::Team::RED_TEAM : Soccer::Team::BLUE_TEAM;
    players.insert({addr, Participant({
      .ind=int8_t(players.size()),
      .itype=itype,
      .team=team
    })});
    ++(!team?team1:team2);
  }

  void remove_participant(net::Addr addr) {
    std::lock_guard<std::mutex> guard(mtx);
    if(players.find(addr) != std::end(players)) {
      players.erase(addr);
    }
  }

  void change_team(net::Addr addr) {
    std::lock_guard<std::mutex> guard(mtx);
    auto &t = players.at(addr).team;
    if(t == Soccer::Team::RED_TEAM) {
      --team1; ++team2;
      t = Soccer::Team::BLUE_TEAM;
    } else {
      ++team1; --team2;
      t = Soccer::Team::RED_TEAM;
    }
  }

  Soccer get_soccer() {
    std::lock_guard<std::mutex> guard(mtx);
    return Soccer(team1, team2);
  }
};

struct LobbyActor {
  Lobby lobby;
  LobbyActor():
    lobby()
  {}
  bool is_active() {
    return !should_stop();
  }
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool should_stop() = 0;
  virtual Intelligence<IntelligenceType::ABSTRACT> *make_intelligence(Soccer &soccer) = 0;
  virtual ~LobbyActor()
  {}

  std::mutex state_mtx;
  enum class State {
    DEFAULT, STARTED, QUIT
  };
  State state_ = State::DEFAULT;
  void set_state(State state) {
    std::lock_guard<std::mutex> guard(state_mtx);
    state_ = state;
  }
  State state() {
    std::lock_guard<std::mutex> guard(state_mtx);
    return state_;
  }
  void action_leave() {
    set_state(State::QUIT);
  }
  void action_start() {
    set_state(State::STARTED);
  }
  bool has_quit() {
    return state() == State::QUIT;
  }
  bool has_started() {
    return state() == State::STARTED;
  }
};

// server-side
struct LobbyServer : LobbyActor {
  net::Socket<net::SocketType::UDP> &socket;
  std::thread server_thread;
  std::mutex socket_mtx;
  std::mutex finalize_mtx;

  LobbyServer(net::Socket<net::SocketType::UDP> &socket):
    LobbyActor(),
    socket(socket)
  {}

  net::Addr host() {
    return net::Addr(INADDR_ANY, 0);
  }

  static void run(LobbyServer *server) {
    Timer timer;
    timer.set_time(Timer::system_time());
    server->socket.listen(server->socket_mtx,
      [&]() mutable {
        return !server->should_stop();
      },
      [&](const net::Blob &blob) mutable {
        blob.try_visit_as<pkg::lobby_hello_struct>([&](const auto &hello) mutable {
          if(hello.a != pkg::HelloAction::CONNECT) {
            bool found = false;
            std::lock_guard<std::mutex> guard(server->lobby.mtx);
            for(auto &p: server->lobby.players) {
              if(p.first == blob.addr) {
                found = true;
                break;
              }
            }
            if(!found) {
              return !server->should_stop();
            }
          }
          {
            std::lock_guard<std::mutex> guard(server->lobby.mtx);
            switch(hello.a) {
              case pkg::HelloAction::CONNECT:server->lobby.add_participant(blob.addr);break;
              case pkg::HelloAction::DISCONNECT:server->lobby.remove_participant(blob.addr);break;
              case pkg::HelloAction::NOTHING:break;
            }
          }
          Timer::time_t server_time = Timer::system_time();
          for(auto &p : server->lobby.players) {
            auto addr = p.first;
            if(addr == server->host())continue;
            server->socket.send(net::make_package(addr, (pkg::lobby_sync_struct){
              .a = hello.a,
              .time = server_time,
              .address = blob.addr
            }));
          }
        });
        return !server->should_stop();
      }
    );
  }

  bool finalize = false;
  void start() {
    Logger::Info("lserver: started\n");
    lobby.add_participant(host(), IntelligenceType::SERVER);
    finalize = false;
    server_thread = std::thread(LobbyServer::run, this);
  }
  void stop() {
    ASSERT(!should_stop());
    action_unhost();
    {
      std::lock_guard<std::mutex> guard(finalize_mtx);
      finalize = true;
    }
    server_thread.join();
    Logger::Info("lserver: finished\n");
  }
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  void action_unhost() {
    std::lock_guard<std::mutex> guard(lobby.mtx);
    Logger::Info("lserver: action unhost\n");
    for(auto &p : lobby.players) {
      auto &addr = p.first;
      if(addr == host())continue;
      socket.send(net::make_package(addr, (pkg::lobby_sync_struct){
        .a = pkg::HelloAction::DISCONNECT
      }));
    }
  }

  Intelligence<IntelligenceType::ABSTRACT> *make_intelligence(Soccer &soccer) {
    std::lock_guard<std::mutex> guard(lobby.mtx);
    std::set<net::Addr> clients;
    for(auto p : lobby.players) {
      if(p.second.itype == IntelligenceType::REMOTE) {
        clients.insert(p.first);
      }
    }
    return new SoccerServer(lobby.players[host()].ind, soccer, socket, clients);
  }
};

struct LobbyClient : LobbyActor {
  net::Addr host;
  net::Socket<net::SocketType::UDP> socket;
  std::thread client_thread;
  std::mutex finalize_mtx;
  std::mutex socket_mtx;

  // connect at construction
  LobbyClient(net::Socket<net::SocketType::UDP> &socket, net::Addr host):
    LobbyActor(),
    host(host),
    socket(socket)
  {}

  void send_action(pkg::lobby_hello_struct hello) {
    std::lock_guard<std::mutex> guard(socket_mtx);
    socket.send(net::make_package(host, hello));
  }

  std::priority_queue<pkg::lobby_sync_struct> lobby_actions;
  std::mutex lsync_mtx;
  static void run(LobbyClient *client) {
    client->socket.listen(client->socket_mtx,
      [&]() mutable {
        return !client->should_stop();
      },
      [&](const net::Blob &blob) mutable {
        if(blob.addr != client->host) {
          return !client->should_stop();
        }
        blob.try_visit_as<pkg::lobby_sync_struct>([&](const auto lsync) mutable {
          if(lsync.a == pkg::HelloAction::DISCONNECT) {
            return;
          }
          std::lock_guard<std::mutex> guard(client->lsync_mtx);
          client->lobby_actions.push(lsync);
        });
        return !client->should_stop();
      }
    );
  }

  void start() {
    ASSERT(should_stop());
    Logger::Info("lclient: started\n");
    finalize = false;
    send_action((pkg::lobby_hello_struct){
      .a = pkg::HelloAction::CONNECT
    });
    client_thread = std::thread(LobbyClient::run, this);
  }

  bool finalize = false;
  void stop() {
    ASSERT(!should_stop());
    action_quit();
    {
      std::lock_guard<std::mutex> guard(finalize_mtx);
      finalize = true;
    }
    client_thread.join();
    Logger::Info("lclient: finished\n");
  }
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  void action_quit() {
    Logger::Info("lclient: sending action quit\n");
    pkg::lobby_hello_struct hello = { .a = pkg::HelloAction::DISCONNECT };
    send_action(hello);
  }

  Intelligence<IntelligenceType::ABSTRACT> *make_intelligence(Soccer &soccer) {
    return nullptr;
    /* return new SoccerRemote( */
    /*   lobby.players[myaddr].ind, */
    /*   soccer, */
    /*   socket.port(), */
    /*   host */
    /* ); */
  }
};
