#include "MetaServer.hpp"

int main() {
  Logger::Setup("metaserver.log");
  Logger::MirrorLog(stderr);
  MetaServer metaserver;
  metaserver.run();
  Logger::Close();
}
