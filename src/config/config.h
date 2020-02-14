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

#ifndef GEBAAR_CONFIG_H
#define GEBAAR_CONFIG_H

#include <cpptoml.h>
#include <filesystem>
#include <pwd.h>
#include <iostream>
#include <spdlog/spdlog.h>

#define MAX_DIRECTION 9
#define MIN_DIRECTION 1
const std::map<int, std::string> SWIPE_COMMANDS = {
    {1, "left_up"}, {2, "up"},        {3, "right_up"}, {4, "left"},
    {6, "right"},   {7, "left_down"}, {8, "down"},     {9, "right_down"}};
const std::map<int, std::string> PINCH_COMMANDS = {
    {1, "in"}, {2, "out"}};

namespace gebaar::config {
    class Config {
    public:
        Config();

        bool loaded = false;

        void load_config();

        std::string get_command(int fingers, int swipe_type, std::string method);
        std::string key_command;
        std::string laptop_mode_command;
        std::string tablet_mode_command;
        std::string interact_type;
        double pinch_threshold;
        double swipe_threshold;

    private:
        std::map<int, std::map<std::string, std::string>> gesture_commands;
        std::map<int, std::map<std::string, std::string>> touch_commands;
        std::map<int, std::map<std::string, std::string>> pinch_commands;
        bool config_file_exists();

        bool find_config_file();

        std::string config_file_path;
        std::shared_ptr<cpptoml::table> config;
    };
}
#endif //GEBAAR_CONFIG_H
