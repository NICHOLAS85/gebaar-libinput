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

#ifndef GEBAAR_INPUT_HPP
#define GEBAAR_INPUT_HPP

#include "../config/config.h"
#include <fcntl.h>
#include <iterator>
#include <libinput.h>
#include <list>
#include <map>
#include <vector>
#include <zconf.h>

namespace gebaar::io {
struct gesture_swipe_event {
    int fingers;
    double x;
    double y;
};

struct touch_swipe_event {
    int fingers;
    double x;
    double y;
    std::map<int, std::pair<double, double>> prev_xy;
    std::map<int, std::pair<double, double>> delta_xy;
    std::vector<std::pair<int, double>> down_slots;
    std::vector<std::pair<int, double>> up_slots;
    bool isClean = true;
};

class Input {
public:
    Input(std::shared_ptr<gebaar::config::Config> const& config_ptr); //, std::shared_ptr<gebaar::debug::Debug> const& debug_ptr);

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
    struct touch_swipe_event touch_swipe_event;

    bool initialize_context();

    bool gesture_device_exists();

    bool check_chosen_event(std::string ev);

    static int open_restricted(const char* path, int flags, void* user_data)
    {
        int fd = open(path, flags);
        return fd < 0 ? -errno : fd;
    }

    static void close_restricted(int fd, void* user_data)
    {
        close(fd);
    }

    constexpr static struct libinput_interface libinput_interface = {
        .open_restricted = open_restricted,
        .close_restricted = close_restricted,
    };

    void check_multitouch_down_up(std::vector<std::pair<int, double>> slots, std::string downup);

    void apply_swipe(int swipe_type, int fingers);

    int get_swipe_type(double sdx, double sdy);

    void handle_event();

    void handle_touch_event_down(libinput_event_touch* tev);

    void handle_touch_event_up(libinput_event_touch* tev);

    void handle_touch_event_motion(libinput_event_touch* tev);

    void handle_swipe_event_without_coords(libinput_event_gesture* gev, bool begin);

    void handle_swipe_event_with_coords(libinput_event_gesture* gev);
};
}

#endif //GEBAAR_INPUT_HPP
