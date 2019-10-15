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
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <poll.h>
#define FN "input"
#define THRESH 100

/**
 * Input system constructor, we pass our Configuration object via a shared pointer
 *
 * @param config_ptr shared pointer to configuration object
 */
gebaar::io::Input::Input(std::shared_ptr<gebaar::config::Config> const& config_ptr)
{
    config = config_ptr;
    gesture_swipe_event = {};
    touch_swipe_event = {};
}

/**
 * Initialize the libinput context
 *
 * @return bool
 */
bool gebaar::io::Input::initialize_context()
{
    udev = udev_new();
    libinput = libinput_udev_create_context(&libinput_interface, nullptr, udev);
    return libinput_udev_assign_seat(libinput, "seat0") == 0;
}

int gebaar::io::Input::get_swipe_type(double sdx, double sdy)
{
    double x = sdx;
    double y = sdy;
    int swipe_type = 5; // middle = no swipe
        // 1 = left_up, 2 = up, 3 = right_up...
        // 1 2 3
        // 4 5 6
        // 7 8 9
    const double OBLIQUE_RATIO = 0.414; // =~ tan(22.5);

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

void gebaar::io::Input::apply_swipe(int swipe_type, int fingers)
{
    std::string command = config->get_command(fingers, swipe_type);
    spdlog::get("main")->debug("[{}] at {} - {}, fgrs: {}, swpe-typ: {}", FN, __LINE__, __func__, fingers, swipe_type);
    if (command.length() > 0) {
        std::system(command.c_str());
    }
}

/**
 * This function is used to decide how many fingers are used for the touch swipe gesture
 * Each swipe must ensure
 * 1) The touch down of all fingers occur in x amount of time from the previous finger
 * 2) The lift up of all fingers occur in x amount of time from the previous finger
 *
 * Any fingers touched down or lifted outside of x makes the event 'unclean' and therefore, swiping fails
 *
 * @param slots Represents pair(finger, timeoftouch(down/lift))
 */
void gebaar::io::Input::check_multitouch_down_up(std::vector<std::pair<int, double>> slots, std::string downup)
{
    if (touch_swipe_event.isClean && slots.size() > 1) {
        std::vector<std::pair<int, double>>::reverse_iterator iter = slots.rbegin();
        std::vector<std::pair<int, double>>::reverse_iterator prev_iter = std::next(iter, 1);
        double timebetweenslots = iter->second - prev_iter->second;
        if (timebetweenslots <= THRESH) {
            touch_swipe_event.fingers = slots.size();
        } else {
            touch_swipe_event.isClean = false;
            spdlog::get("main")->debug("[{}] at {} - {} - {}: finger added/lifted too late/early. {} ", FN, __LINE__, __func__, downup);
        }
    }
}

/**
 * This event occurs when a finger touches the touchscreen
 * It passes a list of pairs (slot_id, timestamp) to check_multitouch_down_up
 * Each slot corresponds to a finger touched down on the screen
 *
 * @param tev Touch Event
 */
void gebaar::io::Input::handle_touch_event_down(libinput_event_touch* tev)
{
    touch_swipe_event.down_slots.push_back(std::pair<int, double>(libinput_event_touch_get_slot(tev), libinput_event_touch_get_time(tev)));
    check_multitouch_down_up(touch_swipe_event.down_slots, "down_slots");
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
void gebaar::io::Input::handle_touch_event_up(libinput_event_touch* tev)
{
    touch_swipe_event.up_slots.push_back(std::pair<int, double>(libinput_event_touch_get_slot(tev), libinput_event_touch_get_time(tev)));
    check_multitouch_down_up(touch_swipe_event.up_slots, "up_slots");

    if (touch_swipe_event.up_slots.size() == touch_swipe_event.down_slots.size()) {
        std::map<int, std::pair<double, double>>::iterator iter = touch_swipe_event.delta_xy.begin();
        std::vector<int> swipes;
        int swipe_type;
        while (iter != touch_swipe_event.delta_xy.end()) {
            double x = iter->second.first;
            double y = iter->second.second;
            swipe_type = get_swipe_type(x, y);

            if (!swipes.empty() && swipe_type != swipes.back()) {
                touch_swipe_event.isClean = false;
            }
            spdlog::get("main")->debug("[{}] at {} - {}, swpe-type: {}", FN, __LINE__, __func__, swipe_type);
            swipes.push_back(swipe_type);
            iter++;
        }

        if (touch_swipe_event.down_slots.size() != touch_swipe_event.delta_xy.size()) {
            touch_swipe_event.isClean = false;
        }

        if (touch_swipe_event.isClean) {
            apply_swipe(swipe_type, touch_swipe_event.fingers);
        }

        spdlog::get("main")->debug("[{}] at {} - {}, fgrs: {}, d-slts: {}, u-slts: {}, d-xy: {}, prv-xy: {}", FN, __LINE__, __func__, touch_swipe_event.fingers, touch_swipe_event.down_slots.size(), touch_swipe_event.up_slots.size(), touch_swipe_event.delta_xy.size(), touch_swipe_event.prev_xy.size());
        touch_swipe_event = {};
        spdlog::get("main")->debug("[{}] at {} - {}, fgrs: {}, d-slts: {}, u-slts: {}, d-xy: {}, prv-xy: {}", FN, __LINE__, __func__, touch_swipe_event.fingers, touch_swipe_event.down_slots.size(), touch_swipe_event.up_slots.size(), touch_swipe_event.delta_xy.size(), touch_swipe_event.prev_xy.size());
        spdlog::get("main")->debug("[{}] at {} - {}: touch gesture finished, {}\n\n", FN, __LINE__, __func__, touch_swipe_event.isClean);
    }
}

/**
 * This event occurs when a finger moves on the touchscreen
 * mimics handle_swipe_event_with_coords but for multiple tracks (each touched down finger)
 *
 * libinput touch event has no get_dx, get_dy functions. Store previous coordinates to acquire dx and dy
 *
 * @param tev Touch Event
 */
void gebaar::io::Input::handle_touch_event_motion(libinput_event_touch* tev)
{
    if (touch_swipe_event.delta_xy.find(libinput_event_touch_get_slot(tev)) == touch_swipe_event.delta_xy.end()) {
        touch_swipe_event.delta_xy.insert(std::pair<int, std::pair<double, double>>(libinput_event_touch_get_slot(tev), { 0, 0 }));
        touch_swipe_event.prev_xy.insert(std::pair<int, std::pair<double, double>>(libinput_event_touch_get_slot(tev), { libinput_event_touch_get_x(tev), libinput_event_touch_get_y(tev) }));
    } else {
        std::pair<double, double> prevcoord = touch_swipe_event.prev_xy.find(libinput_event_touch_get_slot(tev))->second;
        double prevx = prevcoord.first;
        double prevy = prevcoord.second;
        touch_swipe_event.delta_xy.find(libinput_event_touch_get_slot(tev))->second = { libinput_event_touch_get_x(tev) - prevx, libinput_event_touch_get_y(tev) - prevy };
        touch_swipe_event.prev_xy.find(libinput_event_touch_get_slot(tev))->second = { libinput_event_touch_get_x(tev), libinput_event_touch_get_y(tev) };
    }
}

/**
 * This event has no coordinates, so it's an event that gives us a begin or end signal.
 * If it begins, we get the amount of fingers used.
 * If it ends, we check what kind of gesture we received.
 *
 * @param gev Gesture Event
 * @param begin Boolean to denote begin or end of gesture
 */
void gebaar::io::Input::handle_swipe_event_without_coords(libinput_event_gesture* gev, bool begin)
{
    if (begin) {
        gesture_swipe_event.fingers = libinput_event_gesture_get_finger_count(gev);
    } else {
        double x = gesture_swipe_event.x;
        double y = gesture_swipe_event.y;
        int swipe_type = get_swipe_type(x, y);
        apply_swipe(swipe_type, gesture_swipe_event.fingers);

        gesture_swipe_event = {};
    }
}

/**
 * Swipe events with coordinates, add it to the current tally
 * @param gev Gesture Event
 */
void gebaar::io::Input::handle_swipe_event_with_coords(libinput_event_gesture* gev)
{
    gesture_swipe_event.x += libinput_event_gesture_get_dx(gev);
    gesture_swipe_event.y += libinput_event_gesture_get_dy(gev);
}

/**
 * Initialize the input system
 * @return bool
 */
bool gebaar::io::Input::initialize()
{
    initialize_context();
    return gesture_device_exists();
}

/**
 * Run a poll loop on the file descriptor that libinput gives us
 */
void gebaar::io::Input::start_loop()
{
    struct pollfd fds {
    };
    fds.fd = libinput_get_fd(libinput);
    fds.events = POLLIN;
    fds.revents = 0;

    while (poll(&fds, 1, -1) > -1) {
        handle_event();
    }
}

gebaar::io::Input::~Input()
{
    libinput_unref(libinput);
}

/**
 * Check if there's a device that supports gestures on this system
 * @return
 */
bool gebaar::io::Input::gesture_device_exists()
{
    swipe_event_group = "";
    while ((libinput_event = libinput_get_event(libinput)) != nullptr) {
        auto device = libinput_event_get_device(libinput_event);
        if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_GESTURE)) {
            swipe_event_group = "GESTURE";
        } else if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH)) {
            swipe_event_group = "TOUCH";
        }

        libinput_event_destroy(libinput_event);
        libinput_dispatch(libinput);
    }
    if (swipe_event_group.empty()) {
        spdlog::get("main")->error("[{}] at {} - {}: Gesture/Touch device not found", FN, __LINE__, __func__);
    } else {
        spdlog::get("main")->debug("[{}] at {} - {}: Gesture/Touch device found", FN, __LINE__, __func__);
        spdlog::get("main")->debug("[{}] at {} - {}: Gebaar using '{}' events", FN, __LINE__, __func__, swipe_event_group);
    }
    return !swipe_event_group.empty();
}

