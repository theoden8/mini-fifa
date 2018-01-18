#pragma once

#include "Debug.hpp"
#include "Logger.hpp"

#include <cstdint>
#include <map>
#include <thread>
#include <mutex>

#include "Network.hpp"
#include "Soccer.hpp"
#include "Intelligence.hpp"

namespace pkg {
  enum class HelloAction : int8_t {
    CONNECT, DISCONNECT, NOTHING
  };

  struct hello_struct {
    HelloAction a;
  };

  struct lobbysync_struct {
    HelloAction a = HelloAction::NOTHING;
    Timer::time_t time;
    net::Addr address;

    constexpr bool operator<(const lobbysync_struct &other) const {
      return time < other.time;
    }

    constexpr bool operator>(const lobbysync_struct &other) const {
      return time > other.time;
    }
  };
}

struct Lobby {
  struct Participant {
    int8_t ind;
    IntelligenceType itype;
    bool team;
  };
  std::map<net::Addr, Participant> players;
  std::mutex mtx;

  Lobby()
  {}

  int team1=0, team2=0;

  void reset() {
    players.clear();
  }

  void add_participant(net::Addr addr, IntelligenceType itype=IntelligenceType::REMOTE) {
    std::lock_guard<std::mutex> guard(mtx);
    bool team = (team1 > team2) ? Soccer::Team::RED_TEAM : Soccer::Team::BLUE_TEAM;
    players.insert({addr, Participant({
      .ind=int8_t(players.size()),
      .itype=itype,
      .team=team
    })});
    ++(team?team1:team2);
  }

  void remove_participant(net::Addr addr) {
    std::lock_guard<std::mutex> guard(mtx);
    if(players.find(addr) != std::end(players)) {
      players.erase(addr);
    }
  }

  void change_team(net::Addr addr) {
    std::lock_guard<std::mutex> guard(mtx);
    bool &t = players.at(addr).team;
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
  Lobby &lobby;
  LobbyActor(Lobby &lobby):
    lobby(lobby)
  {
    lobby.reset();
  }
  bool is_active() {
    return !should_stop();
  }
  virtual void stop() = 0;
  virtual bool should_stop() = 0;
  virtual Intelligence<IntelligenceType::ABSTRACT> *make_intelligence(Soccer &soccer) = 0;
  virtual ~LobbyActor()
  {}
};

// server-side
struct LobbyServer : LobbyActor {
  net::Addr host;
  net::Addr metaserver;
  net::Socket<net::SocketType::UDP> socket;
  std::thread server_thread;
  std::mutex socket_mtx;
  std::mutex finalize_mtx;

  LobbyServer(Lobby &lobby, net::Addr metaserver, net::port_t port=4567):
    LobbyActor(lobby),
    host(net::ip_from_ints(0, 0, 0, 0), port),
    socket(port),
    metaserver(metaserver)
  {
    Logger::Info("lserver: started\n");
    lobby.add_participant(host, IntelligenceType::SERVER);
    server_thread = std::thread(LobbyServer::run, this);
  }

  static void run(LobbyServer *server) {
    Timer timer;
    timer.set_time(Timer::system_time());
    server->socket.listen(server->socket_mtx, [&](const net::Blob &blob) mutable {
      blob.try_visit_as<pkg::hello_struct>([&](const auto &hello) mutable {
        if(hello.a != pkg::HelloAction::CONNECT) {
          bool found = false;
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
        switch(hello.a) {
          case pkg::HelloAction::CONNECT:server->lobby.add_participant(blob.addr);break;
          case pkg::HelloAction::DISCONNECT:server->lobby.remove_participant(blob.addr);break;
          case pkg::HelloAction::NOTHING:break;
        }
        Timer::time_t server_time = Timer::system_time();
        for(auto &p : server->lobby.players) {
          auto addr = p.first;
          server->socket.send(net::make_package(addr, (pkg::lobbysync_struct){
            .a = hello.a,
            .time = server_time,
            .address = blob.addr
          }));
        }
      });
      return !server->should_stop();
    });
  }

  bool finalize = false;
  void stop() {
    Logger::Info("lserver: finished\n");
    action_unhost();
    std::lock_guard<std::mutex> guard(finalize_mtx);
    finalize = true;
    server_thread.join();
  }
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  void action_unhost() {
    for(auto &p : lobby.players) {
      std::lock_guard<std::mutex> guard(socket_mtx);
      Logger::Info("lserver: action unhost\n");
      socket.send(net::make_package(p.first, (pkg::lobbysync_struct){
        .a = pkg::HelloAction::DISCONNECT
      }));
    }
  }

  Intelligence<IntelligenceType::ABSTRACT> *make_intelligence(Soccer &soccer) {
    std::lock_guard<std::mutex> guard(lobby.mtx);
    std::vector<net::Addr> clients;
    for(auto p : lobby.players) {
      if(p.second.itype == IntelligenceType::REMOTE) {
        clients.push_back(p.first);
      }
    }
    return new SoccerServer(lobby.players[host].ind, soccer, host.port, clients);
  }
};

struct LobbyClient : LobbyActor {
  net::Addr host;
  net::Socket<net::SocketType::UDP> socket;
  std::thread client_thread;
  std::mutex finalize_mtx;
  std::mutex socket_mtx;
  bool is_connected = false;

  // connect at construction
  LobbyClient(Lobby &lobby, net::Addr addr, net::port_t port=4567):
    LobbyActor(lobby), socket(port)
  {
    connect(addr);
  }

  void send_action(pkg::hello_struct hello) {
    std::lock_guard<std::mutex> guard(socket_mtx);
    socket.send(net::make_package(host, hello));
  }

  std::priority_queue<pkg::lobbysync_struct> lobby_actions;
  std::mutex lsync_mtx;
  static void run(LobbyClient *client) {
    client->socket.listen(client->socket_mtx, [&](const net::Blob &blob) mutable {
      if(blob.addr != client->host) {
        return !client->should_stop();
      }
      blob.try_visit_as<pkg::lobbysync_struct>([&](const auto lsync) mutable {
        if(lsync.a == pkg::HelloAction::DISCONNECT) {
          return;
        }
        std::lock_guard<std::mutex> guard(client->lsync_mtx);
        client->lobby_actions.push(lsync);
      });
      return !client->should_stop();
    });
  }

  void connect(net::Addr addr) {
    Logger::Info("lclient: started\n");
    ASSERT(!is_connected);
    host = addr;
    finalize = false;
    is_connected = true;
    pkg::hello_struct hello = { .a = pkg::HelloAction::CONNECT };
    send_action(hello);
    client_thread = std::thread(LobbyClient::run, this);
  }

  bool finalize = false;
  void stop() {
    Logger::Info("lclient: finished\n");
    if(is_connected) {
      action_quit();
    }
    std::lock_guard<std::mutex> guard(finalize_mtx);
    finalize = true;
    client_thread.join();
  }
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  void action_quit() {
    Logger::Info("lclient: sending action quit\n");
    pkg::hello_struct hello = { .a = pkg::HelloAction::DISCONNECT };
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
