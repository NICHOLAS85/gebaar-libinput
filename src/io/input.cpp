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

#include "input.h"
#include <poll.h>

bool runproc(const char* cmdline) {
  if (strlen(cmdline) > 0) {
    spdlog::get("main")->info(
        "[{}] at {} - {} - Executing '{}'",
        FN, __LINE__, __func__, cmdline);
    int status = std::system(cmdline);
    if (status != 0) {
      spdlog::get("main")->warn("{} -> Non-zero exit code: {}", cmdline,
                                 WEXITSTATUS(status));
    }
    return true;
  } else {
    return false;
  }
}

/**
 * Input system constructor, we pass our Configuration object via a shared
 * pointer
 *
 * @param config_ptr shared pointer to configuration object
 */
gebaar::io::Input::Input(
    std::shared_ptr<gebaar::config::Config> const& config_ptr) {
  config = config_ptr;
  gesture_swipe_event = {};
  touch_swipe_event = {};
  gesture_pinch_event = {};
  gesture_pinch_event.scale = DEFAULT_SCALE;
}

/**
 * Initialize the libinput context
 *
 * @return bool
 */
bool gebaar::io::Input::initialize_context() {
  udev = udev_new();
  libinput = libinput_udev_create_context(&libinput_interface, nullptr, udev);
  return libinput_udev_assign_seat(libinput, "seat0") == 0;
}

size_t gebaar::io::Input::get_swipe_type(double sdx, double sdy) {
  double x = sdx;
  double y = sdy;
  size_t swipe_type = 5;               // middle = no swipe
                                       // 1 = left_up, 2 = up, 3 = right_up...
                                       // 1 2 3
                                       // 4 5 6
                                       // 7 8 9
  const double OBLIQUE_RATIO = 0.414;  // =~ tan(22.5);

  if (abs(x) > abs(y)) {
    // left or right swipe
    swipe_type += x < 0 ? -1 : 1;

    // check for oblique swipe
    if (abs(y) / abs(x) > OBLIQUE_RATIO) {
      swipe_type += y < 0 ? -3 : 3;
    }
  } else {
    // up of down swipe
    swipe_type += y < 0 ? -3 : 3;

    // check for oblique swipe
    if (abs(x) / abs(y) > OBLIQUE_RATIO) {
      swipe_type += x < 0 ? -1 : 1;
    }
  }
  return swipe_type;
}

bool gebaar::io::Input::test_above_threshold(size_t swipe_type, double length,
                                             libinput_device* dev) {
  double w = 0;
  double h = 0;
  libinput_device_get_size(dev, &w, &h);

  size_t dim;
  if (swipe_type % 2 != 0) {
    double d = hypot(w, h);
    dim = d * config->settings.touch_longswipe_screen_percentage / 100;
  } else if (swipe_type < 4 || swipe_type > 6) {
    dim = h * config->settings.touch_longswipe_screen_percentage / 100;
  } else {
    dim = w * config->settings.touch_longswipe_screen_percentage / 100;
  }

  spdlog::get("main")->debug(
      "percentage {}, required length {}, actual length {}",
      config->settings.touch_longswipe_screen_percentage, dim, length);
  return (length > dim);
}

void gebaar::io::Input::apply_swipe(size_t swipe_type, size_t fingers, std::string type) {
  std::string command;
  command = config->get_swipe_command(fingers, type, swipe_type, true);

  spdlog::get("main")->debug(
      "[{}] at {} - {} - fingers: {}, type: {}, gesture: {} ... ",
      FN, __LINE__, __func__, fingers, type,
      config->get_swipe_type_name(swipe_type));
  if (runproc(command.c_str())) {
    gesture_swipe_event.executed = true;
  } else {
    command = config->get_swipe_command(fingers, type, swipe_type, false);
    if (! runproc(command.c_str())){
      gesture_swipe_event.executed = true;
    }
  }
}

