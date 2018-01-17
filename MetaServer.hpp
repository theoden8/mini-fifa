#pragma once

#include "Timer.hpp"
#include "Network.hpp"
#include "Lobby.hpp"

#include <cstring>

#include <thread>
#include <mutex>
#include <set>
#include <vector>
#include <string>

namespace pkg {
  enum class MetaServerAction {
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

  MetaServer(net::port_t port=5678):
    gamelist(),
    socket(port)
  {}

  void run() {
    std::mutex mtx;
    socket.listen(mtx, [&](const net::Blob &blob) {
      blob.try_visit_as<pkg::metaserver_hello_struct>([&](auto hello) {
        switch(hello.action) {
          case pkg::MetaServerAction::HELLO:
            if(users.find(blob.addr) != std::end(users)) {
              Logger::Info("added user %s\n", blob.addr.to_str().c_str());
              users.insert(blob.addr);
            }
          break;
          case pkg::MetaServerAction::HOST_GAME:
            hello.name[29] = '\0';
            register_host(blob.addr, hello.name);
          break;
          case pkg::MetaServerAction::UNHOST_GAME:
            hello.name[29] = '\0';
            unregister_host(blob.addr);
          break;
        }
      });
      return true;
    });
  }

  void register_host(net::Addr host, std::string name) {
    ASSERT(name.length() < 80);
    gamelist.add_game(host, name);
    pkg::metaserver_response_struct response = {
      .action = pkg::MetaServerAction::HOST_GAME,
      .host = host
    };
    strcpy(response.name, name.c_str());
    for(auto &addr : users) {
      net::Package<pkg::metaserver_response_struct> package(addr, response);
      socket.send(package);
    }
  }

  void unregister_host(net::Addr host) {
    gamelist.delete_game(host);
    pkg::metaserver_response_struct response = {
      .action = pkg::MetaServerAction::UNHOST_GAME,
      .host = host
    };
    for(auto &addr : users) {
      net::Package<pkg::metaserver_response_struct> package(addr, response);
      socket.send(package);
    }
  }
};

struct MetaServerClient {
  GameList gamelist;
  net::Addr metaserver;
  net::Socket<net::SocketType::UDP> socket;

  Timer timer;
  static constexpr int SEND_HELLO = 1;

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
  {
    timer.set_time(.0);
    timer.set_timeout(SEND_HELLO, .5);
  }

  static void run(MetaServerClient *client) {
    constexpr int EVENT_REFRESH = 1;
    client->timer.set_timeout(EVENT_REFRESH, .4);
    client->socket.listen(client->socket_mtx, [&]() {
      auto t = Timer::system_time();
      client->timer.set_time(t);
      if(client->timer.timed_out(EVENT_REFRESH)) {
        client->timer.set_event(EVENT_REFRESH);
        std::lock_guard<std::mutex> guard(client->socket_mtx);
        printf("sending hello to metaserver\n");
        client->send_action((pkg::metaserver_hello_struct){
          .action = pkg::MetaServerAction::HELLO
        });
      }
    },
    [&](const net::Blob &blob) {
      if(blob.addr != client->metaserver) {
        return client->should_stop();
      }
      blob.try_visit_as<pkg::metaserver_response_struct>([&](const auto &response) {
        switch(response.action) {
          case pkg::MetaServerAction::HOST_GAME:client->register_host(response.host, response.name);break;
          case pkg::MetaServerAction::UNHOST_GAME:client->unregister_host(response.host);break;
          case pkg::MetaServerAction::HELLO:break;
        }
      });
      return client->should_stop();
    });
  }

  void register_host(net::Addr host, std::string name) {
    ASSERT(name.length() < 80);
    gamelist.add_game(host, name);
  }

  void unregister_host(net::Addr host) {
    gamelist.delete_game(host);
  }

  void action_host(std::string gamename) {
    char name[30];
    pkg::metaserver_hello_struct data = {
      .action = pkg::MetaServerAction::HOST_GAME
    };
    for(int i = 0; i < std::min<int>(gamename.length(), 30); ++i)data.name[i]=gamename[i];
    data.name[29] = '\0';
    send_action(data);
  }

  void action_unhost() {
    pkg::metaserver_hello_struct data = {
      .action = pkg::MetaServerAction::UNHOST_GAME
    };
    send_action(data);
  }

  void send_action(pkg::metaserver_hello_struct hello) {
    std::lock_guard<std::mutex> guard(socket_mtx);
    net::Package<pkg::metaserver_hello_struct> package(metaserver, hello);
    socket.send(package);
  }

  LobbyActor *make_lobby() {
    // not implemented
    return nullptr;
  }

  void start() {
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
    std::lock_guard<std::mutex> guard(finalize_mtx);
    finalize = true;
    user_thread.join();
  }
};
