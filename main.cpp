#include "Window.hpp"
#include "Pitch.hpp"

int main() {
  gl::Window w;
  Logger::Setup("prog.log");
  Logger::MirrorLog(stderr);
  w.init();
  w.idle();
  w.clear();
  Logger::Close();
}
