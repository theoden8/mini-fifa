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
  struct metaserver_query_struct {
    MSAction action;
    net::Addr addr;
  };

  struct metaserver_query_response_struct {
    net::Addr addr;
    int8_t active;
  };

  struct metaserver_host_response_struct {
    MSAction action;
    net::Addr host;
    int8_t you = false;
    char name[30] = "";

    void set_name(std::string &s) {
      strncpy(name, s.c_str(), std::min<int>(s.length() + 1, 30));
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

  bool find(net::Addr host) {
    return games.find(host) != std::end(games);
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
    constexpr Timer::key_t EVENT_CHECK_STATUSES = 1;
    timer.set_timeout(EVENT_CHECK_STATUSES, Timer::time_t(3.));
    Logger::Info("mserver: started at port %hu\n", socket.port());
    socket.listen(
      [&]() mutable {
        timer.set_time(Timer::system_time());
        // clean up inactive users
        timer.periodic(EVENT_CHECK_STATUSES, [&]() mutable {
          Logger::Info("mserver: cleaning up inactive users\n");
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
            if(gamelist.find(u)) {
              unregister_host(u);
            }
            users.erase(u);
            user_timer.erase(Timer::key_t(u.ip));
          }
          Logger::Info("mserver: users [ %s]\n", s.c_str());
        });
        return !feof(stdin);
      },
      [&](const net::Blob &blob) mutable {
        Logger::Info("mserver: received package from %s\n", blob.addr.to_str().c_str());
        // find out if the user already exists
        bool found = users.find(blob.addr) != std::end(users);
        user_timer.set_time(Timer::system_time());
        if(found) {
          user_timer.set_event(Timer::key_t(blob.addr.ip));
        }
        static_assert(sizeof(pkg::metaserver_hello_struct) != sizeof(pkg::metaserver_host_struct));
        // received hello package
        blob.try_visit_as<pkg::metaserver_hello_struct>([&](const auto hello) mutable {
          Logger::Info("mserver: recognized as hello package, found=%d\n", found);
          if(!found) {
            // add user
            Logger::Info("mserver: added user %s\n", blob.addr.to_str().c_str());
            user_timer.set_event(Timer::key_t(blob.addr.ip));
            user_timer.set_timeout(Timer::key_t(blob.addr.ip), Timer::time_t(3.));
            users.insert(blob.addr);
          } else if(hello.action == pkg::MSAction::QUERY) {
            // send random game information
            if(!gamelist.games.empty()) {
              int i = 0; int j = rand() % gamelist.games.size();
              for(auto &it : gamelist.games) {
                if(i == j) {
                  pkg::metaserver_host_response_struct gameinfo = {
                    .action = pkg::MSAction::HOST,
                    .host = it.first,
                    .you = false,
                  };
                  gameinfo.set_name(it.second);
                  socket.send(net::make_package(blob.addr, gameinfo));
                  Logger::Info("mserver: randomly sending host info on %s\n", blob.addr.to_str().c_str());
                  break;
                }
                ++i;
              }
            }
          }
        });
        static_assert(sizeof(pkg::metaserver_hello_struct) != sizeof(pkg::metaserver_query_struct));
        // respond whether the address is active or not
        blob.try_visit_as<pkg::metaserver_query_struct>([&](auto query) mutable {
          Logger::Info("mserver: recognized as query package, found=%d\n");
          if(!found) {
            return;
          }
          ASSERT(query.action == pkg::MSAction::QUERY);
          socket.send(net::make_package(blob.addr, (pkg::metaserver_query_response_struct){
            .addr = query.addr,
            .active = gamelist.find(query.addr)
          }));
        });
        static_assert(sizeof(pkg::metaserver_query_struct) != sizeof(pkg::metaserver_host_struct));
        // received hosting action
        blob.try_visit_as<pkg::metaserver_host_struct>([&](auto host) mutable {
          Logger::Info("mserver: recognized as hosting struct\n");
          switch(host.action) {
            case pkg::MSAction::HELLO:break;
            case pkg::MSAction::QUERY:break;
            case pkg::MSAction::HOST:
              if(found) {
                host.name[29] = '\0';
                Logger::Info("mserver: hosting game name='%s'\n", host.name);
                register_host(blob.addr, host.name);
                pkg::metaserver_host_response_struct response = {
                  .action = pkg::MSAction::HOST,
                  .host = blob.addr
                };
                std::string name = host.name;
                response.set_name(name);
                Logger::Info("mserver: sending action host host=%s name=%s\n", blob.addr.to_str().c_str(), name.c_str());
                for(auto &u : users) {
                  response.you = (blob.addr == u);
                  socket.send(net::make_package(u, response));
                }
              }
            break;
            case pkg::MSAction::UNHOST:
              if(found) {
                host.name[29] = '\0';
                Logger::Info("mserver: unhosting game\n");
                unregister_host(blob.addr);
                Logger::Info("mserver: sending action unhost host=%s\n", blob.addr.to_str().c_str());
                for(auto &u : users) {
                  socket.send(net::make_package(u, (pkg::metaserver_host_response_struct){
                    .action = pkg::MSAction::UNHOST,
                    .host = blob.addr,
                    .you = (blob.addr == u)
                  }));
                }
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
    if(!gamelist.find(host) || gamelist.games.at(host) != name) {
      gamelist.add_game(host, name);
    }
  }

  // parent lock
  void unregister_host(net::Addr host) {
    if(gamelist.find(host)) {
      gamelist.delete_game(host);
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
  std::recursive_mutex mservers_mtx;
  std::recursive_mutex lmaker_mtx;
  std::recursive_mutex finalize_mtx;

  MetaServerClient(std::set<net::Addr> metaservers, net::port_t port=5679):
    socket(port),
    metaservers(metaservers)
  {
    set_timer();
  }

  Timer timer;
  static constexpr Timer::key_t EVENT_SEND_HELLO = 1;
  static constexpr Timer::key_t EVENT_SEND_QUERY = 2;
  void set_timer() {
    timer.set_timeout(EVENT_SEND_HELLO, Timer::time_t(1.));
    timer.set_timeout(EVENT_SEND_QUERY, Timer::time_t(2.));
  }

  static void run(MetaServerClient *client) {
    client->socket.listen(
      [&]() mutable {
        if(client->has_quit() || client->has_hosted()) {
          return !client->should_stop();
        }
        /* usleep(1e6 / 24.); */
        client->timer.set_time(Timer::system_time());
        // send hello to the metaserver every second
        client->timer.periodic(EVENT_SEND_HELLO, [&]() mutable {
          if(rand() % 3) {
            Logger::Info("mclient: sending hello\n");
            client->send_action((pkg::metaserver_hello_struct){
              .action = pkg::MSAction::HELLO
            });
          } else {
            Logger::Info("mclient: sending query\n");
            client->send_action((pkg::metaserver_hello_struct){
              .action = pkg::MSAction::QUERY
            });
          }
        });
        // send query for random host
        /* client->timer.periodic(EVENT_SEND_QUERY, [&]() mutable { */
        /*   int i = 0, j = rand() % client->gamelists.size(); */
        /*   for(auto &e:client->gamelists) { */
        /*     if(i == j) { */
        /*       int k = 0, m = rand() % e.second.games.size(); */
        /*       for(auto &e2:e.second.games) { */
        /*         if(k == m) { */
        /*           auto &addr = e2.first; */
        /*           Logger::Info("mclient: sending query for host %s\n", addr.to_str().c_str()); */
        /*           client->send_action((pkg::metaserver_query_struct){ */
        /*             .action = pkg::MSAction::QUERY, */
        /*             .addr = addr */
        /*           }); */
        /*           break; */
        /*         } */
        /*         ++k; */
        /*       } */
        /*       break; */
        /*     } */
        /*     ++i; */
        /*   } */
        /* }); */
        return !client->should_stop();
      },
      [&](const net::Blob &blob) {
        if(client->has_quit() || client->has_hosted()) {
          return !client->should_stop();
        }
        {
          std::lock_guard<std::recursive_mutex> guard(client->mservers_mtx);
          if(client->metaservers.find(blob.addr) == std::end(client->metaservers)) {
            return !client->should_stop();
          }
        }
        static_assert(sizeof(pkg::metaserver_query_response_struct) != sizeof(pkg::metaserver_host_response_struct));
        // recognize as a query response struct
        blob.try_visit_as<pkg::metaserver_query_response_struct>([&](const auto response) mutable {
          // unregister if no longer marked active
          Logger::Info("mclient: received query response for %s\n", response.addr.to_str().c_str());
          if(client->gamelists[blob.addr].find(response.addr) && !response.active) {
            client->unregister_host(blob.addr, response.addr);
          }
        });
        // recognize as a hosting respond struct
        blob.try_visit_as<pkg::metaserver_host_response_struct>([&](const auto response) mutable {
          switch(response.action) {
            case pkg::MSAction::HELLO:break;
            case pkg::MSAction::HOST:
              Logger::Info("mclient: register game host=%s name=%s\n", blob.addr.to_str().c_str(), response.name);
              client->register_host(blob.addr, response.host, response.name);
              if(response.you) {
                client->set_state(State::HOSTED);
                std::lock_guard<std::recursive_mutex> guard(client->lmaker_mtx);
                client->lobbyMaker = (LobbyMaker){
                  .ltype = LobbyMaker::type::SERVER
                };
              }
            break;
            case pkg::MSAction::UNHOST:
              Logger::Info("mclient: unregister game host=%s\n", blob.addr.to_str().c_str());
              client->unregister_host(blob.addr, response.host);
            break;
          }
        });
        return !client->should_stop();
      }
    );
  }

  std::recursive_mutex state_mtx;
  enum class State {
    DEFAULT,
    HOSTED,
    JOINED,
    QUIT
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
    gamelists[metaserver].add_game(host, gamename);
  }

  void unregister_host(net::Addr metaserver, net::Addr host) {
    gamelists[metaserver].delete_game(host);
  }

  void action_host(std::string gamename) {
    pkg::metaserver_host_struct data = {
      .action = pkg::MSAction::HOST
    };
    data.set_name(gamename);
    Logger::Info("mclient: sending action host name='%s'\n", data.name);
    send_action(data);
  }

  void action_join(net::Addr host) {
    Logger::Info("sending action join game host=%s\n", host.to_str().c_str());
    set_state(State::JOINED);
    std::lock_guard<std::recursive_mutex> guard(lmaker_mtx);
    lobbyMaker = (LobbyMaker){
      .ltype = LobbyMaker::type::CLIENT,
      .host = host
    };
  }

  void add_metaserver(net::Addr addr) {
    std::lock_guard<std::recursive_mutex> guard(mservers_mtx);
    metaservers.insert(addr);
  }

  template <typename DataT>
  void send_action(DataT data) {
    std::lock_guard<std::recursive_mutex> mguard(mservers_mtx);
    for(auto &m : metaservers) {
      socket.send(net::make_package(m, data));
    }
  }

  LobbyActor *make_lobby() {
    std::lock_guard<std::recursive_mutex> lmguard(lmaker_mtx);
    if(lobbyMaker.ltype == LobbyMaker::type::SERVER) {
      return new LobbyServer(socket, metaservers, mservers_mtx);
    } else if(lobbyMaker.ltype == LobbyMaker::type::CLIENT) {
      return new LobbyClient(socket, lobbyMaker.host);
    }
  }

  void start() {
    gamelists.clear();
    set_state(State::DEFAULT);
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
    std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
    return finalize;
  }

  void stop() {
    ASSERT(!should_stop());
    {
      std::lock_guard<std::recursive_mutex> guard(finalize_mtx);
      finalize = true;
    }
    user_thread.join();
    Logger::Info("mclient: finished\n");
  }

  ~MetaServerClient()
  {}
};
