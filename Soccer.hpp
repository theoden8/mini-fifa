#pragma once

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Ball.hpp"
#include "Player.hpp"
#include "Pitch.hpp"
#include "Post.hpp"

#include "Timer.hpp"

struct Team;

struct Soccer {
  std::vector<Player> players;
  Pitch pitch;
  Post post_red, post_blue;
  Ball ball;

  Soccer(size_t team1sz=2, size_t team2sz=2):
    players(), post_red(Team::RED_TEAM), post_blue(Team::BLUE_TEAM),
    team1(*this, team1sz, Team::RED_TEAM), team2(*this, team2sz, Team::BLUE_TEAM)
  {
    for(int i = 0; i < team1sz + team2sz; ++i) {
      Team &t = get_team(i);
      float top1 = .1 * team1.size() / 2;
      float top2 = .1 * team2.size() / 2;
      if(get_team(i).id() == Team::RED_TEAM) {
        players.push_back(Player(i, Team::RED_TEAM, {.1f, top1 - .1*i}));
      } else {
        players.push_back(Player(i, Team::BLUE_TEAM, {-.1f, top2 - .1*(i-team1.size())}));
      }
      players.back().unit.face(glm::vec3(0, 0, 0));
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
    set_timer();
  }

  void set_timer() {
  }

  void keyboard(int key) {
    ball.keyboard(key);
    players[0].keyboard(key);
    if(key == GLFW_KEY_Z) {
      z_perform(ball.owner());
    } else if(key == GLFW_KEY_X) {
      x_perform(0, players[0].unit.facing);
    } else if(key == GLFW_KEY_C) {
      c_perform(0, players[0].unit.facing);
    } else if(key == GLFW_KEY_V) {
      v_perform(0);
    }
  }

  void mouse(float m_x, float m_y, float m_z, float width, float height, Camera &cam) {
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
    double s_x = 2 * m_x - 1, s_y = 2 * m_y - 1, s_z = m_z;

    glm::vec3 mvec(s_x, s_y, s_z);
    glm::mat4 model_mat = pitch.transform.get_matrix();
    glm::mat4 cam_mat = cam.get_matrix();
    glm::mat4 inverse = glm::inverse(cam_mat * model_mat);
    glm::vec4 viewport(0, 0, width, height);
    glGetFloatv(GL_VIEWPORT, glm::value_ptr(viewport)); GLERROR
    glm::vec3 unproject = glm::unProject(
      mvec,
      pitch.transform.get_matrix(),
      cam.get_matrix(),
      viewport
    );
    /* glm::vec4 v(rand_dc(), rand_dc(), 0, 1); auto w = inverse * v * model_mat * cam_mat; */
    /* printf("[%.3f %.3f %.3f %.3f] -> [%.3f %.3f %.3f %.3f]\n", */
    /*   v.x, v.y, v.z, v.w, */
    /*   w.x, w.y, w.z, w.w */
    /* ); */
    /* glm::vec4 pos = inverse * lhs; */
    /* printf("[%.3f %.3f %.3f %.3f] -> [%.3f %.3f %.3f %.3f]\n\n", s_x, s_y, 0., 1., pos.x, pos.y, pos.z, pos.w); */
    cursor = glm::vec2(unproject.x, unproject.y);
    /* ball.position() = glm::vec3(unproject.x*2, unproject.y, ball.unit.height()); */
  }
  void display(Camera &cam) {
    idle(glfwGetTime());

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
        if(players[indices[i]].unit.pos.y > players[indices[j]].unit.pos.y) {
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

// gameplay
  Timer timer;
  enum class GameState {
    IN_PROGRESS,
    RED_START,
    BLUE_START,
    RED_THROWIN,
    BLUE_THROWIN,
    HALFTIME,
    FINISHED
  };
  GameState state = GameState::RED_START;
  struct Team {
    static constexpr bool RED_TEAM = 0;
    static constexpr bool BLUE_TEAM = 1;

    Soccer &soccer;
    size_t teamSize;
    bool teamId;

    Team(Soccer &soccer, size_t teamSize, bool id):
      soccer(soccer), teamSize(teamSize), teamId(id)
    {}

    size_t size() const {
      return teamSize;
    }

    bool id() const {
      return teamId;
    }

    Player &operator[](size_t i) {
      if(!teamId) {
        return soccer.players[i];
      } else {
        return soccer.players[soccer.team1.size() + i];
      }
    }

    Player operator[](size_t i) const {
      if(!teamId) {
        return soccer.players[i];
      } else {
        return soccer.players[soccer.team1.size() + i];
      }
    }

    bool operator==(const Team &other) const {
      return id() == other.id();
    }

    decltype(auto) begin() const { return soccer.players.begin() + ((teamId==RED_TEAM) ? 0 : soccer.team1.size()); }
    decltype(auto) begin() { return soccer.players.begin() + ((teamId==RED_TEAM) ? 0 : soccer.team1.size()); }
    decltype(auto) end() const {
      if(teamId == RED_TEAM) {
        if(soccer.team2.size())return soccer.players.end();
        else return soccer.players.begin() + size();
      } else {
        return soccer.players.end();
      }
    }
    decltype(auto) end() {
      if(teamId == RED_TEAM) {
        if(soccer.team2.size())return soccer.players.end();
        else return soccer.players.begin() + size();
      } else {
        return soccer.players.end();
      }
    }
  };

  Team team1, team2;
  glm::vec2 cursor;

  void idle(double curtime) {
    timer.set_time(curtime);
    ball.idle(timer.current_time);
    for(auto &p: players) {
      p.idle(timer.current_time);
    }
    idle_control();
  }

  void idle_control() {
    if(ball.owner() != Ball::NO_OWNER) {
      auto &p = players[ball.owner()];
      if(!p.is_sliding() && !ball.is_loose()) {
        printf("controlling fully\n");
        ball.position() = p.possession_point();
      } else if(p.is_sliding()) {
        printf("controlling with slide\n");
        ball.velocity() = p.velocity();
        ball.unit.height() = .005;
      }
    }
    int new_owner = find_best_possession(ball);
    set_control_player(new_owner);
  }

  const Unit::vec_t single_player_pass_point = glm::vec3(.0, 2./3, 0);

  int find_best_possession(Ball &ball) {
    // if noone controls, closest gets the ball
    // if someone controls, closest other than the owner or nothing controls the ball
    glm::vec3 ball_pos = ball.position();
    int owner = ball.owner();
    double best_control_potential = NAN;
    for(int i = 0; i < players.size(); ++i) {
      if(i == ball.owner())continue;
      if(ball.owner() != Ball::NO_OWNER && get_team(ball.owner()) == get_team(i))continue;
      auto &p = players[i];
      double value = p.get_control_potential(ball);
      if(std::isnan(value))continue;
      if(std::isnan(best_control_potential) || best_control_potential > value) {
        best_control_potential = value;
        owner = i;
      }
    }
    // case when the current owner no longer controls the ball
    if(ball.owner() != Ball::NO_OWNER && std::isnan(best_control_potential)) {
      float cur_potential = players[ball.owner()].get_control_potential(ball);
      if(std::isnan(cur_potential)) {
        owner = Ball::NO_OWNER;
        players[ball.owner()].timestamp_lost_ball();
        /* if(state == GameState::RED_THROWIN || state == GameState::BLUE_THROWIN)state=GameState::IN_PROGRESS; */
      }
    }
    return owner;
  }

  void set_control_player(int playerId) {
    // lose the ball if we are a player and we lost it
    if(ball.owner() != Ball::NO_OWNER && playerId != ball.owner()) {
      players[ball.owner()].timestamp_lost_ball();
    }
    // get it if we are player and new owner
    if(ball.owner() != playerId && playerId != Ball::NO_OWNER) {
      players[playerId].timestamp_got_ball();
      ball.last_touched = playerId;
    }
    ball.timestamp_set_owner(playerId);
  }

  Team &get_team(size_t i) {
    return (i < team1.size()) ? team1 : team2;
  }

  int get_pass_destination(int playerId) {
    if(playerId == Ball::NO_OWNER)return Ball::NO_OWNER;
    Team &team = get_team(playerId);

    int teamPlayerId = (team.id() == Team::RED_TEAM) ? playerId : playerId - team1.size();

    int ind_pass_to = Ball::NO_OWNER;
    double range = NAN;
    for(int i = 0; i < team.size(); ++i) {
      if(teamPlayerId == i)continue;
      int dist = Unit::length(team[teamPlayerId].unit.pos - team[i].unit.pos);
      if(std::isnan(range) || dist < range) {
        range = dist;
        ind_pass_to = i;
      }
    }
    return team[ind_pass_to].id();
  }

  void z_perform(int playerId) {
    if(playerId == Ball::NO_OWNER)return;
    auto &p = players[playerId];
    if(p.is_jumping()) {
      through_ball(playerId);
    } else {
      pass_ball(playerId);
    }
  }

  const float pass_speed = .5;
  Unit::vec_t pass_vector{1, 0, .5};
  void pass_ball(int playerId) {
    auto &p = players[ball.owner()];
    if(!p.can_pass())return;
    p.timestamp_passed();
    if(!p.has_ball)return;
    int pass_to = get_pass_destination(ball.owner());
    glm::vec3 dest = (pass_to == Ball::NO_OWNER) ? single_player_pass_point : players[pass_to].possession_point();
    Unit::vec_t dir = dest - p.unit.pos;
    float angle = atan2(dir.y, dir.x);
    p.kick_the_ball(ball, pass_vector * pass_speed, angle);
  }

  const float through_ball_speed = 1.;
  Unit::vec_t through_ball_vector{1, 0, .25};
  void through_ball(int playerId) {
    auto &p = players[playerId];
    if(!p.can_pass())return;
    p.timestamp_passed();
    if(!p.has_ball)return;
    p.kick_the_ball(ball, through_ball_vector * through_ball_speed, p.unit.facing);
  }

  void x_perform(int playerId, float direction) {
    if(playerId == Ball::NO_OWNER)return;
    auto &p = players[playerId];
    if(playerId == ball.owner() && !p.is_sliding()) {
      p.unit.face(direction);
      p.kick_the_ball(ball, .5f * Unit::vec_t(.5, 0, 1), direction);
    } else if(p.can_slide()) {
      p.timestamp_slide();
      Unit::vec_t slide_vec(std::cos(direction), std::sin(direction), 0);
      slide_vec *= p.slide_speed * p.slide_duration;
      p.unit.move(p.unit.pos + slide_vec, p.slide_duration);
    }
  }

  void c_perform(int playerId, float direction) {
    if(playerId == Ball::NO_OWNER)return;
    auto &p = players[playerId];
    if(playerId == ball.owner() && !p.is_jumping()) {
      long_ball(playerId, direction);
    } else {
      auto &p = players[playerId];
      p.unit.face(direction);
    }
  }

  const float cross_speed = 1.;
  Unit::vec_t cross_vector{1.25, 0, 1};
  void long_ball(int playerId, float cross_direction) {
    auto &p = players[playerId];
    p.unit.face(cross_direction);
    p.kick_the_ball(ball, cross_speed * cross_vector, cross_direction);
  }

  void v_perform(int playerId) {
    players[0].jump();
  }

  bool throw_in_check() {
    auto bpos = ball.position();
    if(pitch.playable_area().contains(bpos.x, bpos.y))return false;
    if(ball.last_touched == Ball::NO_OWNER || get_team(ball.last_touched).id() == Team::RED_TEAM)state = GameState::RED_THROWIN;
    else state = GameState::BLUE_THROWIN;
    ball.velocity() = Unit::vec_t(0, 0, 0);
    ball.position().z = 0;
  }
};
