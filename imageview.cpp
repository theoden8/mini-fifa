#include "incgraphics.h"
#include "Transformation.hpp"
#include "Logger.hpp"
#include "Debug.hpp"

#include "Timer.hpp"
#include "Button.hpp"

class ImageViewer {
  C_STRING(font_name, "assets/Verdana.ttf");

  C_STRING(tex_name, "assets/button.png");
  ui::Button<tex_name, font_name> button;

  void setup() {
    button.setx(-1, 1);
    button.sety(-1, 1);
    button.init();
  }
protected:
  const GLFWvidmode *vidmode = nullptr;
  size_t width_, height_;
  void init_glfw() {
    int rc = glfwInit();
    ASSERT(rc == 1);

    vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    ASSERT(vidmode != nullptr);
    width_ = vidmode->width;
    height_ = vidmode->height;
    width_ = 800, height_ = 600;

    /* glfwWindowHint(GLFW_SAMPLES, 4); */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL

    window = glfwCreateWindow(width(), height(), "imageview", nullptr, nullptr);
    ASSERT(window != nullptr);
    glfwMakeContextCurrent(window); GLERROR
  }
  void init_controls() {
    // ensure we can capture the escape key
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE); GLERROR
    /* glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); GLERROR */
  }
public:
  GLFWwindow *window = nullptr;
  ImageViewer(): width_(0), height_(0) {}
  size_t width() const { return width_; }
  size_t height() const { return height_; }
  void run() {
    init_glfw();
    init_controls();
    ui::Font::setup();
    setup();
    Timer::time_t current_time = .0;
    glfwSwapInterval(2); GLERROR
    while(!glfwWindowShouldClose(window)) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); GLERROR
      button.label.set_text("text");
      button.display();
      glfwPollEvents(); GLERROR
      glfwSwapBuffers(window); GLERROR
    }
    // display image
    ui::Font::cleanup();
    glfwDestroyWindow(window); GLERROR
    glfwTerminate(); GLERROR
  }
  void resize(float new_width, float new_height) {
    width_ = new_width, height_ = new_height;
  }
};

int main(int argc, char *argv[]) {
  Logger::Setup("imageview.log");
  Logger::MirrorLog(stderr);
  ImageViewer imview;
  imview.run();
  Logger::Close();
}
