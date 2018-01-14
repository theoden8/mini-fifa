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
  enum class HelloAction {
    CONNECT, DISCONNECT, NOTHING
  };

  struct hello_struct {
    HelloAction a = HelloAction::NOTHING;
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
  bool is_active() {
    return !should_stop();
  }
  virtual void stop() = 0;
  virtual bool should_stop() = 0;
  virtual Intelligence<IntelligenceType::ABSTRACT> make_intelligence(Soccer &soccer) = 0;
};

// server-side
struct LobbyServer : LobbyActor {
  Lobby &lobby;

  net::Addr host;
  net::Socket<net::SocketType::UDP> socket;
  std::thread server_thread;
  std::mutex finalize_mtx;

  LobbyServer(Lobby &lobby, net::port_t port):
    lobby(lobby),
    host(net::ip_from_ints(0, 0, 0, 0), port), socket(port)
  {
    lobby.add_participant(host, IntelligenceType::SERVER);
    server_thread = std::thread(LobbyServer::run, this);
  }

  static void run(LobbyServer *server) {
    using sys_time_t = std::chrono::nanoseconds;
    auto server_time_start = std::chrono::system_clock::now();
    while(!server->should_stop()) {
      net::Package<pkg::hello_struct> package;
      if(server->socket.receive(package)) {
        auto &hello = package.data;
        if(hello.a != pkg::HelloAction::NOTHING)continue;
        switch(hello.a) {
          case pkg::HelloAction::CONNECT:server->lobby.add_participant(package.addr);break;
          case pkg::HelloAction::DISCONNECT:server->lobby.remove_participant(package.addr);break;
          case pkg::HelloAction::NOTHING:break;
        }
        // send information about the new member to everyone
        auto server_time_now = std::chrono::system_clock::now();
        Timer::time_t server_time = 1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(server_time_now - server_time_start).count();
        pkg::lobbysync_struct data = { .a = hello.a, .time = server_time, .address = package.addr };
        for(auto &p : server->lobby.players) {
          net::Package<pkg::lobbysync_struct> spackage(p.first, data);
          server->socket.send(spackage);
        }
      }
    }
  }

  Intelligence<IntelligenceType::ABSTRACT> make_intelligence(Soccer &soccer) {
    std::lock_guard<std::mutex> guard(lobby.mtx);
    std::vector<net::Addr> clients;
    for(auto p : lobby.players) {
      if(p.second.itype == IntelligenceType::REMOTE) {
        clients.push_back(p.first);
      }
    }
    return Server(lobby.players[host].ind, soccer, host.port, clients);
    TERMINATE("server not found\n");
  }

  bool finalize = false;
  void stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    finalize = true;
    server_thread.join();
  }
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  ~LobbyServer() {
    if(!should_stop()) {
      stop();
      server_thread.join();
    }
  }
};

struct LobbyClient : LobbyActor {
  Lobby &lobby;

  net::Addr host;
  net::Socket<net::SocketType::UDP> socket;
  std::thread client_thread;
  std::mutex finalize_mtx;
  std::mutex socket_mtx;
  bool is_connected = false;

  LobbyClient(Lobby &lobby, net::port_t port):
    lobby(lobby), socket(port)
  {}

  // connect at construction
  LobbyClient(Lobby &lobby, net::port_t port, net::Addr addr):
    lobby(lobby), socket(port)
  {
    connect(addr);
  }

  void send_action(pkg::hello_struct hello) {
    std::lock_guard<std::mutex> guard(socket_mtx);
    net::Package<pkg::hello_struct> package(host, hello);
    socket.send(package);
  }

  bool finalize = false;
  void stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    finalize = true;
    client_thread.join();
  }
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  static void run(LobbyClient *client) {
    while(!client->should_stop()) {
    }
  }

  void connect(net::Addr addr) {
    ASSERT(!is_connected);
    host = addr;
    finalize = false;
    is_connected = true;
    pkg::hello_struct hello = { .a = pkg::HelloAction::CONNECT };
    send_action(hello);
    client_thread = std::thread(
      LobbyClient::run,
      this
    );
  }

  void disconnect() {
    pkg::hello_struct hello = { .a = pkg::HelloAction::DISCONNECT };
    send_action(hello);
    stop();
    is_connected = false;
  }

  Intelligence<IntelligenceType::ABSTRACT> make_intelligence(Soccer &soccer) {
    return Remote(
      lobby.players[myaddr].ind,
      soccer,
      socket.port(),
      host
    );
  }

  ~LobbyClient() {
    if(is_connected) {
      disconnect();
    }
  }
};
