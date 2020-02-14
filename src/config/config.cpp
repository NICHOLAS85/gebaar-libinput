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


#include <zconf.h>
#include "config.h"
#include "../util.h"
#include <iterator>
#define FN "config"

/**
 * Check if config file exists at current path
 */
bool gebaar::config::Config::config_file_exists()
{
    auto true_path = std::filesystem::path(config_file_path);
    if ( !std::filesystem::exists(true_path) ) {
        spdlog::get("main")->error("[{}] at {} - config file not found: '{}'", FN, __LINE__, config_file_path);
        exit(EXIT_FAILURE);
    }
    else {
        spdlog::get("main")->debug("[{}] at {} - config file found: '{}'", FN, __LINE__, config_file_path);
    }
    return std::filesystem::exists(true_path);
}

/**
 * Load Configuration from TOML file
 */
void gebaar::config::Config::load_config()
{
    if (find_config_file()) {
        if (config_file_exists()) {
            config = cpptoml::parse_file(std::filesystem::path(config_file_path));
            spdlog::get("main")->debug("[{}] at {} - Config parsed", FN, __LINE__);
            spdlog::get("main")->debug("[{}] at {} - Generating gesture SWIPE_COMMANDS", FN, __LINE__);
            auto gesture_command_swipe_table = config->get_table_array_qualified("command-swipe-gesture");
            for (const auto& table : *gesture_command_swipe_table)
            {
                auto fingers = table->get_as<int>("fingers");
                for (std::pair<int, std::string> element : SWIPE_COMMANDS) {
                    gesture_commands[*fingers][element.second] = table->get_qualified_as<std::string>(element.second).value_or("");
                }
            }

            spdlog::get("main")->debug("[{}] at {} - Generating touch SWIPE_COMMANDS", FN, __LINE__);
            auto touch_command_swipe_table = config->get_table_array_qualified("command-swipe-touch");
            for (const auto& table : *touch_command_swipe_table)
            {
                auto fingers = table->get_as<int>("fingers");
                for (std::pair<int, std::string> element : SWIPE_COMMANDS) {
                    touch_commands[*fingers][element.second] = table->get_qualified_as<std::string>(element.second).value_or("");
                }
            }

            spdlog::get("main")->debug("[{}] at {} - Generating PINCH_COMMANDS", FN, __LINE__);
            auto pinch_command_table = config->get_table_array_qualified("command-pinch");
            for (const auto& table : *pinch_command_table)
            {
                auto fingers = table->get_as<int>("fingers");
                for (std::pair<int, std::string> element : PINCH_COMMANDS) {
                    pinch_commands[*fingers][element.second] = table->get_qualified_as<std::string>(element.second).value_or("");
                }
            }

            key_command = *config->get_qualified_as<std::string>("command-key.190");

            laptop_mode_command = *config->get_qualified_as<std::string>("command-switch.laptop");
            tablet_mode_command = *config->get_qualified_as<std::string>("command-switch.tablet");

            interact_type = *config->get_qualified_as<std::string>("settings.interact.type");

            pinch_threshold = config->get_qualified_as<double>("settings.pinch.threshold").value_or(0.25);
            swipe_threshold = config->get_qualified_as<double>("settings.gesture.threshold").value_or(0.5);

            loaded = true;
            spdlog::get("main")->debug("[{}] at {} - Config loaded", FN, __LINE__);
        }
    }
}

/**
 * Find the configuration file according to XDG spec
 * @return bool
 */
bool gebaar::config::Config::find_config_file()
{
    std::string temp_path = gebaar::util::stringFromCharArray(getenv("XDG_CONFIG_HOME"));
    if (temp_path.empty()) {
        // first get the path to HOME
        temp_path = gebaar::util::stringFromCharArray(getenv("HOME"));
        if (temp_path.empty()) {
            temp_path = getpwuid(getuid())->pw_dir;
        }
        // then append .config
        if (!temp_path.empty()) {
            temp_path.append("/.config");
        }
    }
    if (!temp_path.empty()) {
        config_file_path = temp_path;
        config_file_path.append("/gebaar/gebaard.toml");
        spdlog::get("main")->debug("[{}] at {} - config path generated: '{}'", FN, __LINE__, config_file_path);
        return true;
    }
    spdlog::get("main")->debug("[{}] at {} - config path not generated: '{}'", FN, __LINE__, config_file_path);
    return false;
}

gebaar::config::Config::Config()
{
    if (!loaded) {
        load_config();
    }
}

/**
 * Given a number of fingers and a swipe type return configured command
 */

std::string gebaar::config::Config::get_command(int fingers, int swipe_type, std::string method){
    if (fingers > 1 && swipe_type >= MIN_DIRECTION && swipe_type <= MAX_DIRECTION){
        if (strcmp(method.c_str(), "GESTURE") == 0) {
            if (gesture_commands.count(fingers)) {
                spdlog::get("main")->info("[{}] at {} - gesture: {} finger {} ... executing", FN, __LINE__, fingers, SWIPE_COMMANDS.at(swipe_type));
                return gesture_commands[fingers][SWIPE_COMMANDS.at(swipe_type)];
            }
        } else if (strcmp(method.c_str(), "TOUCH") == 0) {
            if (touch_commands.count(fingers)) {
                spdlog::get("main")->info("[{}] at {} - gesture: {} finger {} ... executing", FN, __LINE__, fingers, SWIPE_COMMANDS.at(swipe_type));
                return touch_commands[fingers][SWIPE_COMMANDS.at(swipe_type)];
            }
        } else {
            if (pinch_commands.count(fingers)) {
              spdlog::get("main")->info("[{}] at {} - gesture: {} finger {} ... executing", FN, __LINE__, fingers, PINCH_COMMANDS.at(swipe_type));
              return pinch_commands[fingers][PINCH_COMMANDS.at(swipe_type)];
            }
        }
    }
    return "";
}