/**
 * This function is used to decide how many fingers are used for the touch swipe
 * gesture Each swipe must ensure 1) The touch down of all fingers occur in x
 * amount of time from the previous finger 2) The lift up of all fingers occur
 * in x amount of time from the previous finger
 *
 * Any fingers touched down or lifted outside of x makes the event 'unclean' and
 * therefore, swiping fails
 *
 * @param slots Represents pair(finger, timeoftouch(down/lift))
 */
void gebaar::io::Input::check_multitouch_down_up(
    std::vector<std::pair<size_t, double>> slots) {
  if (slots.size() > 1) {
    std::vector<std::pair<size_t, double>>::reverse_iterator iter =
        slots.rbegin();
    std::vector<std::pair<size_t, double>>::reverse_iterator prev_iter =
        std::next(iter, 1);
    double timebetweenslots = iter->second - prev_iter->second;
    if (timebetweenslots <= THRESH) {
      touch_swipe_event.fingers = slots.size();
    }
  } else {
    touch_swipe_event.fingers = slots.size();
  }
}

/**
 * This event occurs when a finger touches the touchscreen
 * It passes a list of pairs (slot_id, timestamp) to check_multitouch_down_up
 * Each slot corresponds to a finger touched down on the screen
 *
 * @param tev Touch Event
 */
void gebaar::io::Input::handle_touch_event_down(libinput_event_touch* tev) {
  touch_swipe_event.down_slots.push_back(std::pair<size_t, double>(
      libinput_event_touch_get_slot(tev), libinput_event_touch_get_time(tev)));
  check_multitouch_down_up(touch_swipe_event.down_slots);
}

/**
 * This event occurs when a finger lifts up from the touchscreen
 * It passes a list of pairs (slot_id, timestamp) to check_multitouch_down_up
 * Each slot corresponds to a finger lifted from the screen
 *
 * If all the fingers are lifted, we check the swipe type of all fingers,
 * If all fingers swipe in the same direction, success
 *
 * @param tev Touch Event
 */
void gebaar::io::Input::handle_touch_event_up(libinput_event_touch* tev) {
  touch_swipe_event.up_slots.push_back(std::pair<size_t, double>(
      libinput_event_touch_get_slot(tev), libinput_event_touch_get_time(tev)));
  check_multitouch_down_up(touch_swipe_event.up_slots);

  bool a =
      touch_swipe_event.up_slots.size() == touch_swipe_event.down_slots.size();

  if (a) {
    std::vector<size_t> swipes;
    size_t swipe_type;
    size_t slot;
    double dx;
    double dy;
    double swipe_length;
    for (auto iter = touch_swipe_event.delta_xy.begin();
         iter != touch_swipe_event.delta_xy.end(); iter++) {
      slot = iter->first;
      dx = iter->second.first;
      dy = iter->second.second;
      swipe_length = get_swipe_length(dx, dy);
      libinput_device* dev =
          libinput_event_get_device(libinput_event_touch_get_base_event(tev));
      swipe_type = get_swipe_type(dx, dy);

      if (touch_swipe_event.fingers == 1) {
        if (!test_above_threshold(swipe_type, swipe_length, dev)) {
          spdlog::get("main")->debug("swipe not above threshold");
          break;
        } else {
          spdlog::get("main")->debug("swipe above threshold");
        }
      }

      spdlog::get("main")->debug(
          "[{}] at {} - {}, slot: {}, swipe-type: {}, length: {}", FN, __LINE__,
          __func__, slot, config->get_swipe_type_name(swipe_type),
          swipe_length);

      if (!swipes.empty() && swipe_type != swipes.back()) {
        break;
      }

      swipes.push_back(swipe_type);
    }

    /*
      1) Check number of down slots equals
      calculated number of fingers (check_multi_touch_downup). This prevents
      swipes when fingers are added too late or lifted too early

      2) Check number down slots equals number of touches sensed moving across
      the screen. This prevents swipes where the fingers are lifted and placed
      back on the screen before the touch_swipe_event structure is refreshed
      (causing additional downslots)

      3) Check number of valid swipes (each finger of multi touch
      swipe) equals calculated number of fingers. This only allows swipes
      where all swiping fingers are going in the same direction
    */
    bool is_valid_gesture = true;
    is_valid_gesture =
        (is_valid_gesture &&
         (touch_swipe_event.down_slots.size() == touch_swipe_event.fingers));
    if (!is_valid_gesture) {
      spdlog::get("main")->info("down slots do not match number of fingers");
    } else {
      is_valid_gesture =
          (is_valid_gesture && (touch_swipe_event.down_slots.size() ==
                                touch_swipe_event.delta_xy.size()));
      if (!is_valid_gesture) {
        spdlog::get("main")->info("down slots do not match motion slots");
      } else {
        is_valid_gesture =
            (is_valid_gesture && (swipes.size() == touch_swipe_event.fingers));
        if (!is_valid_gesture) {
          spdlog::get("main")->info(
              "number of valid swipes {} do not match number of fingers {}",
              swipes.size(), touch_swipe_event.fingers);
        } else {
          apply_swipe(swipe_type, touch_swipe_event.fingers, swipe_event_group);
        }
      }
    }

    spdlog::get("main")->debug(
        "[{}] at {} - {}, fgrs: {}, d-slts: {}, u-slts: {}, d-xy: {}, "
        "prv-xy: "
        "{}",
        FN, __LINE__, __func__, touch_swipe_event.fingers,
        touch_swipe_event.down_slots.size(), touch_swipe_event.up_slots.size(),
        touch_swipe_event.delta_xy.size(), touch_swipe_event.prev_xy.size());
    touch_swipe_event = {};
    spdlog::get("main")->debug("[{}] at {} - {}: touch gesture finished\n\n",
                               FN, __LINE__, __func__);
  }
}

