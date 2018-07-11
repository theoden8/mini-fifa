#pragma once

#include "Camera.hpp"
#include "CursorObject.hpp"
#include "Intelligence.hpp"
#include "Soccer.hpp"
#include "BackgroundObject.hpp"
#include "SoccerObject.hpp"

struct GameObject {
  Camera cam;
  BackgroundObject backgrObj;
  SoccerObject soccerObject;

  ui::CursorObject &cursor; // no ownership, but may modify

  size_t w_width;
  size_t w_height;

  Timer::time_t current_time = 0.;

  GameObject(Soccer &soccer, Intelligence<IntelligenceType::ABSTRACT> &intelligence, ui::CursorObject &cursor):
    backgrObj(),
    soccerObject(soccer, intelligence),
    cursor(cursor)
  {}

  bool is_active() {
    return !soccerObject.intelligence.has_quit();
  }

  void init() {
    Logger::Info("gobject: intiialized\n");
    backgrObj.init();
    soccerObject.init();
  }

  void set_winsize(size_t w, size_t h) {
    w_width=w,w_height=h;
    cam.update(float(w_width)/w_height);
  }

  void keyboard(GLFWwindow *window) {
    cam.keyboard(window, double(w_width)/w_height);
  }

  void keypress(int key, int mods) {
    soccerObject.keypress(key, mods);
  }

  void mouse(double m_x, double m_y) {
    cam.mouse(m_x, m_y);
    soccerObject.set_cursor(cursor, m_x, m_y, w_width, w_height, cam);
  }

  void mouse_click(int button, int action) {
    soccerObject.mouse_click(button, action);
  }

  void idle() {
    soccerObject.intelligence.idle(current_time);
  }

  void display() {
    if(!is_active())return;
    backgrObj.display(cam);
    soccerObject.display(cam);
  }

  void clear() {
    Logger::Info("gobject: clearance\n");
    backgrObj.clear();
    soccerObject.clear();
  }
};
