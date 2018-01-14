#include "Window.hpp"

int main() {
  Logger::Setup("minififa.log");
  Logger::MirrorLog(stderr);
  Window w;
  w.run();
  /* Soccer server_soccer; */
  /* Soccer client_soccer; */
  /* net::Addr server_addr(INADDR_LOOPBACK, net::port_t(2345)); */
  /* net::Addr client_addr(INADDR_LOOPBACK, net::port_t(2346)); */
  /* { */
  /*   Server iserver(0, server_soccer, server_addr.port, {client_addr}); */
  /*   Remote iclient(0, client_soccer, client_addr.port, server_addr); */
  /*   Window w(client_soccer, iclient); */
  /*   w.run(iserver); */
  /* } */
  Logger::Close();
}
