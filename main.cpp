#include "Window.hpp"

int main() {
  Logger::Setup("prog.log");
  Logger::MirrorLog(stderr);
  Soccer server_soccer;
  Soccer client_soccer;
  net::Addr server_addr(net::ip_from_ints(0, 0, 0, 0), net::port_t(2345));
  net::Addr client_addr(net::ip_from_ints(0, 0, 0, 0), net::port_t(2346));
  {
    Remote<RemoteType::SERVER> iserver(0, server_soccer, server_addr.port, {client_addr});
    Remote<RemoteType::CLIENT> iclient(0, client_soccer, client_addr.port, server_addr);
    Window w(client_soccer, iclient);
    w.run(iserver);
  }
  Logger::Close();
}
