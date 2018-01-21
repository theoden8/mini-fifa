#include "Client.hpp"
#include "Window.hpp"

int main(int argc, char *argv[]) {
  /* Soccer server_soccer; */
  /* Soccer client_soccer; */
  /* net::Addr server_addr(net::ip_from_ints(127, 0, 0, 1), net::port_t(2345)); */
  /* net::Addr client_addr(net::ip_from_ints(127, 0, 0, 1), net::port_t(2346)); */
  /* { */
  /*   Server iserver(0, server_soccer, server_addr.port, {client_addr}); */
  /*   Remote iclient(0, client_soccer, client_addr.port, server_addr); */
  /*   Window w(client_soccer, iclient); */
  /*   w.run(iserver); */
  /* } */

  Logger::Setup("minififa.log");
  Logger::MirrorLog(stderr);
  net::port_t port = (argc == 2) ? atoi(argv[1]) : 5679;
  std::set<net::Addr> metaservers;
  /* metaservers.insert(net::Addr(net::ip_from_ints(127, 0, 0, 1), net::port_t(5677))); */
  metaservers.insert(net::Addr(net::ip_from_ints(127, 0, 0, 1), net::port_t(5678)));
  Client client(metaservers, port);
  client.start();
  Window w(client);
  w.run();
  client.stop();
  Logger::Close();
}
