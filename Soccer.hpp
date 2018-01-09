#pragma once

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/intersect.hpp>

#include "Transformation.hpp"
#include "Camera.hpp"
#include "Ball.hpp"
#include "Player.hpp"
#include "Pitch.hpp"
#include "Post.hpp"
#include "Cursor.hpp"

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

  enum class CursorState {
    DEFAULT,
    X_AIM,
    C_AIM,
    F_AIM
  };
  CursorState cursorState = CursorState::DEFAULT;
  void keyboard(int key) {
    ball.keyboard(key);
    players[0].keyboard(key);
    if(key == GLFW_KEY_Z) {
      z_action(ball.owner());
    } else if(key == GLFW_KEY_X) {
      cursorState = CursorState::X_AIM;
    } else if(key == GLFW_KEY_C) {
      cursorState = CursorState::C_AIM;
    } else if(key == GLFW_KEY_V) {
      v_action(0);
    } else if(key == GLFW_KEY_F) {
      cursorState = CursorState::F_AIM;
    } else if(key == GLFW_KEY_ESCAPE) {
      if(cursorState != CursorState::DEFAULT) {
        cursorState = CursorState::DEFAULT;
      }
    }
  }

  void set_cursor(Cursor &cursor, float m_x, float m_y, float width, float height, Camera &cam) {
    float s_x = m_x * 2 - 1, s_y = m_y * 2 - 1;
    glm::vec4 screenPos = glm::vec4(s_x, -s_y, 1.f, 1.f);
    glm::vec4 worldPos = glm::inverse(cam.get_matrix()) * screenPos;
    glm::vec3 dir = glm::normalize(glm::vec3(worldPos));
    float dist;
    bool intersects = glm::intersectRayPlane(
      cam.cameraPos,
      dir,
      glm::vec3(0, 0, 0),
      glm::vec3(0, 0, 1),
      dist
    );
    glm::vec3 pos = cam.cameraPos + dir * dist;
    cursorPoint = glm::vec2(pos.x, pos.y);
    /* printf("cursor (%f %f) -> (%f %f %f)\n", s_x, s_y, pos.x,pos.y, pos.z); */
    /* ball.position() = glm::vec3(cursorPoint.x, cursorPoint.y, ball.unit.height()); */
    if(cursorState == CursorState::DEFAULT) {
      cursor.state = Cursor::State::POINTER;
    } else {
      cursor.state = Cursor::State::SELECTOR;
    }
  }

  void mouse_click(int button, int action) {
    auto &p = players[0];
    glm::vec3 cpos(cursorPoint.x, cursorPoint.y, 0);
    if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
      if(cursorState == CursorState::DEFAULT) {
        p.unit.move(cpos);
      } else {
        cursorState = CursorState::DEFAULT;
      }
    } else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      switch(cursorState) {
        case CursorState::DEFAULT:
        break;
        case CursorState::X_AIM:
          x_action(p.id(), p.unit.facing_angle(cpos));
          cursorState = CursorState::DEFAULT;
        break;
        case CursorState::C_AIM:
          c_action(p.id(), cpos);
          cursorState = CursorState::DEFAULT;
        break;
        case CursorState::F_AIM:
          f_action(p.id(), p.unit.facing_angle(cpos));
          cursorState = CursorState::DEFAULT;
        break;
      }
    }
  }

  void display(Camera &cam) {
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
  glm::vec2 cursorPoint;

  void idle(double curtime) {
    timer.set_time(curtime);
    idle_control();
    ball.idle(timer.current_time);
    for(auto &p: players) {
      p.idle(timer.current_time);
    }
  }

  void idle_control() {
    if(is_active_player(ball.owner()) && !ball.is_loose()) {
      auto &p = get_player(ball.owner());
      /* printf("controlling fully\n"); */
      ball.position() = p.unit.point_offset(p.possession_offset);
      ball.position().z = p.unit.height() + ball.default_height;
    }
    int new_owner = find_best_possession(ball);
    set_control_player(new_owner);
  }

  bool is_active_player(int playerId) {
    return playerId != Ball::NO_OWNER;
  }

  const Unit::vec_t single_player_pass_point = glm::vec3(.0, 2./3, 0);

  bool is_able_to_tackle(float val) {
    return !std::isnan(val);
  }
  int find_best_possession(Ball &ball) {
    // if noone controls, closest gets the ball
    // if someone controls, closest other than the owner or nothing controls the ball
    Unit::loc_t ball_pos = ball.position();
    int owner = ball.owner();
    double bcp = NAN; // best control potential
    for(auto &p : players) {
      if(p.is_owner(ball))continue;
      if(is_active_player(ball.owner()) && get_team(ball.owner()).id() == p.team)continue;
      double pcp = p.get_control_potential(ball);
      if(!is_able_to_tackle(pcp))continue;
      if(!is_able_to_tackle(bcp) || bcp > pcp) {
        bcp = pcp;
        owner = p.id();
      }
    }
    // case when the current owner no longer controls the ball
    if(is_active_player(ball.owner()) && !is_able_to_tackle(bcp)) {
      auto &p = get_player(ball.owner());
      double pcp = p.get_control_potential(ball);
      if(!is_able_to_tackle(pcp)) {
        owner = Ball::NO_OWNER;
      }
    }
    return owner;
  }

  void set_control_player(int playerId) {
    // lose the ball if we are a player and we lost it
    if(is_active_player(ball.owner())) {
      auto &p = get_player(ball.owner());
      if(!is_active_player(playerId)) {
        p.timestamp_dispossess(ball, Player::CANT_HOLD_BALL_SHOT);
      } else if(playerId != ball.owner()) {
        p.timestamp_dispossess(ball, Player::CANT_HOLD_BALL_DISPOSSESS);
      } else if(players[playerId].is_sliding_fast()) {
        p.timestamp_slowdown(Player::SLOWDOWN_SLID);
      }
    }
    // get it if we are player and new owner
    if(ball.owner() != playerId && is_active_player(playerId)) {
      auto &p = get_player(playerId);
      p.timestamp_got_ball(ball);
      if(p.is_sliding_fast()) {
        p.timestamp_dispossess(ball, Player::CANT_HOLD_BALL_SHOT);
        ball.unit.face(p.unit.facing_angle(p.unit.dest));
        ball.unit.moving_speed = p.unit.moving_speed;
      } else if(p.is_going_up()) {
        Unit::loc_t dest = single_player_pass_point;
        int pass_to = get_pass_destination(ball.owner());
        if(is_active_player(pass_to)) {
          dest = players[pass_to].possession_point();
        }
        float dist = glm::distance(p.unit.pos, dest);
        float time = std::sqrt(2 * ball.unit.height() / p.G) * .1;
        float speed = std::fmax(ball.unit.moving_speed, p.running_speed * 1.2);

        if(dist < speed * time) {
          ball.vertical_speed = 0;
          speed = dist/time;
        } else {
          ball.vertical_speed = std::fmin(.01*p.tallness, .01*p.tallness*p.G*.05/speed);
        }
        ball.unit.moving_speed = speed;
        ball.is_in_air = true;
        ball.unit.facing_dest = ball.unit.facing_angle(dest);
        p.timestamp_dispossess(ball, Player::CANT_HOLD_BALL_SHOT);
      }
    }
  }

  Team &get_team(int playerId) {
    return (playerId < team1.size()) ? team1 : team2;
  }

  Player &get_player(int playerId) {
    ASSERT(is_active_player(playerId));
    return players[playerId];
  }

  int get_pass_destination(int playerId) {
    if(!is_active_player(playerId))return playerId;
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

  void z_action(int playerId) {
    if(!is_active_player(playerId))return;
    auto &p = get_player(playerId);
    if(!p.can_pass())return;
    p.timestamp_passed();
    if(!p.is_owner(ball))return;
    if(!p.is_jumping()) {
      ball.is_in_air = false;
      Unit::loc_t dest = single_player_pass_point;
      int pass_to = get_pass_destination(ball.owner());
      if(is_active_player(pass_to)) {
        dest = players[pass_to].possession_point();
      }
      p.kick_the_ball(ball, p.running_speed * 1.8, .0, ball.unit.facing_angle(dest));
    } else {
      ball.is_in_air = true;
      p.kick_the_ball(ball, p.running_speed * 1.8, .0, p.unit.facing);
    }
  }

  void x_action(int playerId, float direction) {
    if(!is_active_player(playerId))return;
    auto &p = get_player(playerId);
    if(playerId == ball.owner() && !p.is_sliding()) {
      ball.is_in_air = true;
      p.unit.face(direction);
      p.kick_the_ball(ball, p.running_speed * 1.25, .2 * p.tallness, direction);
    } else if(p.can_slide()) {
      p.timestamp_slide();
      Unit::vec_t slide_vec(std::cos(direction), std::sin(direction), 0);
      slide_vec *= p.slide_speed * p.slide_duration;
      p.unit.slide(p.unit.pos + slide_vec, p.slide_duration);
    }
  }

  void c_action(int playerId, Unit::loc_t dest) {
    if(!is_active_player(playerId))return;
    auto &p = get_player(playerId);
    float direction = p.unit.facing_angle(dest);
    if(playerId == ball.owner() && !p.is_jumping()) {
      ball.reset_height();
      ball.is_in_air = true;
      p.unit.face(direction);
      float dist = glm::distance(ball.unit.pos, dest);
      float vspeed = .3 * p.tallness;
      float speed = std::min(p.running_speed * 1.8f, 5.f * p.G * dist / vspeed);
      p.kick_the_ball(ball, speed, vspeed, direction);
      p.timestamp_slowdown(Player::SLOWDOWN_SHOT);
    } else {
      p.unit.face(direction);
    }
  }

  void v_action(int playerId) {
    if(!is_active_player(playerId))return;
    auto &p = get_player(playerId);
    if(p.is_owner(ball)) {
      p.jump(.15 * p.tallness);
    } else {
      p.jump(.2 * p.tallness);
    }
  }

  void f_action(int playerId, float direction) {
    if(!is_active_player(playerId))return;
    auto &p = get_player(playerId);
    p.unit.face(direction);
  }
};
