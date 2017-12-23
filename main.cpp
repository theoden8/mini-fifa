#include "Window.hpp"
#include "Model.hpp"

int main() {
  gl::Window w;
  Logger::Setup("prog.log");
  Logger::MirrorLog(stderr);
  w.init();
  w.idle();
  w.clear();
  Logger::Close();
}
