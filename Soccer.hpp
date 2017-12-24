#pragma once

#include <glm/gtc/matrix_inverse.hpp>

#include "Ball.hpp"
#include "Player.hpp"
#include "Pitch.hpp"
#include "Transformation.hpp"
#include "Camera.hpp"

struct Soccer {
  Pitch pitch;
  Ball ball;
  std::vector<Player> players;

  static constexpr bool RED_TEAM = false;
  static constexpr bool BLUE_TEAM = true;

  glm::vec2 cursor;

  Soccer(size_t team1=1, size_t team2=1):
    players()
  {
    for(int i = 0; i < team1; ++i) {
      players.push_back(Player(RED_TEAM, {-.1f, .0f}));
    }
    for(int i = 0; i < team2; ++i) {
      players.push_back(Player(BLUE_TEAM, {.1f, .0f}));
    }
  }

  void init() {
    pitch.init();
    ball.init();
    for(auto &p: players) {
      p.init();
    }
  }

  void Keyboard(GLFWwindow *w) {
    ball.Keyboard(w);
    players[0].Keyboard(w);
    /* for(auto &p: players) { */
    /*   p.Keyboard(w); */
    /* } */
  }

  void mouse(float m_x, float m_y, Camera &cam) {
    // pitch:
    // pos = ([-2, 2], [-1, 1], 0, 1)
    // displayed = MVP * pos
    //
    // mouse -> pitch:
    // mouse = MVP * pos
    // MVP^-1 * mouse = pos
    // MVP = cam * model
    // MVP^-1 = model^-1 * cam^-1
    //
    // inverse of an affine transformation is exactly what we need
    double s_x = m_x * 2 - 1, s_y = (2 * m_y - 1);
    glm::vec4 lhs = glm::vec4(s_x, s_y, 0, 1);

    glm::mat4 model_inverse = glm::affineInverse(pitch.transform.get_matrix());
    glm::mat4 cam_inverse = glm::inverseTranspose(cam.get_matrix());
    glm::mat4 inverse_transform = model_inverse * cam_inverse;
    glm::vec4 pos = inverse_transform * lhs;
    cursor = glm::vec2(pos.x, pos.y);
    /* ball.position() = glm::vec3(pos.x, pos.y - 1, ball.position().z); */
    /* printf("%f %f\n", ball.position().x, ball.position().y); */
  }

  double current_time = 0.;
  void idle(double curtime=NAN) {
    current_time = std::isnan(curtime) ? glfwGetTime() : curtime;
    ball.idle(current_time);
    for(auto &p: players) {
      p.idle(current_time);
    }
    update_possession();
    if(current_owner != NO_OWNER) {
      auto &p = players[current_owner];
      glm::vec2 dir = p.direction();
      glm::vec3 dir3 = glm::vec3(dir.x, dir.y, 0);
      if(!p.is_sliding()) {
        ball.position() = p.possession_point();
        ball.velocity() = p.is_running() ? float(p.running_speed) * dir3 : glm::vec3(0, 0, 0);
      } else {
        ball.velocity() = float(p.sliding_speed) * dir3;
      }
    }
  }

  static constexpr int NO_OWNER = -1;
  int current_owner = NO_OWNER;
  void update_possession() {
    // if noone controls, closest gets the ball
    // if someone controls, closest other than the owner or nothing controls the ball
    glm::vec3 ball_pos = ball.position();
    int owner = current_owner;
    double best_tackle_value = NAN;
    for(int i = 0; i < players.size(); ++i) {
      if(i == current_owner)continue;
      auto &p = players[i];
      double value = p.tackle_value(ball_pos);
      if(std::isnan(value))continue;
      if(std::isnan(best_tackle_value) || best_tackle_value > value) {
        best_tackle_value = value;
        owner = i;
      }
    }
    if(current_owner != NO_OWNER && std::isnan(best_tackle_value)) {
      // our owner is not controlling the ball
      // and noone else is able to pick it up
      if(std::isnan(players[current_owner].tackle_value(ball_pos))) {
        owner = NO_OWNER;
        players[current_owner].lose_ball();
      }
      // otherwise there's still the same person reaching the ball
    } else if(owner != NO_OWNER && current_owner != owner) {
      // some player takse over the ball
      players[current_owner].lose_ball();
    }
    current_owner = owner;
  }

  void display(Camera &cam) {
    idle();

    pitch.display(cam);
    ball.display(cam);

    std::vector<int> indices(players.size());
    for(int i = 0; i < players.size(); ++i) {
      indices[i] = i;
    }
    // sort by Y coordinate as we want to see the closest.
    for(int i = 0; i < players.size() - 1; ++i) {
      for(int j = i + 1; j < players.size(); ++j) {
        if(players[indices[i]].cur_pos.y > players[indices[j]].cur_pos.y) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    for(auto &ind: indices) {
      players[ind].display(cam);
    }
  }

  void clear() {
    for(auto &p: players) {
      p.clear();
    }
    ball.clear();
    pitch.clear();
  }
};
