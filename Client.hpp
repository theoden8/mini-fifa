#pragma once

#include "MetaServer.hpp"
#include "Lobby.hpp"
#include "Soccer.hpp"
#include "Intelligence.hpp"

struct Client {
  MetaServerClient mclient;
  Lobby *lobby = nullptr;
  LobbyActor *l_actor = nullptr;
  Soccer *soccer = nullptr;
  Intelligence<IntelligenceType::ABSTRACT> *intelligence = nullptr;

  Client(net::Addr metaserver, net::port_t port=5679):
    mclient(metaserver, port)
  {}

  bool is_active_mclient() {
    return !mclient.should_stop();
  }

  bool is_active_lobby() {
    return lobby != nullptr && l_actor != nullptr;
  }

  bool is_active_game() {
    ASSERT(((soccer == nullptr) ? 1 : 0) == ((intelligence == nullptr) ? 1 : 0));
    return soccer != nullptr && intelligence != nullptr;
  }

  void start_mclient() {
    ASSERT(!is_active_mclient());
    mclient.start();
    Logger::Info("started mclient\n");
  }
  void stop_mclient() {
    ASSERT(is_active_mclient());
    mclient.stop();
  }

  void start_lobby() {
    ASSERT(!is_active_lobby());
    l_actor = mclient.make_lobby();
    lobby = &l_actor->lobby;
    l_actor->start();
  }
  void stop_lobby() {
    ASSERT(is_active_lobby());
    l_actor->stop();
    delete l_actor;
    l_actor = nullptr;
    delete lobby;
    lobby = nullptr;
  }

  void start_game() {
    ASSERT(!is_active_game());
    soccer = new Soccer(l_actor->lobby.get_soccer());
    intelligence = l_actor->make_intelligence(*soccer);
  }
  void stop_game() {
    ASSERT(is_active_game());
    intelligence->stop();
    delete intelligence;
    intelligence = nullptr;
    delete soccer;
    soccer = nullptr;
  }

// actions
  void action_host_game() {
    Logger::Info("client: action host game\n");
    stop_mclient();
    start_lobby();
  }

  void action_start_game() {
    Logger::Info("client: action start game\n");
    start_game();
    stop_lobby();
    intelligence->start();
  }

  void action_quit_lobby() {
    Logger::Info("client: action unhost game\n");
    stop_lobby();
    start_mclient();
    mclient.action_unhost();
  }

  void action_quit_game() {
    Logger::Info("client: action leave game\n");
    stop_game();
    start_mclient();
  }

  // scope functions
  void start() {
    start_mclient();
  }
  void stop() {
    if(is_active_game()) {
      stop_game();
    }
    if(is_active_lobby()) {
      stop_lobby();
    }
    if(is_active_mclient()) {
      mclient.stop();
    }
  }
};
