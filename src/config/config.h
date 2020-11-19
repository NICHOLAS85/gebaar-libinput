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

#ifndef SRC_GEBAAR_CONFIG_H_
#define SRC_GEBAAR_CONFIG_H_

#include <cpptoml.h>
#include <pwd.h>
#include <spdlog/spdlog.h>
#include "utils/filesystem.h"
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <iterator>

#define MAX_DIRECTION 9
#define MIN_DIRECTION 1
#define LONGSWIPE_SCREEN_PERCENT_DEFAULT 70

const std::map<size_t, std::string> SWIPE_COMMANDS = {
    {1, "left_up"},        {2, "up"},
    {3, "right_up"},       {4, "left"},
    {6, "right"},          {7, "left_down"},
    {8, "down"},           {9, "right_down"}
};
const std::map<size_t, std::string> PINCH_COMMANDS = {
    {1, "in"},             {2, "out"},
    {3, "rotate_left"},    {4, "rotate_right"}
};
const std::map<size_t, std::string> SWITCH_COMMANDS = {
    {0, "laptop"},         {1, "tablet"}
};

namespace gebaar::config {
class Config {
   public:
    Config();

    bool loaded = false;

    void load_config();

    struct settings {
        bool pinch_one_shot;
        double pinch_threshold;
        double rotate_threshold;

        double gesture_swipe_threshold;
        bool gesture_swipe_trigger_on_release;

        double touch_longswipe_screen_percentage;
        std::string interact_type;
    } settings;

    std::string get_swipe_command(size_t fingers, std::string type, size_t swipe_type, bool one_shot);
    std::string get_pinch_command(size_t fingers, std::string type, size_t swipe_type);
    std::string get_switch_command(size_t key);
    std::string get_swipe_type_name(size_t key);


   private:
    bool config_file_exists();

    bool find_config_file();

    std::string config_file_path;
    std::shared_ptr<cpptoml::table> config;
    std::map<size_t, std::map<std::string, std::map<bool, std::map<std::string, std::string>>>> swipe_commands;
    std::map<size_t, std::map<std::string, std::map<std::string, std::string>>> pinch_commands;
    std::map<std::string, std::string> switch_commands;
};
}  // namespace gebaar::config
#endif  // SRC_CONFIG_CONFIG_H_
