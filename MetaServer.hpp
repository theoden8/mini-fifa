#pragma once

#include "Timer.hpp"
#include "Network.hpp"
#include "Lobby.hpp"

#include <cstdint>
#include <cstring>

#include <thread>
#include <mutex>
#include <set>
#include <vector>
#include <string>

namespace pkg {
  enum class MetaServerAction : int8_t {
    HELLO,
    HOST_GAME,
    UNHOST_GAME
  };

  struct metaserver_hello_struct {
    MetaServerAction action;
    char name[30];
  };

  struct metaserver_response_struct {
    MetaServerAction action;
    net::Addr host;
    bool you = false;
    char name[30];
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
          std::string s = "";
          for(auto &u : users) {
            if(user_timer.timed_out(Timer::key_t(u.ip))) {
              Logger::Info("mserver: removing user %s\n", u.to_str().c_str());
              users.erase(u);
              user_timer.erase(Timer::key_t(u.ip));
            } else {
              s += u.to_str() + " ";
            }
          }
          Logger::Info("mserver: users [ %s]\n", s.c_str());
        }
        return !feof(stdin);
      },
      [&](const net::Blob &blob) mutable {
        Logger::Info("mserver: received package from %s\n", blob.addr.to_str().c_str());
        blob.try_visit_as<pkg::metaserver_hello_struct>([&](auto hello) mutable {
          bool found = users.find(blob.addr) != std::end(users);
          Logger::Info("mserver: recognized as hello package, found=%d\n", found);
          user_timer.set_time(Timer::system_time());
          if(found) {
            user_timer.set_event(Timer::key_t(blob.addr.ip));
          }
          switch(hello.action) {
            case pkg::MetaServerAction::HELLO:
              if(!found) {
                // add user
                Logger::Info("mserver: added user %s\n", blob.addr.to_str().c_str());
                user_timer.set_event(Timer::key_t(blob.addr.ip));
                user_timer.set_timeout(Timer::key_t(blob.addr.ip), 2.);
                users.insert(blob.addr);
              }
            break;
            case pkg::MetaServerAction::HOST_GAME:
              if(found) {
                hello.name[29] = '\0';
                Logger::Info("mserver: hosting game name='%s'\n", hello.name);
                register_host(blob.addr, hello.name);
              }
            break;
            case pkg::MetaServerAction::UNHOST_GAME:
              if(found) {
                hello.name[29] = '\0';
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

  void register_host(net::Addr host, std::string name) {
    ASSERT(name.length() < 30);
    gamelist.add_game(host, name);
    pkg::metaserver_response_struct response = {
      .action = pkg::MetaServerAction::HOST_GAME,
      .host = host
    };
    strcpy(response.name, name.c_str());
    Logger::Info("mserver: sending action host host=%s name=%s\n", host.to_str().c_str(), name.c_str());
    for(auto &addr : users) {
      response.you = (host == addr);
      net::Package<pkg::metaserver_response_struct> package(addr, response);
      socket.send(package);
    }
  }

  void unregister_host(net::Addr host) {
    gamelist.delete_game(host);
    Logger::Info("mserver: sending action unhost host=%s\n", host.to_str().c_str());
    for(auto &addr : users) {
      socket.send(net::make_package(addr, (pkg::metaserver_response_struct){
        .action = pkg::MetaServerAction::UNHOST_GAME,
        .host = host,
        .you = (host == addr)
      }));
    }
  }
};

struct MetaServerClient {
  GameList gamelist;
  net::Addr metaserver;
  net::Socket<net::SocketType::UDP> socket;

  struct LobbyMaker {
    enum class type { SERVER, CLIENT };
    type ltype = type::SERVER;
    net::Addr host;
  } lobbyMaker;

  std::thread user_thread;
  std::mutex socket_mtx;
  std::mutex finalize_mtx;

  MetaServerClient(net::Addr metaserver, net::port_t port=5679):
    metaserver(metaserver), socket(port)
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
            .action = pkg::MetaServerAction::HELLO
          });
        }
        return !client->should_stop();
      },
      [&](const net::Blob &blob) {
        if(blob.addr != client->metaserver) {
          return !client->should_stop();
        }
        blob.try_visit_as<pkg::metaserver_response_struct>([&](const auto &response) mutable {
          switch(response.action) {
            case pkg::MetaServerAction::HELLO:break;
            case pkg::MetaServerAction::HOST_GAME:
              client->register_host(response.host, response.name);
              if(response.you) {
                std::lock_guard<std::mutex> guard(client->state_mtx);
                client->state_hosted = true;
              }
            break;
            case pkg::MetaServerAction::UNHOST_GAME:
              client->unregister_host(response.host);
            break;
          }
        });
        return !client->should_stop();
      }
    );
  }

  std::mutex state_mtx;
  bool state_hosted = false;
  bool has_hosted() {
    std::lock_guard<std::mutex> guard(state_mtx);
    return state_hosted;
  }

  void register_host(net::Addr host, std::string name) {
    ASSERT(name.length() < 80);
    Logger::Info("mclient: register game host=%s name=%s\n", host.to_str().c_str(), name.c_str());
    gamelist.add_game(host, name);
  }

  void unregister_host(net::Addr host) {
    Logger::Info("mclient: unregister game host=%s\n", host.to_str().c_str());
    gamelist.delete_game(host);
  }

  void action_host(std::string gamename) {
    char name[30];
    pkg::metaserver_hello_struct data = {
      .action = pkg::MetaServerAction::HOST_GAME
    };
    for(int i = 0; i < std::min<int>(gamename.length(), 30); ++i)data.name[i]=gamename[i];
    data.name[29] = '\0';
    Logger::Info("mclient: sending action host name='%s'\n", data.name);
    send_action(data);
  }

  void action_unhost() {
    Logger::Info("mclient: sending action unhost game\n");
    {
      std::lock_guard<std::mutex> guard(state_mtx);
      state_hosted = false;
    }
    send_action((pkg::metaserver_hello_struct){
      .action = pkg::MetaServerAction::UNHOST_GAME
    });
  }

  void send_action(pkg::metaserver_hello_struct hello) {
    std::lock_guard<std::mutex> guard(socket_mtx);
    net::Package<pkg::metaserver_hello_struct> package(metaserver, hello);
    socket.send(package);
  }

  LobbyActor *make_lobby() {
    Lobby *lobby = new Lobby();
    if(lobbyMaker.ltype == LobbyMaker::type::SERVER) {
      return new LobbyServer(*lobby, metaserver);
    } else if(lobbyMaker.ltype == LobbyMaker::type::CLIENT) {
      return new LobbyClient(*lobby, lobbyMaker.host);
    }
  }

  void start() {
    Logger::Info("mclient: started, mserver=%s\n", metaserver.to_str().c_str());
    ASSERT(should_stop());
    finalize = false;
    user_thread = std::thread(
      MetaServerClient::run,
      this
    );
  }

  bool finalize = true;
  bool should_stop() {
    std::lock_guard<std::mutex> guard(finalize_mtx);
    return finalize;
  }

  void stop() {
    {
      std::lock_guard<std::mutex> guard(finalize_mtx);
      finalize = true;
    }
    user_thread.join();
    Logger::Info("mclient: finished\n");
  }

  ~MetaServerClient() {
    if(should_stop()) {
      stop();
    }
  }
};