double gebaar::io::Input::get_swipe_length(double sdx, double sdy) {
  return sqrt(pow(sdx, 2) + pow(sdy, 2) * 1.0);
}

/**
 * This event occurs when a finger moves on the touchscreen
 * mimics handle_swipe_event_with_coords but for multiple tracks (each touched
 * down finger)
 *
 * libinput touch event has no get_dx, get_dy functions. Store previous
 * coordinates to acquire dx and dy
 *
 * @param tev Touch Event
 */
void gebaar::io::Input::handle_touch_event_motion(libinput_event_touch* tev) {
  if (touch_swipe_event.delta_xy.find(libinput_event_touch_get_slot(tev)) ==
      touch_swipe_event.delta_xy.end()) {
    touch_swipe_event.delta_xy.insert(
        std::pair<size_t, std::pair<double, double>>(
            libinput_event_touch_get_slot(tev), {0, 0}));
    touch_swipe_event.prev_xy.insert(
        std::pair<size_t, std::pair<double, double>>(
            libinput_event_touch_get_slot(tev),
            {libinput_event_touch_get_x(tev),
             libinput_event_touch_get_y(tev)}));
  } else {
    std::pair<double, double> prevcoord =
        touch_swipe_event.prev_xy.find(libinput_event_touch_get_slot(tev))
            ->second;
    double prevx = prevcoord.first;
    double prevy = prevcoord.second;
    touch_swipe_event.delta_xy.find(libinput_event_touch_get_slot(tev))
        ->second.first += (libinput_event_touch_get_x(tev) - prevx);
    touch_swipe_event.delta_xy.find(libinput_event_touch_get_slot(tev))
        ->second.second += (libinput_event_touch_get_y(tev) - prevy);
    touch_swipe_event.prev_xy.find(libinput_event_touch_get_slot(tev))->second =
        {libinput_event_touch_get_x(tev), libinput_event_touch_get_y(tev)};
    spdlog::get("main")->debug(
        "[{}] at {} - {} dx: {} , dy: {}", FN, __LINE__, __func__,
        touch_swipe_event.delta_xy.find(libinput_event_touch_get_slot(tev))
            ->second.first,
        touch_swipe_event.delta_xy.find(libinput_event_touch_get_slot(tev))
            ->second.second);
  }
}

