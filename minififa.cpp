#include "Client.hpp"
#include "Window.hpp"

int main(int argc, char *argv[]) {
  std::string execdir = sys::get_executable_directory(argc, argv);
  Logger::Setup();
  Logger::SetLogOutput(sys::Path(execdir) / sys::Path("minififa.log"));
  Logger::MirrorLog(stdout);
  Logger::Info("dir '%s'\n", execdir.c_str());
  const std::string curdir = sys::get_current_dir();
  Logger::Info("curdir '%s'\n", curdir.c_str());
  Logger::Info("execdir '%s'\n", execdir.c_str());
  net::port_t port = (argc == 2) ? atoi(argv[1]) : 5679;
  std::set<net::Addr> metaservers;
  /* metaservers.insert(net::Addr(net::ipv4_from_ints(127, 0, 0, 1), net::port_t(5677))); */
  metaservers.insert(net::Addr(net::ipv4_from_ints(127, 0, 0, 1), net::port_t(5678)));
  Client client(metaservers, port);
  client.start();
  Window w(client, execdir);
  w.run();
  client.stop();
  Logger::Close();
}
