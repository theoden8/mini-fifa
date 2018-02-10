#pragma once

#include <cstdint>
#include <cstdio>
#include <climits>
#include <map>
#include <deque>
#include <chrono>
#include <limits>

#include "Logger.hpp"
#include "Debug.hpp"

struct Timer {
  using time_t = double;
  using key_t = uint32_t;

  static constexpr key_t CURRENT_TIME = std::numeric_limits<key_t>::max();

  static constexpr time_t time_start() {
    return .0;
  }

  static time_t system_time() {
    static std::mutex mtx;
    static auto systime_start = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> guard(mtx);
    auto systime_now = std::chrono::system_clock::now();
    return 1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(systime_now - systime_start).count();
  }

  time_t prev_time = time_start();
  time_t current_time = time_start();
  std::map<key_t, time_t> events;
  std::map<key_t, std::deque<time_t>> event_counters;
  std::map<key_t, time_t> timeouts;

  Timer()
  {}

  /* void dump_times() { */
  /*   for(const auto &[k,t] : events){Logger::Info("%d: %f\n",k,t);} */
  /* } */

  void set_time(time_t curtime) {
    prev_time = current_time;
    current_time = curtime;
  }

  void set_event(key_t key) {
    ASSERT(key != CURRENT_TIME);
    events[key] = current_time;
  }

  time_t elapsed(key_t key=CURRENT_TIME) const {
    /* Logger::Info("elapsed access: %d\n", key); */
    if(key == CURRENT_TIME)return current_time - prev_time;
    return current_time - events.at(key);
  }

  void set_event_counter(key_t key) {
    event_counters[key].push_back(key);
  }

  time_t difference(key_t key) const {
    return event_counters.at(key).back() - event_counters.at(key).front();
  }

  int get_count(key_t key) {
    while(difference(key) > timeouts[key]) {
      event_counters[key].pop_front();
    }
    return event_counters[key].size();
  }

  void set_timeout(key_t key, time_t timeout) {
    if(events.find(key) == events.end()) {
      set_event(key);
    }
    timeouts[key] = timeout;
  }

  bool timed_out(key_t key) const {
    /* Logger::Info("timeout access: %d\n", key); */
    time_t timeout = .0;
    if(events.find(key) == events.end()) {
      return true;
    }
    if(timeouts.find(key) != timeouts.end()) {
      timeout = timeouts.at(key);
    }
    return elapsed(key) > timeout;
  }

  template <typename F>
  void periodic(key_t key, F &&func) {
    if(timed_out(key)) {
      set_event(key);
      func();
    }
  }

  void erase(key_t key) {
    if(events.find(key) != events.end()) {
      events.erase(key);
    }
    if(event_counters.find(key) != event_counters.end()) {
      event_counters.erase(key);
    }
    if(timeouts.find(key) != timeouts.end()) {
      timeouts.erase(key);
    }
  }
};
