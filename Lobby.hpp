#pragma once

#include "Debug.hpp"
#include "Logger.hpp"
#include "Optimizations.hpp"

#include <cstdint>

#include <map>
#include <set>
#include <thread>
#include <mutex>

#include "Network.hpp"
#include "Soccer.hpp"
#include "Intelligence.hpp"

namespace pkg {
  enum class LobbyAction : int8_t {
    NOTHING, CONNECT, DISCONNECT, UNHOST, START, QUERY
  };

  struct lobby_hello_struct {
    LobbyAction action;
  } ATTRIB_PACKED;

  struct lobby_start_struct {
    LobbyAction action = LobbyAction::START;
    int8_t index;
    int8_t team1;
    int8_t team2;
  } ATTRIB_PACKED;

  struct lobby_query_struct {
    LobbyAction action = LobbyAction::QUERY;
    net::Addr addr;
  } ATTRIB_PACKED;

  struct lobby_participant_struct {
    int8_t ind;
    IntelligenceType itype;
    int8_t team;
  } ATTRIB_PACKED;

  struct lobby_query_response_struct {
    net::Addr addr;
    int8_t active;
    pkg::lobby_participant_struct info;
  } ATTRIB_PACKED;

  enum class MSAction : int8_t {
    HELLO,
    QUERY,
    HOST,
    UNHOST
  };

  // all int8
  struct metaserver_hello_struct {
    MSAction action = MSAction::HELLO;
  } ATTRIB_PACKED;

  // all int8=char
  struct metaserver_host_struct {
    MSAction action;
    char name[30] = "";

    void set_name(std::string &s) {
      strncpy(name, s.c_str(), std::min<int>(s.length() + 1, 30));
      name[29] = '\0';
    }
  } ATTRIB_PACKED;
}

class Lobby {
public:
private:
  std::recursive_mutex mtx;
  std::map<net::Addr, pkg::lobby_participant_struct> players;
  int team1_=0, team2_=0;
public:
  Lobby()
  {}

  void add_participant(net::Addr addr, IntelligenceType itype=IntelligenceType::REMOTE) {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    bool team = (team1_ <= team2_) ? Soccer::Team::RED_TEAM : Soccer::Team::BLUE_TEAM;
    players.insert({addr, pkg::lobby_participant_struct({
      .ind=int8_t(players.size()),
      .itype=itype,
      .team=team
    })});
    ++(!team?team1_:team2_);
  }

  size_t size() {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    return players.size();
  }

  bool empty() {
    return size() == 0;
  }

  void remove_participant(net::Addr addr) {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    if(players.find(addr) != std::end(players)) {
      --(!players[addr].team?team1_:team2_);
      players.erase(addr);
    }
  }

  decltype(auto) operator[](net::Addr addr) {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    return players[addr];
  }

  bool find(net::Addr addr) {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    return players.find(addr) != std::end(players);
  }

  template <typename F>
  void iterate(F &&func) {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    for(auto &p : players) {
      if(!func(p)) {
        break;
      }
    }
  }

  net::Addr random() {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    ASSERT(!players.empty());
    int i = 0, j = rand() % players.size();
    for(const auto &p : players) {
      if(i == j) {
        return p.first;
      }
      ++i;
    }
    throw std::runtime_error("bullshit!");
  }

  void change_team(net::Addr addr) {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    auto &t = players.at(addr).team;
    if(t == Soccer::Team::RED_TEAM) {
      --team1_; ++team2_;
      t = Soccer::Team::BLUE_TEAM;
    } else {
      ++team1_; --team2_;
      t = Soccer::Team::RED_TEAM;
    }
  }

  auto team1() {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    return team1_;
  }

  auto team2() {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    return team2_;
  }