bool gebaar::io::Input::check_chosen_event(std::string ev)
{
    if (swipe_event_group == ev) {
        return true;
    }
    return false;
}

/**
 * Handle an event from libinput and run the appropriate action per event type
 */
void gebaar::io::Input::handle_event()
{
    libinput_dispatch(libinput);
    while ((libinput_event = libinput_get_event(libinput))) {
        switch (libinput_event_get_type(libinput_event)) {
        case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
            if (check_chosen_event("GESTURE")) {
                handle_swipe_event_without_coords(libinput_event_get_gesture_event(libinput_event), true);
            }
            break;
        case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:
            if (check_chosen_event("GESTURE")) {
                handle_swipe_event_with_coords(libinput_event_get_gesture_event(libinput_event));
            }
            break;
        case LIBINPUT_EVENT_GESTURE_SWIPE_END:
            if (check_chosen_event("GESTURE")) {
                handle_swipe_event_without_coords(libinput_event_get_gesture_event(libinput_event), false);
            }
            break;
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
        case LIBINPUT_EVENT_TOUCH_DOWN:
            if (check_chosen_event("TOUCH")) {
                handle_touch_event_down(libinput_event_get_touch_event(libinput_event));
            }
            break;
        case LIBINPUT_EVENT_TOUCH_UP:
            if (check_chosen_event("TOUCH")) {
                handle_touch_event_up(libinput_event_get_touch_event(libinput_event));
            }
            break;
        case LIBINPUT_EVENT_TOUCH_MOTION:
            if (check_chosen_event("TOUCH")) {
                handle_touch_event_motion(libinput_event_get_touch_event(libinput_event));
            }
            break;
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
        case LIBINPUT_EVENT_GESTURE_PINCH_BEGIN:
            break;
        case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE:
            break;
        case LIBINPUT_EVENT_GESTURE_PINCH_END:
            break;
        case LIBINPUT_EVENT_SWITCH_TOGGLE:
            break;
        }

        libinput_event_destroy(libinput_event);
        libinput_dispatch(libinput);
    }
}