/**
 * Reset swipe event struct to defaults
 */
void gebaar::io::Input::reset_swipe_event() {
  gesture_swipe_event = {};
}

/**
 * Reset pinch event struct to defaults
 */
void gebaar::io::Input::reset_pinch_event() {
  gesture_pinch_event = {};
  gesture_pinch_event.scale = DEFAULT_SCALE;
  gesture_pinch_event.executed = false;
  gesture_pinch_event.continuous = false;
  gesture_pinch_event.rotating = false;
  gesture_pinch_event.angle = 0;
}

/**
 * Pinch one_shot gesture handle
 * @param new_scale last reported scale between the fingers
 */
void gebaar::io::Input::handle_one_shot_pinch(double new_scale) {
  if (new_scale > gesture_pinch_event.scale) {  // Scale up
    spdlog::get("main")->debug("[{}] at {} - {}: Scale up", FN, __LINE__,
                               __func__);
    // Add 1 to required distance to get 2 > x > 1
    if (new_scale > 1 + config->settings.pinch_threshold) {
      std::string command = config->get_pinch_command(gesture_pinch_event.fingers, "ONESHOT", 2);
      spdlog::get("main")->debug(
          "[{}] at {} - {} - fingers: {}, type: ONESHOT, gesture: PINCH OUT ... ",
          FN, __LINE__, __func__, gesture_pinch_event.fingers);
      if (runproc(command.c_str())) {
        gesture_pinch_event.executed = true;
      } else {
        inc_step(&gesture_pinch_event.step);
        handle_continuous_pinch(new_scale);
        gesture_pinch_event.continuous = true;
      }
    }
  } else {  // Scale Down
    spdlog::get("main")->debug("[{}] at {} - {}: Scale down {} < 1 - {}", FN, __LINE__,
                               __func__, new_scale, config->settings.pinch_threshold);
    // Substract from 1 to have inverted value for pinch in gesture
    if (gesture_pinch_event.scale < 1 - config->settings.pinch_threshold) {
      std::string command = config->get_pinch_command(gesture_pinch_event.fingers, "ONESHOT", 1);
      spdlog::get("main")->debug(
          "[{}] at {} - {} - fingers: {}, type: ONESHOT, gesture: PINCH IN ... ",
          FN, __LINE__, __func__, gesture_pinch_event.fingers);
      if (runproc(command.c_str())) {
        gesture_pinch_event.executed = true;
      } else {
        dec_step(&gesture_pinch_event.step);
        handle_continuous_pinch(new_scale);
        gesture_pinch_event.continuous = true;
      }
    }
  }
}

/**
 * Pinch continuous gesture handle
 * Calculates the trigger value according to current step
 * @param new_scale last reported scale between the fingers
 */
void gebaar::io::Input::handle_continuous_pinch(double new_scale) {
  int step = gesture_pinch_event.step == 0 ? gesture_pinch_event.step + 1
                                           : gesture_pinch_event.step;
  double trigger = 1 + (config->settings.pinch_threshold * step);
  spdlog::get("main")->debug(
      "[{}] at {} - {} - scale: {} gesture_scale: {} trigger: {}",
      FN, __LINE__, __func__, new_scale, gesture_pinch_event.scale, trigger);
  if (new_scale > gesture_pinch_event.scale) {  // Scale up
    spdlog::get("main")->debug("[{}] at {} - {}: Scale up", FN, __LINE__,
                               __func__);
    if (new_scale >= trigger) {
      std::string command = config->get_pinch_command(gesture_pinch_event.fingers, "CONTINUOUS", 2);
      spdlog::get("main")->debug(
          "[{}] at {} - {} - fingers: {}, type: CONTINUOUS, gesture: PINCH OUT ... ",
          FN, __LINE__, __func__, gesture_pinch_event.fingers);
      if (runproc(command.c_str())) {
        inc_step(&gesture_pinch_event.step);
      } else {
        gesture_pinch_event.executed = true;
      }
    }
  } else {  // Scale down
    spdlog::get("main")->debug("[{}] at {} - {}: Scale down", FN, __LINE__,
                               __func__);
    if (new_scale <= trigger) {
      std::string command = config->get_pinch_command(gesture_pinch_event.fingers, "CONTINUOUS", 1);
      spdlog::get("main")->debug(
          "[{}] at {} - {} - fingers: {}, type: CONTINUOUS, gesture: PINCH IN ... ",
          FN, __LINE__, __func__, gesture_pinch_event.fingers);
      if (runproc(command.c_str())) {
        dec_step(&gesture_pinch_event.step);
      } else {
        gesture_pinch_event.executed = true;
      }
    }
  }
}

