#define GLM_ENABLE_EXPERIMENTAL
#include "MetaServer.hpp"

int main(int argc ,char *argv[]) {
  Logger::Setup("metaserver.log");
  Logger::MirrorLog(stderr);
  net::port_t port = (argc == 2) ? atoi(argv[1]) : 5678;
  MetaServer metaserver(port);
  metaserver.run();
  Logger::Close();
}
