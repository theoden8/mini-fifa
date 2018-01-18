#pragma once

#include <cstdlib>
#include <unistd.h>

#include <vector>
#include <tuple>
#include <string>
#include <map>

#include "incgraphics.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Logger.hpp"
#include "Debug.hpp"

#include "ClientObject.hpp"

namespace glfw {
  void error_callback(int error, const char* description);
  void keypress_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
  void size_callback(GLFWwindow *window, int new_width, int new_height);
  void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
  void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
}

std::map <GLFWwindow *, void *> window_reference;

class Window {
protected:
  size_t width_, height_;

  ClientObject cObject;

  void start() {
    init_glfw();
    init_glew();
    init_controls();
    /* gl_version(); */
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

    window = glfwCreateWindow(width(), height(), "pitch", nullptr, nullptr);
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); GLERROR
  }
  const GLFWvidmode *vidmode = nullptr;
public:
  GLFWwindow *window = nullptr;
  Window(net::Addr metaserver):
    width_(0), height_(0),
    cObject(metaserver)
  {}
  size_t width() const { return width_; }
  size_t height() const { return height_; }
  void gl_version() {
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
  void run() {
    ui::Font::setup();
    start();
    cObject.client.start();
    cObject.init();

    Timer::time_t current_time = .0;
    glfwSwapInterval(2); GLERROR
    while(!glfwWindowShouldClose(window)) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); GLERROR
      cObject.display(window, width(), height());
      mouse();
      glfwPollEvents(); GLERROR
      glfwSwapBuffers(window); GLERROR
    }
    cObject.clear();
    glfwTerminate(); GLERROR
    ui::Font::cleanup();
    cObject.client.stop();
  }
  /* void display() { */
    /* glEnable(GL_DEPTH_CLAMP); GLERROR */
    /* glEnable(GL_DEPTH_TEST); GLERROR */
    /* glDepthFunc(GL_LESS); GLERROR */
    /* glEnable(GL_STENCIL_TEST); GLERROR */
    /* glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); GLERROR */
    /* glStencilMask(0xFF); GLERROR */

    /* glEnable(GL_CULL_FACE); GLERROR */
    /* glCullFace(GL_FRONT); GLERROR */
    /* glFrontFace(GL_CW); GLERROR */
  /* } */
  void resize(float new_width, float new_height) {
    width_ = new_width, height_ = new_height;
  }
  void keyboard_event(int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
      cObject.keypress(key);
    }
    if(action == GLFW_RELEASE) {
    }
  }
  void mouse() {
    double m_x, m_y;
    glfwGetCursorPos(window, &m_x, &m_y);
    m_x /= width(), m_y /= height();
    cObject.mouse(m_x, m_y);
  }
  void mouse_click(double x, double y, int button, int action, int mods) {
    double m_x = x/width(), m_y = y/height();
    cObject.mouse(m_x, m_y);
    cObject.mouse_click(button, action);
  }
  void mouse_scroll(double xoffset, double yoffset) {
  }
};

static Window &get_window(GLFWwindow *window) {
  return *((Window *)(window_reference[window]));
}

namespace glfw {
void error_callback(int error, const char* description) {
#ifndef NDEBUG
  Logger::Error("[GLFW] code %i msg: %s\n", error, description);
#endif
}
void keypress_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  get_window(window).keyboard_event(key, scancode, action, mods);
}
void size_callback(GLFWwindow *window, int new_width, int new_height) {
  get_window(window).resize((float)new_width, (float)new_height);
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  double m_x, m_y;
  glfwGetCursorPos(window, &m_x, &m_y);
  get_window(window).mouse_click(m_x, m_y, button, action, mods);
}
void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  get_window(window).mouse_scroll(xoffset, yoffset);
}

}
