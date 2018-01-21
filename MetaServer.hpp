#pragma once

#include "Timer.hpp"
#include "Network.hpp"
#include "Lobby.hpp"

#include <cstdint>
#include <cstring>

#include <thread>
#include <mutex>
#include <set>
#include <unordered_set>
#include <vector>
#include <string>

namespace pkg {
  enum class MSAction : int8_t {
    HELLO,
    HOST_GAME,
    UNHOST_GAME
  };

  struct metaserver_hello_struct {
    MSAction action = MSAction::HELLO;
  };

  struct metaserver_host_struct {
    MSAction action;
    char name[30] = "";

    void set_name(std::string &s) {
      memcpy(name, s.c_str(), std::min<int>(s.length() + 1, 30));
      name[29] = '\0';
    }
  };

  struct metaserver_response_struct {
    MSAction action;
    net::Addr host;
    int8_t you = false;
    char name[30] = "";

    void set_name(std::string &s) {
      memcpy(name, s.c_str(), std::min<int>(s.length() + 1, 30));
      name[29] = '\0';
    }
  };
}

struct GameList {
  std::map<net::Addr, std::string> games;

  void add_game(net::Addr host, std::string name) {
    games[host] = name;
  }

  void delete_game(net::Addr host) {
    games.erase(host);
  }
};

struct MetaServer {
  GameList gamelist;
  net::Socket<net::SocketType::UDP> socket;

  Timer timer;

  std::set<net::Addr> users;
  Timer user_timer;

  MetaServer(net::port_t port=5678):
    gamelist(),
    socket(port)
  {}

  void run() {
    std::mutex mtx;
    constexpr Timer::key_t CHECK_STATUSES = 1;
    timer.set_timeout(CHECK_STATUSES, 5.);
    Logger::Info("mserver: started at port %hu\n", socket.port());
    socket.listen(mtx,
      [&]() {
        timer.set_time(Timer::system_time());
        if(timer.timed_out(CHECK_STATUSES)) {
          Logger::Info("mserver: cleaning up inactive users\n");
          timer.set_event(CHECK_STATUSES);
          user_timer.set_time(Timer::system_time());
          std::string s = "";
          // workaround because it fails when the iterated set changes
          std::set<net::Addr> exusers;
          for(auto &u : users) {
            if(user_timer.timed_out(Timer::key_t(u.ip))) {
              Logger::Info("mserver: removing user %s\n", u.to_str().c_str());
              exusers.insert(u);
            } else {
              s += u.to_str() + " ";
            }
          }
          for(auto &u : exusers) {
            users.erase(u);
            user_timer.erase(Timer::key_t(u.ip));
          }
          Logger::Info("mserver: users [ %s]\n", s.c_str());
        }
        return !feof(stdin);
      },
      [&](const net::Blob &blob) mutable {
        Logger::Info("mserver: received package from %s\n", blob.addr.to_str().c_str());
        bool found = users.find(blob.addr) != std::end(users);
        user_timer.set_time(Timer::system_time());
        if(found) {
          user_timer.set_event(Timer::key_t(blob.addr.ip));
        }
        static_assert(sizeof(pkg::metaserver_hello_struct) != sizeof(pkg::metaserver_host_struct));
        // received hello package
        blob.try_visit_as<pkg::metaserver_hello_struct>([&](auto hello) mutable {
          Logger::Info("mserver: recognized as hello package, found=%d\n", found);
          ASSERT(hello.action == pkg::MSAction::HELLO);
          if(!found) {
            // add user
            Logger::Info("mserver: added user %s\n", blob.addr.to_str().c_str());
            user_timer.set_event(Timer::key_t(blob.addr.ip));
            user_timer.set_timeout(Timer::key_t(blob.addr.ip), 2.);
            users.insert(blob.addr);
          }
        });
        // received hosting action
        blob.try_visit_as<pkg::metaserver_host_struct>([&](auto host) mutable {
          Logger::Info("mserver: recognized as hosting struct\n");
          switch(host.action) {
            case pkg::MSAction::HELLO:break;
            case pkg::MSAction::HOST_GAME:
              if(found) {
                host.name[29] = '\0';
                Logger::Info("mserver: hosting game name='%s'\n", host.name);
                register_host(blob.addr, host.name);
              }
            break;
            case pkg::MSAction::UNHOST_GAME:
              if(found) {
                host.name[29] = '\0';
                Logger::Info("mserver: unhosting game\n");
                unregister_host(blob.addr);
              }
            break;
          }
        });
        return !feof(stdin);
    });
    Logger::Info("mserver: finisned\n");
  }

  // parent lock
  void register_host(net::Addr host, std::string name) {
    ASSERT(name.length() < 30);
    if(gamelist.games.find(host) == std::end(gamelist.games) || gamelist.games.at(host) != name) {
      gamelist.add_game(host, name);
    }
    pkg::metaserver_response_struct response = {
      .action = pkg::MSAction::HOST_GAME,
      .host = host
    };
    response.set_name(name);
    Logger::Info("mserver: sending action host host=%s name=%s\n", host.to_str().c_str(), name.c_str());
    for(auto &addr : users) {
      response.you = (host == addr);
      socket.send(net::make_package(addr, response));
    }
  }

  // parent lock
  void unregister_host(net::Addr host) {
    gamelist.delete_game(host);
    Logger::Info("mserver: sending action unhost host=%s\n", host.to_str().c_str());
    for(auto &addr : users) {
      socket.send(net::make_package(addr, (pkg::metaserver_response_struct){
        .action = pkg::MSAction::UNHOST_GAME,
        .host = host,
        .you = (host == addr)
      }));
    }
  }
};