  void clear() {
    std::lock_guard<std::recursive_mutex> guard(mtx);
    players.clear();
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

  std::recursive_mutex state_mtx;
  enum class State {
    DEFAULT, STARTED, QUIT
  };
  State state_ = State::DEFAULT;
  void set_state(State state) {
    std::lock_guard<std::recursive_mutex> guard(state_mtx);
    state_ = state;
  }
  State state() {
    std::lock_guard<std::recursive_mutex> guard(state_mtx);
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
  virtual Soccer get_soccer() {
    return Soccer(lobby.team1(), lobby.team2());
  }
  virtual bool is_server() {
    return false;
  }
  virtual bool is_client() {
    return false;
  }
};

// server-side
struct LobbyServer : LobbyActor {
  net::Socket<net::SocketType::UDP> &socket;
  std::thread server_thread;
  std::recursive_mutex finalize_mtx;

  std::set<net::Addr> &metaservers;
  std::recursive_mutex &mservers_mtx;

  Timer timer;
  Timer user_timer;
  static constexpr Timer::key_t EVENT_SEND_HELLO_MSERVERS = 1;
  static constexpr Timer::key_t EVENT_SEND_HELLO_USERS = 2;
  static constexpr Timer::key_t EVENT_CHECK_STATUSES = 3;

  LobbyServer(net::Socket<net::SocketType::UDP> &socket, std::set<net::Addr> &metaservers, std::recursive_mutex &mservers_mtx):
    LobbyActor(),
    socket(socket),
    metaservers(metaservers),
    mservers_mtx(mservers_mtx)
  {
    set_timer();
  }

  net::Addr host() {
    return net::Addr(INADDR_ANY, 0);
  }

  void set_timer() {
    timer.set_timeout(EVENT_SEND_HELLO_MSERVERS, Timer::time_t(1.));
    timer.set_timeout(EVENT_SEND_HELLO_USERS, Timer::time_t(1.));
    timer.set_timeout(EVENT_CHECK_STATUSES, Timer::time_t(3.));
  }

  static void run(LobbyServer *server) {
    server->timer.set_time(Timer::system_time());
    server->socket.listen(
      [&]() mutable {
        server->trigger_events();
        if(server->has_started() || server->has_quit()) {
          return !server->should_stop();
        }
        /* usleep(1e6 / 24); */
        server->timer.set_time(Timer::system_time());
        // send hello to servers
        server->timer.periodic(EVENT_SEND_HELLO_MSERVERS, [&]() mutable {
          Logger::Info("%.2f lserver: sending hello to metaservers\n", server->timer.current_time);
          std::lock_guard<std::recursive_mutex> mguard(server->mservers_mtx);
          for(auto &m : server->metaservers) {
            server->socket.send(net::make_package(m, (pkg::metaserver_hello_struct){
              .action = pkg::MSAction::HELLO
            }));
          }
        });
        // send hello to clients
        server->timer.periodic(EVENT_SEND_HELLO_USERS, [&]() mutable {
          if(rand() % 3 || server->lobby.empty()) {
            server->send_action((pkg::lobby_hello_struct){
              .action = pkg::LobbyAction::NOTHING
            });
          } else {
            net::Addr addr = server->lobby.random();
            server->send_action((pkg::lobby_query_response_struct){
              .addr = addr,
              .active = true,
              .info = server->lobby[addr]
            });
          }
        });
        // clean up inactive users
        server->timer.periodic(EVENT_CHECK_STATUSES, [&]() mutable {
          server->user_timer.set_time(Timer::system_time());
          std::string s = "";
          std::set<net::Addr> exusers;
          {
            server->lobby.iterate([&](auto &p) mutable {
              const auto &u = p.first;
              if(u == server->host()) {
                return true;
              }
              if(server->user_timer.timed_out(Timer::key_t(u.ip))) {
                Logger::Info("%.2f lserver: removing user %s\n", server->timer.current_time, u.to_str().c_str());
                exusers.insert(u);
              } else {
                s += u.to_str() + " ";
              }
              return true;
            });
          }
          for(auto &u : exusers) {
            if(server->lobby.find(u)) {
              server->action_kick(u);
            }
          }
          Logger::Info("%.2f lserver: users [ %s]\n", server->timer.current_time, s.c_str());
        });
        return !server->should_stop();
      },
      [&](const net::Blob &blob) mutable {
        if(server->has_started() || server->has_quit()) {
          return !server->should_stop();
        }
        bool found = server->lobby.find(blob.addr);
        if(found) {
          server->action_activity(blob.addr);
        }
        // received hello from client
        static_assert(net::Typecheck::all_distinct<pkg::lobby_hello_struct, pkg::lobby_query_struct>);
        blob.try_visit_as<pkg::lobby_hello_struct>([&](const auto hello) mutable {
          Logger::Info("%.2f received signal %d from %s\n", server->timer.current_time, hello.action, blob.addr.to_str().c_str());
          switch(hello.action) {
            case pkg::LobbyAction::NOTHING:break;
            case pkg::LobbyAction::CONNECT:
              if(!found) {
                server->action_join(blob.addr);
              }
            break;
            case pkg::LobbyAction::DISCONNECT:
              if(found) {
                server->action_kick(blob.addr);
              }
            break;
            case pkg::LobbyAction::QUERY:break;
            case pkg::LobbyAction::UNHOST:break;
            case pkg::LobbyAction::START:break;
          }
        });
        blob.try_visit_as<pkg::lobby_query_struct>([&](const auto query) mutable {
          if(found) {
            pkg::lobby_query_response_struct data = {
              .addr = query.addr,
              .active = server->lobby.find(query.addr)
            };
            if(data.active) {
              data.info = server->lobby[query.addr];
            }
            server->socket.send(net::make_package(blob.addr, data));
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
    if(has_quit()) {
      action_unhost();
    }
    {
      std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
      finalize = true;
    }
    server_thread.join();
    Logger::Info("lserver: finished\n");
  }
  bool should_stop() {
    std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
    return finalize;
  }

  bool is_server() {
    return true;
  }

  template <typename DataT>
  void send_action(DataT data) {
    lobby.iterate([&](const auto &p) mutable {
      const auto &u = p.first;
      if(u == host()) {
        return true;
      }
      socket.send(net::make_package(u, data));
      return true;
    });
  }

  LobbyActor::State last_state = LobbyActor::State::DEFAULT;
  void trigger_events() {
    if(last_state != LobbyActor::State::QUIT && has_quit()) {
      last_state = LobbyActor::State::QUIT;
      action_unhost();
    } else if(last_state != LobbyActor::State::STARTED && has_started()) {
      last_state = LobbyActor::State::STARTED;
      action_gstart();
    }
  }

  void action_unhost() {
    Logger::Info("%.2f lserver: sending unhost action to clients\n", Timer::system_time());
    send_action((pkg::lobby_hello_struct){
      .action = pkg::LobbyAction::UNHOST
    });
    Logger::Info("%.2f lserver: sending unhost action to metaservers\n", Timer::system_time());
    {
      std::lock_guard<std::recursive_mutex> guard(mservers_mtx);
      for(auto &m : metaservers) {
        socket.send(net::make_package(m, (pkg::metaserver_host_struct){
          .action = pkg::MSAction::UNHOST
        }));
      }
    }
  }

  void action_gstart() {
    Logger::Info("%.2f lserver: sending start action to clients\n", Timer::system_time());
    lobby.iterate([&](auto &p) mutable {
      const auto &u = p.first;
      if(u == host()) {
        return true;
      }
      socket.send(net::make_package(u, (pkg::lobby_start_struct){
        .action = pkg::LobbyAction::START,
        .index = int8_t(lobby[u].ind),
        .team1 = int8_t(lobby.team1()),
        .team2 = int8_t(lobby.team2())
      }));
      return true;
    });
  }

  void action_activity(net::Addr addr) {
    user_timer.set_time(Timer::system_time());
    user_timer.set_event(Timer::key_t(addr.ip));
  }

  void action_join(net::Addr addr) {
    Timer::time_t server_time = Timer::system_time();
    Logger::Info("%.2f lserver: sending action join for %s to clients\n", server_time, addr.to_str().c_str());
    lobby.add_participant(addr);
    send_action((pkg::lobby_query_response_struct){
      .addr = addr,
      .active = true,
      .info = lobby[addr]
    });
    user_timer.set_time(Timer::system_time());
    user_timer.set_event(Timer::key_t(addr.ip));
    user_timer.set_timeout(Timer::key_t(addr.ip), Timer::time_t(3.));
  }

  void action_kick(net::Addr addr) {
    Timer::time_t server_time = Timer::system_time();
    Logger::Info("%.2f lserver: sending action kick for %s to clients\n", server_time, addr.to_str().c_str());
    lobby.remove_participant(addr);
    send_action((pkg::lobby_query_response_struct){
      .addr = addr,
      .active = false,
    });
    user_timer.erase(Timer::key_t(addr.ip));
  }

  Intelligence<IntelligenceType::ABSTRACT> *make_intelligence(Soccer &soccer) {
    std::set<net::Addr> clients;
    lobby.iterate([&](const auto &p) mutable {
      const auto &u = p.first;
      if(p.second.itype == IntelligenceType::REMOTE) {
        clients.insert(u);
      }
      return true;
    });
    return new SoccerServer(lobby[host()].ind, soccer, socket, clients);
  }
};

struct LobbyClient : LobbyActor {
  net::Addr host;
  net::Socket<net::SocketType::UDP> &socket;
  std::thread client_thread;
  std::recursive_mutex finalize_mtx;

  struct GameMaker {
    int ind;
    int team1;
    int team2;
  } gameMaker;
  std::recursive_mutex gmaker_mtx;

  // connect at construction
  LobbyClient(net::Socket<net::SocketType::UDP> &socket, net::Addr host):
    LobbyActor(),
    host(host),
    socket(socket)
  {
    set_timer();
  }

  template <typename DataT>
  void send_action(const DataT hello) {
    socket.send(net::make_package(host, hello));
  }

  Timer timer;
  static constexpr Timer::key_t EVENT_SEND_HELLO = 1;
  static constexpr Timer::key_t EVENT_HOST_ACTIVITY = 2;
  void set_timer() {
    timer.set_timeout(EVENT_SEND_HELLO, Timer::time_t(1.));
    timer.set_timeout(EVENT_HOST_ACTIVITY, Timer::time_t(3.));
  }

  void register_host_activity() {
    timer.set_time(Timer::system_time());
    timer.set_event(EVENT_HOST_ACTIVITY);
  }

  static void run(LobbyClient *client) {
    client->timer.set_time(Timer::system_time());
    client->timer.set_event(EVENT_HOST_ACTIVITY);
    client->socket.listen(
      [&]() mutable {
        client->trigger_events();
        if(client->has_started() || client->has_quit()) {
          return !client->should_stop();
        }
        client->timer.set_time(Timer::system_time());
        client->timer.periodic(EVENT_SEND_HELLO, [&]() mutable {
          if(rand() % 3 || client->lobby.empty()) {
            Logger::Info("%.2f lclient: sending hello\n", client->timer.current_time);
            client->send_action((pkg::lobby_hello_struct){
              .action = pkg::LobbyAction::NOTHING
            });
          } else {
            Logger::Info("%.2f lclient: sending query\n", client->timer.current_time);
            client->send_action((pkg::lobby_query_struct){
              .action = pkg::LobbyAction::QUERY,
              .addr = client->lobby.random()
            });
          }
        });
        if(client->timer.timed_out(EVENT_HOST_ACTIVITY) && !client->has_quit()) {
          Logger::Info("%.2f lclient: host timed out (%.2fs)\n", client->timer.current_time, client->timer.elapsed(EVENT_HOST_ACTIVITY));
          client->action_leave();
        }
        return !client->should_stop();
      },
      [&](const net::Blob &blob) mutable {
        if(client->has_started() || client->has_quit() || blob.addr != client->host) {
          return !client->should_stop();
        }
        static_assert(net::Typecheck::all_distinct<
          pkg::lobby_hello_struct,
          pkg::lobby_query_response_struct,
          pkg::lobby_start_struct
        >);
        client->register_host_activity();
        // received idle ping from host
        blob.try_visit_as<pkg::lobby_hello_struct>([&](const auto hello) mutable {
          if(hello.action == pkg::LobbyAction::UNHOST) {
            Logger::Info("%.2f lclient: received UNHOST\n", client->timer.current_time);
            client->action_leave();
            return;
          } else {
            Logger::Info("%.2f lclient: received ping\n", client->timer.current_time);
          }
        });
        // received lobby query response
        blob.try_visit_as<pkg::lobby_query_response_struct>([&](const auto qresp) mutable {
          Logger::Info("%.2f lclient: received query response for (%hhd, %d, %s):\n", client->timer.current_time, qresp.info.ind, qresp.info.team?1:0, qresp.addr.to_str().c_str());
          if(qresp.active) {
            client->lobby[qresp.addr] = qresp.info;
          } else if(client->lobby.find(qresp.addr)) {
            client->lobby.remove_participant(qresp.addr);
          }
        });
        // received lobby start
        blob.try_visit_as<pkg::lobby_start_struct>([&](const auto start) mutable {
          Logger::Info("%.2f lclient: received start package from server\n", client->timer.current_time);
          {
            std::lock_guard<std::recursive_mutex> guard(client->gmaker_mtx);
            client->gameMaker.ind = start.index;
            client->gameMaker.team1 = start.team1;
            client->gameMaker.team2 = start.team2;
          }
          client->action_start();
        });
        return !client->should_stop();
      }
    );
  }

  bool finalize = true;
  void start() {
    ASSERT(should_stop());
    Logger::Info("lclient: started\n");
    finalize = false;
    socket.send(net::make_package(host, (pkg::lobby_hello_struct) {
      .action = pkg::LobbyAction::CONNECT
    }));
    client_thread = std::thread(LobbyClient::run, this);
  }
  void stop() {
    ASSERT(!should_stop());
    if(state() == LobbyActor::State::DEFAULT) {
      action_quit();
    }
    {
      std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
      finalize = true;
    }
    client_thread.join();
    Logger::Info("lclient: finished\n");
  }
  bool should_stop() {
    std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
    return finalize;
  }

  bool is_client() {
    return true;
  }

  LobbyActor::State last_state = LobbyActor::State::DEFAULT;
  void trigger_events() {
    if(last_state != LobbyActor::State::QUIT && has_quit()) {
      Logger::Info("lclient: triggered quit\n");
      last_state = LobbyActor::State::QUIT;
      action_quit();
    } else if(last_state != LobbyActor::State::STARTED && has_started()) {
      Logger::Info("lclient: triggered start\n");
      last_state = LobbyActor::State::STARTED;
    }
  }

  void action_quit() {
    Logger::Info("lclient: sending action quit\n");
    send_action((pkg::lobby_hello_struct){
      .action = pkg::LobbyAction::DISCONNECT
    });
    action_leave();
  }

  Soccer get_soccer() {
    std::lock_guard<std::recursive_mutex> guard(gmaker_mtx);
    return Soccer(gameMaker.team1, gameMaker.team2);
  }

  Intelligence<IntelligenceType::ABSTRACT> *make_intelligence(Soccer &soccer) {
    std::lock_guard<std::recursive_mutex> guard(gmaker_mtx);
    return new SoccerRemote(gameMaker.ind, soccer, socket, host);
  }
};
