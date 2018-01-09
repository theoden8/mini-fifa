#include "Window.hpp"

int main() {
  Soccer soccer;
  Remote<RemoteType::Server> iserver(0, soccer, 2345);
  Window w(soccer, iserver);
  Logger::Setup("prog.log");
  Logger::MirrorLog(stderr);
  w.run();
  Logger::Close();
}
