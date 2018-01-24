#include "Client.hpp"
#include "Window.hpp"

int main(int argc, char *argv[]) {
  Logger::Setup("minififa.log");
  Logger::MirrorLog(stderr);
  net::port_t port = (argc == 2) ? atoi(argv[1]) : 5679;
  std::set<net::Addr> metaservers;
  /* metaservers.insert(net::Addr(net::ipv4_from_ints(127, 0, 0, 1), net::port_t(5677))); */
  /* metaservers.insert(net::Addr(net::ipv4_from_ints(127, 0, 0, 1), net::port_t(5678))); */
  Client client(metaservers, port);
  client.start();
  Window w(client);
  w.run();
  client.stop();
  Logger::Close();
}
