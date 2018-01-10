#pragma once

#include <vector>
#include <mutex>

#include "Ball.hpp"
#include "Player.hpp"
#include "Timer.hpp"

struct Team;

struct Soccer {
  std::vector<Player> players;
  Ball ball;

  std::mutex mtx;

  Soccer(size_t team1sz=1, size_t team2sz=2):
    players(),
    team1(*this, team1sz, Team::RED_TEAM),
    team2(*this, team2sz, Team::BLUE_TEAM)
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
    set_timer();
  }

  void set_timer() {
    ball.set_timer();
    for(auto &p : players) {
      p.set_timer();
    }
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

  void idle(Timer::time_t curtime) {
    std::lock_guard<std::mutex> guard(mtx);
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

  bool is_active_player(int playerId) const {
    return playerId != Ball::NO_OWNER;
  }

  const Unit::loc_t single_player_pass_point = glm::vec3(.0, 2./3, 0);

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
        int pass_to = get_pass_destination(p.id());
        if(is_active_player(pass_to)) {
          dest = players[pass_to].possession_point();
        } else {
          dest = single_player_pass_point;
        }
        float dist = glm::distance(ball.unit.pos, dest);
        float time = std::sqrt(2 * ball.unit.height() / ball.G) * .1;
        float speed = std::fmax(ball.unit.moving_speed, 350 * Unit::GAUGE);

        if(dist < speed * time) {
          ball.vertical_speed = 0;
          speed = dist / time;
        } else {
          ball.vertical_speed = std::fmin(10. * Unit::GAUGE, 10. * Unit::GAUGE * ball.G * .5 * dist / speed);
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

  const Player &get_player(int playerId) const {
    ASSERT(is_active_player(playerId));
    return players[playerId];
  }

  int get_pass_destination(int playerId) {
    if(!is_active_player(playerId))return playerId;
    Team &team = get_team(playerId);
    int ind_pass_to = Ball::NO_OWNER;
    double range = NAN;
    for(int i = 0; i < team.size(); ++i) {
      if(playerId == team[i].id())continue;
      float dist = glm::distance(ball.unit.pos, team[i].unit.pos);
      if(std::isnan(range) || dist < range) {
        range = dist;
        ind_pass_to = team[i].id();
      }
    }
    return ind_pass_to;
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
      p.kick_the_ball(ball, 300. * Unit::GAUGE, 20. * Unit::GAUGE, direction);
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
      float vspeed = 30. * Unit::GAUGE;
      float speed = std::min(522.f * Unit::GAUGE, 5.f * p.G * dist / vspeed);
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
      p.jump(15. * Unit::GAUGE);
    } else {
      p.jump(20. * Unit::GAUGE);
    }
  }

  void f_action(int playerId, float direction) {
    if(!is_active_player(playerId))return;
    auto &p = get_player(playerId);
    p.unit.face(direction);
  }

  void s_action(int playerId) {
    if(!is_active_player(playerId))return;
    auto &p = get_player(playerId);
    p.unit.stop();
  }

  void m_action(int playerId, Unit::loc_t dest) {
    if(!is_active_player(playerId))return;
    auto &p = get_player(playerId);
    p.unit.move(dest);
  }
};