/**
 * Rotate one_shot gesture handle
 * @param new_angle last reported angle between fingers
 */
void gebaar::io::Input::handle_one_shot_rotate(double new_angle) {
  if (gesture_pinch_event.executed) { // A pinch may have already triggered
    return;
  }
  spdlog::get("main")->debug("[{}] at {} - {}: gpe_angle: {} new_angle: {}", FN, __LINE__,
                             __func__, gesture_pinch_event.angle, new_angle);
  if (new_angle > gesture_pinch_event.angle) { // Rotate right
    spdlog::get("main")->debug("[{}] at {} - {}: Rotate right", FN, __LINE__,
                               __func__);
    if (new_angle > config->settings.rotate_threshold) {
      std::string command = config->get_pinch_command(gesture_pinch_event.fingers, "ONESHOT", 4);
      spdlog::get("main")->debug(
          "[{}] at {} - {} - fingers: {}, type: ONESHOT, gesture: ROTATE RIGHT ... ",
          FN, __LINE__, __func__, gesture_pinch_event.fingers);
      if (runproc(command.c_str())) {
        gesture_pinch_event.executed = true;
      } else {
        inc_step(&gesture_pinch_event.step);
        handle_continuous_rotate(new_angle);
        gesture_pinch_event.continuous = true;
        gesture_pinch_event.rotating = true;
      }
    }
  } else { // Rotate left
    spdlog::get("main")->debug("[{}] at {} - {}: Rotate left", FN, __LINE__,
                               __func__);
    if (abs(new_angle) > config->settings.rotate_threshold) {
      std::string command = config->get_pinch_command(gesture_pinch_event.fingers, "ONESHOT", 3);
      spdlog::get("main")->debug(
          "[{}] at {} - {} - fingers: {}, type: ONESHOT, gesture: ROTATE LEFT ... ",
          FN, __LINE__, __func__, gesture_pinch_event.fingers);
      if (runproc(command.c_str())) {
        gesture_pinch_event.executed = true;
      } else {
        dec_step(&gesture_pinch_event.step);
        handle_continuous_rotate(new_angle);
        gesture_pinch_event.continuous = true;
        gesture_pinch_event.rotating = true;
      }
    }
  }
}

/**
 * Rotate continuous gesture handle
 * Calculates the trigger value according to current step
 * @param new_angle last reported angle between fingers
 */
