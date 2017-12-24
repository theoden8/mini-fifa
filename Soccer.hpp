#pragma once

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Camera.hpp"
#include "Ball.hpp"
#include "Player.hpp"
#include "Pitch.hpp"
#include "Post.hpp"
#include "Transformation.hpp"

#include "invert_4x4_mat.hpp"

struct Soccer {
  Pitch pitch;
  Post post_red, post_blue;
  Ball ball;
  std::vector<Player> players;

  static constexpr bool RED_TEAM = false;
  static constexpr bool BLUE_TEAM = true;

  size_t team1_size;
  size_t team2_size;

  double loose_ball_period = .16;
  glm::vec2 cursor;

  Soccer(size_t team1=2, size_t team2=1):
    players(), post_red(RED_TEAM), post_blue(BLUE_TEAM),
    team1_size(team1), team2_size(team2)
  {
    for(int i = 0; i < team1 + team2; ++i) {
      bool t = get_team(i);
      if(t == RED_TEAM) {
        players.push_back(Player(RED_TEAM, {.1f, .0f}));
      } else {
        players.push_back(Player(BLUE_TEAM, {-.1f, .0f}));
      }
    }
  }

  void init() {
    pitch.init();
    post_red.init();
    post_blue.init();
    ball.init();
    for(auto &p: players) {
      p.init();
    }
  }

  void Keyboard(GLFWwindow *w) {
    ball.Keyboard(w);
    players[0].Keyboard(w);
    if(glfwGetKey(w, GLFW_KEY_Z)) {
      z_perform();
    }
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
    double s_x = m_x * 2 - 1, s_y = 2 * m_y - 1;
    glm::vec4 lhs = glm::vec4(s_x, s_y, 0, 1);

    glm::mat4 model_inverse = pitch.transform.inverse();
    glm::mat4 cam_mat = cam.get_matrix();
    glm::mat4 cam_inverse;
    float inverted[16];
    invert_4x4(glm::value_ptr(cam_mat), glm::value_ptr(cam_inverse));

    glm::mat4 transformat = cam_mat * pitch.transform.get_matrix();
    glm::mat4 inverse_transform = model_inverse * cam_inverse;
    auto rand_dc = [](){ return float(rand() % 2000 - 1000) / 1000; };
    glm::vec4 v(rand_dc(), rand_dc(), 0, 1); auto w = v * transformat; auto u = w * inverse_transform;
    printf("[%.3f %.3f %.3f %.3f] -> [%.3f %.3f %.3f %.3f] -> [%.3f %.3f %.3f %.3f]\n",
      v.x, v.y, v.z, v.w,
      w.x, w.y, w.z, w.w,
      u.x, u.y, u.z, u.w
    );
    glm::vec4 pos = inverse_transform * lhs;
    printf("[%.3f %.3f %.3f %.3f] -> [%.3f %.3f %.3f %.3f]\n\n", s_x, s_y, 0., 1., pos.x, pos.y, pos.z, pos.w);
    cursor = glm::vec2(pos.x, pos.y);
    /* ball.position() = glm::vec3(pos.x, pos.y - 1, ball.position().z); */
  }

  bool ball_is_loose() {
    if(current_owner == NO_OWNER)return false;
    return current_time - players[current_owner].time_got_ball < loose_ball_period;
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
      if(!p.is_sliding()) {
        if(!ball_is_loose()) {
          ball.position() = p.possession_point();
          ball.velocity() = p.speed();
        }
      } else {
        ball.velocity() = p.speed();
      }
    }
  }

  const glm::vec3 single_player_pass_point = glm::vec3(.0, 2./3, 0);

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
      if(current_owner != NO_OWNER && get_team(current_owner) == get_team(i))continue;
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
        players[current_owner].lose_ball(current_time);
      }
      // otherwise there's still the same person reaching the ball
    } else if(owner != NO_OWNER && current_owner != owner) {
      // some player takse over the ball
      players[current_owner].lose_ball(current_time);
    }
    if(owner != NO_OWNER && current_owner != owner)players[owner].get_ball(current_time);
    current_owner = owner;
  }

  bool get_team(size_t i) {
    return (i < team1_size) ? RED_TEAM : BLUE_TEAM;
  }

  int get_pass_destination(int playerId) {
    bool team = get_team(playerId);
    if(team == NO_OWNER)return NO_OWNER;
    int start = (team == RED_TEAM) ? 0 : team1_size;
    int end = (team == RED_TEAM) ? team1_size : team2_size;

    int ind_pass_to = NO_OWNER;
    double range = NAN;
    for(int i = start; i < end; ++i) {
      if(playerId == i)continue;
      int dist = glm::length(players[playerId].position() - players[i].position());
      if(std::isnan(range) || dist < range) {
        range = dist;
        ind_pass_to = i;
      }
    }
    return ind_pass_to;
  }

  void z_perform() {
    if(current_owner == NO_OWNER)return;
    auto &p = players[current_owner];
    if(p.is_jumping()) {
      through_ball();
    } else {
      pass();
    }
  }

  double pass_speed = .01;
  glm::vec3 pass_vector{1, 0, .5};
  void pass() {
    if(current_owner == NO_OWNER)return;
    auto &p = players[current_owner];
    if(!p.try_pass(current_time))return;
    int pass_to = get_pass_destination(current_owner);
    glm::vec3 dest = (pass_to == NO_OWNER) ? single_player_pass_point : players[pass_to].possession_point();
    glm::vec3 dir = dest - p.position();
    float angle = atan2(dir.y, dir.x);
    p.kick_the_ball(ball, pass_vector * float(pass_speed), angle);
  }

  double through_ball_speed = .02;
  glm::vec3 through_ball_vector{1, 0, 0};
  void through_ball() {
    if(current_owner == NO_OWNER)return;
    auto &p = players[current_owner];
    if(!p.try_pass(current_time))return;
    if(p.height() < .001)return;
    p.kick_the_ball(ball, through_ball_vector * float(through_ball_speed), p.facing);
  }

  void display(Camera &cam) {
    idle();

    pitch.display(cam);
    post_red.display(cam);
    post_blue.display(cam);
    ball.display(cam);

    std::vector<int> indices(players.size());
    for(int i = 0; i < players.size(); ++i) {
      indices[i] = i;
    }
    // sort by Y coordinate as we want to see the closest.
    for(int i = 0; i < players.size() - 1; ++i) {
      for(int j = i + 1; j < players.size(); ++j) {
        if(players[indices[i]].position().y > players[indices[j]].position().y) {
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
    post_red.clear();
    post_blue.clear();
    pitch.clear();
  }
};