struct MetaServerClient {
  std::set<net::Addr> metaservers;
  std::map<net::Addr, GameList> gamelists;
  net::Socket<net::SocketType::UDP> socket;

  struct LobbyMaker {
    enum class type : int8_t { SERVER, CLIENT };
    type ltype = type::SERVER;
    net::Addr host;
  } lobbyMaker;

  std::thread user_thread;
  std::mutex mservers_mtx;
  std::mutex socket_mtx;
  std::mutex lmaker_mtx;
  std::mutex finalize_mtx;

  MetaServerClient(std::set<net::Addr> metaservers, net::port_t port=5679):
    socket(port),
    metaservers(metaservers)
  {}

  static void run(MetaServerClient *client) {
    Timer timer;
    constexpr int EVENT_REFRESH = 1;
    timer.set_timeout(EVENT_REFRESH, Timer::time_t(1.));
    client->socket.listen(client->socket_mtx,
      [&]() mutable {
        usleep(1e6 / 24.);
        timer.set_time(Timer::system_time());
        if(timer.timed_out(EVENT_REFRESH)) {
          timer.set_event(EVENT_REFRESH);
          Logger::Info("mclient: sending hello\n");
          client->send_action((pkg::metaserver_hello_struct){
            .action = pkg::MSAction::HELLO
          });
        }
        return !client->should_stop();
      },
      [&](const net::Blob &blob) {
        {
          std::lock_guard<std::mutex> guard(client->mservers_mtx);
          if(client->metaservers.find(blob.addr) == std::end(client->metaservers)) {
            return !client->should_stop();
          }
        }
        blob.try_visit_as<pkg::metaserver_response_struct>([&](const auto &response) mutable {
          switch(response.action) {
            case pkg::MSAction::HELLO:break;
            case pkg::MSAction::HOST_GAME:
              client->register_host(blob.addr, response.host, response.name);
              if(response.you) {
                client->set_state(State::HOSTED);
                std::lock_guard<std::mutex> guard(client->lmaker_mtx);
                client->lobbyMaker = (LobbyMaker){
                  .ltype = LobbyMaker::type::SERVER
                };
              }
            break;
            case pkg::MSAction::UNHOST_GAME:
              client->unregister_host(blob.addr, response.host);
            break;
          }
        });
        return !client->should_stop();
      }
    );
  }

  std::mutex state_mtx;
  enum class State {
    DEFAULT,
    HOSTED,
    JOINED,
    QUIT
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
  bool has_hosted() {
    return state() == State::HOSTED;
  }
  bool has_quit() {
    return state() == State::QUIT;
  }
  bool has_joined() {
    return state() == State::JOINED;
  }

  void register_host(net::Addr metaserver, net::Addr host, std::string gamename) {
    ASSERT(gamename.length() < 30);
    Logger::Info("mclient: register game host=%s name=%s\n", host.to_str().c_str(), gamename.c_str());
    gamelists[metaserver].add_game(host, gamename);
  }

  void unregister_host(net::Addr metaserver, net::Addr host) {
    Logger::Info("mclient: unregister game host=%s\n", host.to_str().c_str());
    gamelists[metaserver].delete_game(host);
  }

  void action_host(std::string gamename) {
    char name[30];
    pkg::metaserver_host_struct data = {
      .action = pkg::MSAction::HOST_GAME
    };
    data.set_name(gamename);
    Logger::Info("mclient: sending action host name='%s'\n", data.name);
    send_action(data);
  }

  void action_join(net::Addr host) {
    Logger::Info("sending action join game host=%s\n", host.to_str().c_str());
    set_state(State::JOINED);
    std::lock_guard<std::mutex> guard(lmaker_mtx);
    lobbyMaker = (LobbyMaker){
      .ltype = LobbyMaker::type::CLIENT,
      .host = host
    };
  }

  void action_unhost() {
    Logger::Info("mclient: sending action unhost game\n");
    set_state(State::DEFAULT);
    send_action((pkg::metaserver_host_struct){
      .action = pkg::MSAction::UNHOST_GAME
    });
  }

  void add_metaserver(net::Addr addr) {
    std::lock_guard<std::mutex> guard(mservers_mtx);
    metaservers.insert(addr);
  }

  template <typename DataT>
  void send_action(DataT data) {
    std::lock_guard<std::mutex> guard(socket_mtx);
    std::lock_guard<std::mutex> mguard(mservers_mtx);
    for(auto &m : metaservers) {
      socket.send(net::make_package(m, data));
    }
  }

  LobbyActor *make_lobby() {
    std::lock_guard<std::mutex> guard(socket_mtx);
    std::lock_guard<std::mutex> lmguard(lmaker_mtx);
    if(lobbyMaker.ltype == LobbyMaker::type::SERVER) {
      return new LobbyServer(socket);
    } else if(lobbyMaker.ltype == LobbyMaker::type::CLIENT) {
      return new LobbyClient(socket, lobbyMaker.host);
    }
  }

  void start() {
    gamelists.clear();
    std::string s = "[ ";
    for(auto &m : metaservers) {
      s += m.to_str() + " ";
    }
    s += "]";
    Logger::Info("mclient: started, mserver=%s\n", s.c_str());
    ASSERT(should_stop());
    finalize = false;
    user_thread = std::thread(MetaServerClient::run, this);
  }

  bool finalize = true;
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  void stop() {
    ASSERT(!should_stop());
    {
      std::lock_guard<std::mutex> guard(finalize_mtx);
      finalize = true;
    }
    user_thread.join();
    Logger::Info("mclient: finished\n");
  }

  ~MetaServerClient()
  {}
};