void gebaar::io::Input::handle_continuous_rotate(double new_angle) {
  int step = gesture_pinch_event.step == 0 ? gesture_pinch_event.step + 1
                                           : gesture_pinch_event.step;
  double trigger = config->settings.rotate_threshold * step;
  spdlog::get("main")->debug(
      "[{}] at {} - {} - scale: {} gesture_scale: {} trigger: {}",
      FN, __LINE__, __func__, new_angle, gesture_pinch_event.scale, trigger);
  if (new_angle > gesture_pinch_event.angle) { // Rotate right
    spdlog::get("main")->debug("[{}] at {} - {}: Rotate right", FN, __LINE__,
                               __func__);
    if (new_angle >= trigger) {
      std::string command = config->get_pinch_command(gesture_pinch_event.fingers, "CONTINUOUS", 4);
      spdlog::get("main")->debug(
         "[{}] at {} - {} - fingers: {}, type: CONTINUOUS, gesture: ROTATE RIGHT ... ",
         FN, __LINE__, __func__, gesture_pinch_event.fingers);
      if (runproc(command.c_str())) {
        inc_step(&gesture_pinch_event.step);
      } else {
        gesture_pinch_event.executed = true;
      }
    }
  } else { // Rotate left
    spdlog::get("main")->debug("[{}] at {} - {}: Rotate left", FN, __LINE__,
                               __func__);
    if (new_angle <= trigger) {
      std::string command = config->get_pinch_command(gesture_pinch_event.fingers, "CONTINUOUS", 3);
      spdlog::get("main")->debug(
        "[{}] at {} - {} - fingers: {}, type: CONTINUOUS, gesture: ROTATE LEFT ... ",
        FN, __LINE__, __func__, gesture_pinch_event.fingers);
      if (runproc(command.c_str())) {
        dec_step(&gesture_pinch_event.step);
      } else {
        gesture_pinch_event.executed = true;
      }
    }
  }
}

/**
 * Pinch Gesture
 * Supports "one shot" or "continuous" pinch-in, pinch-out, rotate-left, and
 * rotate-right gestures.
 * @param gev Gesture Event
 * @param begin Boolean to denote begin or continuation of gesture.
 **/
void gebaar::io::Input::handle_pinch_event(libinput_event_gesture* gev,
                                           bool begin) {
  if (begin) {
    reset_pinch_event();
    gesture_pinch_event.fingers = libinput_event_gesture_get_finger_count(gev);
  } else {
    if (!gesture_pinch_event.executed) {
      double new_scale = libinput_event_gesture_get_scale(gev);
      double angle_delta = libinput_event_gesture_get_angle_delta(gev);
      double new_angle = gesture_pinch_event.angle + angle_delta;
      if (!gesture_pinch_event.continuous) {
        handle_one_shot_pinch(new_scale);
        handle_one_shot_rotate(new_angle);
      } else {
        if (!gesture_pinch_event.rotating) {
          handle_continuous_pinch(new_scale);
        } else {
          handle_continuous_rotate(new_angle);
        }
      }
      gesture_pinch_event.scale = new_scale;
      gesture_pinch_event.angle = new_angle;
    }
  }
}

/**
 * This event has no coordinates, so it's an event that gives us a begin or end
 * signal. If it begins, we get the amount of fingers used. If it ends, we check
 * what kind of gesture we received.
 *
 * @param gev Gesture Event
 * @param begin Boolean to denote begin or end of gesture
 */
void gebaar::io::Input::handle_swipe_event_without_coords(
    libinput_event_gesture* gev, bool begin) {
  if (begin) {
    gesture_swipe_event.fingers = libinput_event_gesture_get_finger_count(gev);
  } else {
    // This executed when fingers left the touchpad
    if (!gesture_swipe_event.executed &&
        config->settings.gesture_swipe_trigger_on_release) {
      trigger_swipe_command();
    }
    reset_swipe_event();
  }
}

/**
 * Swipe events with coordinates, add it to the current tally
 * @param gev Gesture Event
 */
void gebaar::io::Input::handle_swipe_event_with_coords(
    libinput_event_gesture* gev) {
  if (gesture_swipe_event.executed) {
    return;
  }

  // Since swipe gesture counts in dpi we have to convert
  int threshold_x = config->settings.gesture_swipe_threshold *
                    SWIPE_X_THRESHOLD;
  int threshold_y = config->settings.gesture_swipe_threshold *
                    SWIPE_Y_THRESHOLD;
  gesture_swipe_event.x += libinput_event_gesture_get_dx_unaccelerated(gev);
  gesture_swipe_event.y += libinput_event_gesture_get_dy_unaccelerated(gev);
  if (abs(gesture_swipe_event.x) > threshold_x ||
      abs(gesture_swipe_event.y) > threshold_y) {
    trigger_swipe_command();
  }
}

