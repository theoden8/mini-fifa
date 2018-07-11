#pragma once

#include <cstdlib>
#include <unistd.h>

#include <vector>
#include <tuple>
#include <string>
#include <map>

#include "incgraphics.h"

#include "Transformation.hpp"
#include "ImageLoader.hpp"
#include "Logger.hpp"
#include "Debug.hpp"

#include "Region.hpp"
#include "ClientObject.hpp"

namespace glfw {
  void error_callback(int error, const char* description);
  void keypress_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
  void size_callback(GLFWwindow *window, int new_width, int new_height);
  void cursor_area_callback(GLFWwindow *window, double xpos, double ypos);
  void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
  void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
}

std::map <GLFWwindow *, void *> window_reference;

class Window {
protected:
  size_t width_, height_;

  Region cursor_area{
    {0, 1},
    {0, 1}
  };
  glm::vec2 cursor_pos{0, 0};
  ClientObject cObject;

  void start() {
    init_glfw();
    init_glew();
    init_controls();
    gl_version();
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
    glfwSetCursorPosCallback(window, glfw::cursor_area_callback); GLERROR
    glfwSetMouseButtonCallback(window, glfw::mouse_button_callback); GLERROR
    glfwSetScrollCallback(window, glfw::mouse_scroll_callback); GLERROR
  }
  void init_glew() {
    // Initialize GLEW
    glewExperimental = GL_TRUE; // Needed for core profile
    auto res = glewInit(); GLERROR
    // on some systems, returns invalid value even if succeeds
    if(res != GLEW_OK) {
      Logger::Error("glew error: %s\n", "there was some error initializing glew");
      /* Logger::Error("glew error: %s\n", glewGetErrorString()); */
    }
  }
  void init_controls() {
    // ensure we can capture the escape key
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE); GLERROR
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); GLERROR
  }
  const GLFWvidmode *vidmode = nullptr;
public:
  GLFWwindow *window = nullptr;
  Window(Client &client):
    width_(0), height_(0),
    cObject(client)
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
    start();
    ui::Font::setup();
    cObject.init();

    Timer::time_t current_time = .0;
    glfwSwapInterval(2); GLERROR
    while(!glfwWindowShouldClose(window) && cObject.is_active()) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); GLERROR
      cObject.mouse(cursor_pos.x, cursor_pos.y);
      cObject.display(window, width(), height());
      glfwPollEvents(); GLERROR
      glfwSwapBuffers(window); GLERROR
    }
    cObject.clear();
    ui::Font::cleanup();
    glfwDestroyWindow(window); GLERROR
    glfwTerminate(); GLERROR
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
      cObject.keypress(key, mods);
    }
  }
  void mouse(double m_x, double m_y) {
    m_x /= width(), m_y /= height();
    if(m_x >= cursor_area.x2()) {
      cursor_area.xs += (m_x - cursor_area.x2());
    } else if(m_x <= cursor_area.x1()) {
      cursor_area.xs -= (cursor_area.x1() - m_x);
    }
    if(m_y >= cursor_area.y2()) {
      cursor_area.ys += (m_y - cursor_area.y2());
    } else if(m_y <= cursor_area.y1()) {
      cursor_area.ys -= (cursor_area.y1() - m_y);
    }
    cursor_pos = {m_x - cursor_area.x1(), m_y - cursor_area.y1()};
  }
  void mouse_click(int button, int action, int mods) {
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
/* #ifndef NDEBUG */
  Logger::Error("[GLFW] code %i msg: %s\n", error, description);
/* #endif */
}
void keypress_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  get_window(window).keyboard_event(key, scancode, action, mods);
}
void size_callback(GLFWwindow *window, int new_width, int new_height) {
  get_window(window).resize((float)new_width, (float)new_height);
}
void cursor_area_callback(GLFWwindow *window, double xpos, double ypos) {
  get_window(window).mouse(xpos, ypos);
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  get_window(window).mouse_click(button, action, mods);
}
void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  get_window(window).mouse_scroll(xoffset, yoffset);
}

}
