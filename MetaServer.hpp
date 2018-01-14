#pragma once

#include "Timer.hpp"
#include "Network.hpp"

#include <cstring>

#include <thread>
#include <mutex>
#include <set>
#include <vector>
#include <string>

namespace pkg {
  enum class MetaServerAction {
    NO_ACTION,
    HELLO,
    HOST_GAME,
    UNHOST_GAME
  };

  struct metaserver_hello_struct {
    MetaServerAction action = MetaServerAction::NO_ACTION;
    char name[30];
  };

  struct metaserver_response_struct {
    MetaServerAction action = MetaServerAction::NO_ACTION;
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

  std::set<net::Addr> users;

  MetaServer(net::port_t port=5678):
    gamelist(),
    socket(port)
  {}

  void run() {
    while(1) {
      net::Package<pkg::metaserver_hello_struct> package;
      if(socket.receive(package)) {
        if(package.data.action == pkg::MetaServerAction::NO_ACTION)continue;
        switch(package.data.action) {
          case pkg::MetaServerAction::HELLO:
            if(users.find(package.addr) != std::end(users)) {
              users.insert(package.addr);
            }
          break;
          case pkg::MetaServerAction::HOST_GAME:
            package.data.name[29] = '\0';
            register_host(package.addr, package.data.name);
          break;
          case pkg::MetaServerAction::UNHOST_GAME:
            package.data.name[29] = '\0';
            unregister_host(package.addr);
          case pkg::MetaServerAction::NO_ACTION:break;
        }
      }
    }
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
    auto server_time_start = std::chrono::system_clock::now();
    while(!client->should_stop()) {
      auto server_time_now = std::chrono::system_clock::now();
      Timer::time_t server_time = 1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(server_time_now - server_time_start).count();
      client->try_refresh(server_time);
      client->try_receive();
    }
  }

  bool try_refresh(Timer::time_t curtime) {
    timer.set_time(curtime);
    if(timer.timed_out(SEND_HELLO)) {
      timer.set_event(SEND_HELLO);
      pkg::metaserver_hello_struct hello = { .action = pkg::MetaServerAction::HELLO, };
      send_action(hello);
      return true;
    }
    return false;
  }

  bool try_receive() {
    std::lock_guard<std::mutex> guard(socket_mtx);
    net::Package<pkg::metaserver_response_struct> package;
    if(socket.receive(package)) {
      if(package.data.action == pkg::MetaServerAction::NO_ACTION)return false;
      if(package.addr != metaserver)return false;
      switch(package.data.action) {
        case pkg::MetaServerAction::HOST_GAME:register_host(package.data.host, package.data.name);break;
        case pkg::MetaServerAction::UNHOST_GAME:unregister_host(package.data.host);break;
        case pkg::MetaServerAction::HELLO:break;
        case pkg::MetaServerAction::NO_ACTION:break;
      }
      return true;
    }
    return false;
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

  void start() {
    ASSERT(should_stop());
    finalize = true;
    user_thread = std::thread(
      MetaServerClient::run,
      this
    );
  }

  bool finalize = false;
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
