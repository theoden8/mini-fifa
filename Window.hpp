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
#include "SoccerObject.hpp"
#include "Cursor.hpp"

namespace glfw {
void error_callback(int error, const char* description);
void keypress_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void size_callback(GLFWwindow *window, int new_width, int new_height);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
}

class Window;

std::map <GLFWwindow *, Window *> window_reference;

class Window {
protected:
  size_t width_, height_;

  /* al::Audio audio; */
  Camera cam;
  Soccer &soccer;
  Intelligence &intelligence;
  std::tuple<Background, SoccerObject, Cursor> layers;
  /* std::tuple<Background, Player> layers; */

  void start() {
    init_glfw();
    init_glew();
    init_controls();
    /* audio.Init(); */
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
  Window(Soccer &soccer, Intelligence &intelligence):
    width_(0),
    height_(0),
    soccer(soccer),
    intelligence(intelligence),
    layers(Background(), SoccerObject(soccer, intelligence), Cursor())
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
  void run(Soccer &server_soccer) {
    start();
    cam.init();
    Tuple::for_each(layers, [&](auto &lyr) mutable {
      lyr.init();
    });
    cam.update(float(width()) / float(height()));

    /* audio.Play(); */
    Timer::time_t current_time = .0;
    glfwSwapInterval(2); GLERROR
    while(!glfwWindowShouldClose(window)) {
      cam.keyboard(window, double(width())/height());
      /* keyboard(); */
      idle_mouse();
      server_soccer.idle(current_time);
      soccer.idle(current_time);
      display();
      /* current_time += 1./60; */
      current_time = glfwGetTime();
    }
    /* audio.Stop(); */
    Tuple::for_each(layers, [&](auto &lyr) {
      lyr.clear();
    });
    cam.clear();
    glfwTerminate(); GLERROR
  }
  void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); GLERROR
    /* glEnable(GL_DEPTH_CLAMP); GLERROR */
    /* glEnable(GL_DEPTH_TEST); GLERROR */
    /* glDepthFunc(GL_LESS); GLERROR */
    /* glEnable(GL_STENCIL_TEST); GLERROR */
    /* glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); GLERROR */
    /* glStencilMask(0xFF); GLERROR */

    /* glEnable(GL_CULL_FACE); GLERROR */
    /* glCullFace(GL_FRONT); GLERROR */
    /* glFrontFace(GL_CW); GLERROR */

    Tuple::for_each(layers, [&](auto &lyr) mutable {
      lyr.display(cam);
    });
    glfwPollEvents(); GLERROR
    glfwSwapBuffers(window); GLERROR
  }
  void resize(float new_width, float new_height) {
    width_ = new_width, height_ = new_height;
    cam.update(float(width_)/height_);
  }
  void keyboard_event(int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
      std::get<1>(layers).keyboard(key);
    }
    if(action == GLFW_RELEASE) {
    }
  }
  void idle_mouse() {
    double m_x, m_y;
    glfwGetCursorPos(window, &m_x, &m_y);
    m_x /= width(), m_y /= height();
    cam.mouse(m_x, m_y);
    std::get<1>(layers).set_cursor(std::get<2>(layers), m_x, m_y, width(), height(), cam);
    std::get<2>(layers).mouse(m_x, m_y);
  }
  void mouse_click(double x, double y, int button, int action, int mods) {
    std::get<1>(layers).set_cursor(std::get<2>(layers), x/width(), y/height(), width(), height(), cam);
    std::get<1>(layers).mouse_click(button, action);
  }
  void mouse_scroll(double xoffset, double yoffset) {
  }
};

namespace glfw {
void error_callback(int error, const char* description) {
#ifndef NDEBUG
  Logger::Error("[GLFW] code %i msg: %s\n", error, description);
#endif
}
void keypress_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  window_reference[window]->keyboard_event(key, scancode, action, mods);
}
void size_callback(GLFWwindow *window, int new_width, int new_height) {
  window_reference[window]->resize((float)new_width, (float)new_height);
}
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
  double m_x, m_y;
  glfwGetCursorPos(window, &m_x, &m_y);
  window_reference[window]->mouse_click(m_x, m_y, button, action, mods);
}
void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  window_reference[window]->mouse_scroll(xoffset, yoffset);
}

}