/**
 * Making calculation for swipe direction and triggering
 * command accordingly
 */
void gebaar::io::Input::trigger_swipe_command() {
  double x = gesture_swipe_event.x;
  double y = gesture_swipe_event.y;
  int swipe_type = get_swipe_type(x, y);
  apply_swipe(swipe_type, gesture_swipe_event.fingers, swipe_event_group);
  gesture_swipe_event.x = 0;
  gesture_swipe_event.y = 0;
}

/**
 * Handles switch events.
 *
 * @param gev Switch Event
 * 0 == laptop
 * 1 == tablet
 */
void gebaar::io::Input::handle_switch_event(libinput_event_switch* gev)
{
  int state = libinput_event_switch_get_switch_state(gev);
  int state_2 = libinput_event_switch_get_switch(gev);
  spdlog::get("main")->debug("[{}] at {} - state: {}, state_2: {}", FN, __LINE__, state, state_2);
  if (state_2 == 2) {
    if (state == 0) {
      spdlog::get("main")->debug("[{}] at {} - Laptop Switch", FN, __LINE__);
      swipe_event_group = "GESTURE";
    } else {
      spdlog::get("main")->debug("[{}] at {} - Tablet Switch", FN, __LINE__);
      swipe_event_group = "TOUCH";
    }
    std::string command = config->get_switch_command(state);
    runproc(command.c_str());
  }
}



/**
 * Initialize the input system
 * @return bool
 */
bool gebaar::io::Input::initialize() {
  initialize_context();
  return gesture_device_exists();
}

/**
 * Run a poll loop on the file descriptor that libinput gives us
 */
void gebaar::io::Input::start_loop() {
  struct pollfd fds {};
  fds.fd = libinput_get_fd(libinput);
  fds.events = POLLIN;
  fds.revents = 0;

  while (poll(&fds, 1, -1) > -1) {
    handle_event();
  }
}

gebaar::io::Input::~Input() { libinput_unref(libinput); }

/**
 * Check if there's a device that supports gestures on this system
 * @return
 */
bool gebaar::io::Input::gesture_device_exists() {
  swipe_event_group = "";
  if (strcmp(config->settings.interact_type.c_str(), "BOTH") == 0 ) {
    spdlog::get("main")->debug("[{}] at {} - {}: Interact type set to BOTH", FN, __LINE__, __func__);
    swipe_event_group = "BOTH";
  } else if (strcmp(config->settings.interact_type.c_str(), "TOUCH") == 0 || strcmp(config->settings.interact_type.c_str(), "GESTURE") == 0){
    spdlog::get("main")->debug("[{}] at {} - {}: Interact type set to {}", FN, __LINE__, __func__, config->settings.interact_type.c_str());
    swipe_event_group = config->settings.interact_type;
  } else {
    while ((libinput_event = libinput_get_event(libinput)) != nullptr) {
      auto device = libinput_event_get_device(libinput_event);
      spdlog::get("main")->debug(
          "[{}] at {} - {}: Testing capabilities for device {}", FN, __LINE__,
          __func__, libinput_device_get_name(device));
      if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_GESTURE)) {
        swipe_event_group = "GESTURE";
      } else if (libinput_device_has_capability(device,
                                                LIBINPUT_DEVICE_CAP_TOUCH)) {
        swipe_event_group = "TOUCH";
      }

      libinput_event_destroy(libinput_event);
      libinput_dispatch(libinput);

      if (swipe_event_group == "GESTURE") {
        break;
      }
    }
  }

  if (swipe_event_group.empty()) {
    spdlog::get("main")->error(
        "[{}] at {} - {}: Gesture/Touch device not found", FN, __LINE__,
        __func__);
  } else {
    spdlog::get("main")->debug("[{}] at {} - {}: Gesture/Touch device found",
                               FN, __LINE__, __func__);
    spdlog::get("main")->debug("[{}] at {} - {}: Gebaar using '{}' events", FN,
                               __LINE__, __func__, swipe_event_group);
  }
  return !swipe_event_group.empty();
}

