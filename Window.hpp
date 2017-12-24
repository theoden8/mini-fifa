#pragma once

#include <vector>
#include <tuple>
#include <string>
#include <map>
#include <cstdlib>
#include <unistd.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "incgraphics.h"

#include "Tuple.hpp"
#include "Logger.hpp"
#include "Debug.hpp"

#include "Camera.hpp"
#include "Background.hpp"
#include "Soccer.hpp"

namespace glfw {
void error_callback(int error, const char* description);
void keypress_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void size_callback(GLFWwindow *window, int new_width, int new_height);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
}

namespace gl {
class Window;

std::map <GLFWwindow *, Window *> window_reference;

class Window {
protected:
  size_t width_, height_;

  /* al::Audio audio; */
  Camera cam;
  std::tuple<Background, Soccer> layers;
  /* std::tuple<Background, Player> layers; */

  void start() {
    init_glfw();
    init_glew();
    init_controls();
    /* audio.Init(); */
    /* GLVersion(); */
  }
  void init_glfw() {
    glfwSetErrorCallback(glfw::error_callback);
    int rc = glfwInit();
    ASSERT(rc == 1);

    vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    ASSERT(vidmode != nullptr);
    width_ = vidmode->width;
    height_ = vidmode->height;

    /* glfwWindowHint(GLFW_SAMPLES, 4); */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL

    #define MONITOR nullptr
    window = glfwCreateWindow(width(), height(), "pitch", MONITOR, nullptr);
    ASSERT(window != nullptr);
    window_reference[window] = this;
    glfwMakeContextCurrent(window); GLERROR
    glfwSetKeyCallback(window, glfw::keypress_callback); GLERROR
    glfwSetWindowSizeCallback(window, glfw::size_callback); GLERROR
    glfwSetMouseButtonCallback(window, glfw::mouse_button_callback); GLERROR
    glfwSetScrollCallback(window, glfw::mouse_scroll_callback); GLERROR
  }
  void init_glew() {
    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    GLuint res = glewInit(); GLERROR
    ASSERT(res == GLEW_OK);
  }
  void init_controls() {
    // ensure we can capture the escape key
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE); GLERROR
    /* glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); GLERROR */
  }
  const GLFWvidmode *vidmode = nullptr;
public:
  GLFWwindow *window = nullptr;
  Window():
    width_(0), height_(0)
  {}
  size_t width() const {
    return width_;
  }
  size_t height() const {
    return height_;
  }
  void GLVersion() {
    // get version info
    const GLubyte* renderer = glGetString(GL_RENDERER); GLERROR // get renderer string
    const GLubyte* version = glGetString(GL_VERSION); GLERROR // version as a string
    Logger::Info("Renderer: %s\n", renderer);
    Logger::Info("OpenGL version supported %s\n", version);
    Logger::Info("Supported OpenGL extensions:\n");
    GLint no_exts;
    glGetIntegerv(GL_NUM_EXTENSIONS, &no_exts);
    for(GLint i = 0; i < no_exts; ++i) {
      Logger::Info("\t%s\n", glGetStringi(GL_EXTENSIONS, i));
    }
  }
  void init() {
    start();
    Logger::Info("INIT\n");
    cam.init();
    Tuple::for_each(layers, [&](auto &lyr) mutable {
      Logger::Info("INIT LAYER\n");
      lyr.init();
    });
  }
  void idle() {
    double m_x, m_y;
    /* audio.Play(); */
    while(!glfwWindowShouldClose(window)) {
      display();
      std::get<1>(layers).Keyboard(window);
      cam.keyboard(window, double(width())/height());
      /* keyboard(); */
      glfwGetCursorPos(window, &m_x, &m_y);
      m_x /= width(), m_y /= height();
      cam.mouse(m_x, m_y);
      std::get<1>(layers).mouse(m_x, m_y, cam);
    }
    /* audio.Stop(); */
  }
  void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); GLERROR
    /* glEnable(GL_DEPTH_CLAMP); GLERROR */
    /* glEnable(GL_DEPTH_TEST); GLERROR */
    /* glDepthFunc(GL_LESS); GLERROR */
    /* glEnable(GL_STENCIL_TEST); GLERROR */
    /* glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); GLERROR */
    /* glStencilMask(0xFF); GLERROR */

    glEnable(GL_CULL_FACE); GLERROR
    glCullFace(GL_FRONT); GLERROR
    glFrontFace(GL_CW); GLERROR

    Tuple::for_each(layers, [&](auto &lyr) mutable {
      lyr.display(cam);
    });

    glfwPollEvents(); GLERROR
    glfwSwapBuffers(window); GLERROR
  }
  void resize(float new_width, float new_height) {
    width_ = new_width, height_ = new_height;
    cam.WindowResize(new_width, new_height);
    /* current_screen->Resize(); */
  }
  void keyboard_event(int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, true);
    }
    if(action == GLFW_PRESS) {
      /* current_screen->KeyPress(key, scancode, mods); */
    }
    if(action == GLFW_RELEASE) {
      /* current_screen->KeyRelease(key, scancode, mods); */
    }
  }
  void mouse_click(double x, double y, int button, int action, int mods) {
  }
  void mouse_scroll(double xoffset, double yoffset) {
  }
  void clear() {
    Tuple::for_each(layers, [&](auto &lyr) {
      lyr.clear();
    });
    cam.clear();
    glfwTerminate(); GLERROR
  }
};
}

namespace glfw {
void error_callback(int error, const char* description) {
#ifndef NDEBUG
  Logger::Error("[GLFW] code %i msg: %s\n", error, description);
#endif
}
void keypress_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  gl::window_reference[window]->keyboard_event(key, scancode, action, mods);
}
void size_callback(GLFWwindow *window, int new_width, int new_height) {
  gl::window_reference[window]->resize((float)new_width, (float)new_height);
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  double m_x, m_y;
  glfwGetCursorPos(window, &m_x, &m_y);
  gl::window_reference[window]->mouse_click(m_x, m_y, button, action, mods);
}
void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  gl::window_reference[window]->mouse_scroll(xoffset, yoffset);
}

}
