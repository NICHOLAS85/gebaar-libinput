/*
    gebaar
    Copyright (C) 2019   coffee2code

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SRC_IO_GEBAAR_INPUT_HPP_
#define SRC_IO_GEBAAR_INPUT_HPP_

#include <fcntl.h>
#include <libinput.h>
#include <math.h>
#include <poll.h>
#include <zconf.h>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include "../config/config.h"
#define FN "input"
#define THRESH 100

#define DEFAULT_SCALE 1.0
#define SWIPE_X_THRESHOLD 1000
#define SWIPE_Y_THRESHOLD 500

namespace gebaar::io {
struct gesture_swipe_event {
  int fingers;
  double x;
  double y;

  bool executed;
  bool continuous;
};

struct gesture_pinch_event {
  int fingers;
  double scale;
  double angle;

  bool executed;
  bool continuous;
  bool rotating;
  int step;
};

struct touch_swipe_event {
  size_t fingers;
  double x;
  double y;
  std::map<size_t, std::pair<double, double>> prev_xy;
  std::map<size_t, std::pair<double, double>> delta_xy;
  std::vector<std::pair<size_t, double>> down_slots;
  std::vector<std::pair<size_t, double>> up_slots;
};
class Input {
 public:
  explicit Input(std::shared_ptr<gebaar::config::Config> const& config_ptr);

  ~Input();

  bool initialize();

  void start_loop();

 private:
  std::shared_ptr<gebaar::config::Config> config;
  std::string swipe_event_group;
  struct libinput* libinput;
  struct libinput_event* libinput_event;
  struct udev* udev;

  struct gesture_swipe_event gesture_swipe_event;
  struct gesture_pinch_event gesture_pinch_event;
  struct touch_swipe_event touch_swipe_event;

  bool initialize_context();

  bool gesture_device_exists();

  bool check_chosen_event(std::string ev);

  static int open_restricted(const char* path, int flags,
                             __attribute__((unused)) void* user_data) {
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
  }

  static void close_restricted(int fd,
                               __attribute__((unused)) void* user_data) {
    close(fd);
  }

  constexpr static struct libinput_interface libinput_interface = {
      open_restricted, close_restricted};

  void check_multitouch_down_up(std::vector<std::pair<size_t, double>> slots);

  void apply_swipe(size_t swipe_type, size_t fingers, std::string type);

  size_t get_swipe_type(double sdx, double sdy);
  /*
   * Decrements step of current trigger. Just to skip 0
   * @param cur current step
   */
  inline void dec_step(int* cur) { --*cur == 0 ? --cur : cur; }

  /*
   * Increase step of current trigger. Just to pass -1
   * @param cur current step
   */
  inline void inc_step(int* cur) { ++*cur == 0 ? ++cur : cur; }

  void handle_event();

  /* Swipe event */
  void reset_swipe_event();

  void handle_swipe_event_without_coords(libinput_event_gesture* gev,
                                         bool begin);

  void handle_swipe_event_with_coords(libinput_event_gesture* gev);

  void handle_touch_event_motion(libinput_event_touch* tev);

  void handle_touch_event_down(libinput_event_touch* tev);

  void handle_touch_event_up(libinput_event_touch* tev);

  void trigger_swipe_command();

  double get_swipe_length(double sdx, double sdy);

  bool test_above_threshold(size_t swipe_type, double length,
                            libinput_device* dev);

  /* Pinch event */
  void reset_pinch_event();

  void handle_one_shot_pinch(double new_scale);

  void handle_continuous_pinch(double new_scale);

  void handle_one_shot_rotate(double new_angle);

  void handle_continuous_rotate(double new_angle);

  void handle_pinch_event(libinput_event_gesture* gev, bool begin);

  void handle_switch_event(libinput_event_switch* gev);
};
}  // namespace gebaar::io

#endif  // SRC_IO_GEBAAR_INPUT_HPP_