bool gebaar::io::Input::check_chosen_event(std::string ev) {
  if (strcmp(config->settings.interact_type.c_str(), "BOTH") == 0 ) {
    swipe_event_group = ev;
    return true;
  }
  if (swipe_event_group == ev) {
    return true;
  }
  return false;
}

/**
 * Handle an event from libinput and run the appropriate action per event type
 */
void gebaar::io::Input::handle_event() {
  libinput_dispatch(libinput);
  while ((libinput_event = libinput_get_event(libinput))) {
    switch (libinput_event_get_type(libinput_event)) {
      case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
        if (check_chosen_event("GESTURE")) {
          handle_swipe_event_without_coords(
              libinput_event_get_gesture_event(libinput_event), true);
        }
        break;
      case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:
        if (check_chosen_event("GESTURE")) {
          handle_swipe_event_with_coords(
              libinput_event_get_gesture_event(libinput_event));
        }
        break;
      case LIBINPUT_EVENT_GESTURE_SWIPE_END:
        if (check_chosen_event("GESTURE")) {
          handle_swipe_event_without_coords(
              libinput_event_get_gesture_event(libinput_event), false);
        }
        break;
      case LIBINPUT_EVENT_GESTURE_PINCH_BEGIN:
        handle_pinch_event(libinput_event_get_gesture_event(libinput_event),
                           true);
        break;
      case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE:
        handle_pinch_event(libinput_event_get_gesture_event(libinput_event),
                           false);
        break;
      case LIBINPUT_EVENT_GESTURE_PINCH_END:
        break;
      /*
      case LIBINPUT_EVENT_NONE:
        break;
      case LIBINPUT_EVENT_DEVICE_ADDED:
        break;
      case LIBINPUT_EVENT_DEVICE_REMOVED:
        break;
      case LIBINPUT_EVENT_KEYBOARD_KEY:
        break;
      case LIBINPUT_EVENT_POINTER_MOTION:
        break;
      case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        break;
      case LIBINPUT_EVENT_POINTER_BUTTON:
        break;
      case LIBINPUT_EVENT_POINTER_AXIS:
        break;
      */
      case LIBINPUT_EVENT_TOUCH_DOWN:
        if (check_chosen_event("TOUCH")) {
          handle_touch_event_down(
              libinput_event_get_touch_event(libinput_event));
        }
        break;
      case LIBINPUT_EVENT_TOUCH_UP:
        if (check_chosen_event("TOUCH")) {
          handle_touch_event_up(libinput_event_get_touch_event(libinput_event));
        }
        break;
      case LIBINPUT_EVENT_TOUCH_MOTION:
        if (check_chosen_event("TOUCH")) {
          handle_touch_event_motion(
              libinput_event_get_touch_event(libinput_event));
        }
        break;
      /*
      case LIBINPUT_EVENT_TOUCH_CANCEL:
        break;
      case LIBINPUT_EVENT_TOUCH_FRAME:
        break;
      case LIBINPUT_EVENT_TABLET_TOOL_AXIS:
        break;
      case LIBINPUT_EVENT_TABLET_TOOL_PROXIMITY:
        break;
      case LIBINPUT_EVENT_TABLET_TOOL_TIP:
        break;
      case LIBINPUT_EVENT_TABLET_TOOL_BUTTON:
        break;
      case LIBINPUT_EVENT_TABLET_PAD_BUTTON:
        break;
      case LIBINPUT_EVENT_TABLET_PAD_RING:
        break;
      case LIBINPUT_EVENT_TABLET_PAD_STRIP:
        break;
      case LIBINPUT_EVENT_TABLET_PAD_KEY:
        break;
      */
      case LIBINPUT_EVENT_SWITCH_TOGGLE:
        handle_switch_event(libinput_event_get_switch_event(libinput_event));
        break;
      default:
        break;
    }

    libinput_event_destroy(libinput_event);
    libinput_dispatch(libinput);
  }
}
