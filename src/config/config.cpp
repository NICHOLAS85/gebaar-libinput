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

#include "config.h"
#include <zconf.h>
#include "../util.h"
#define FN "config"

/**
 * Check if config file exists at current path
 */
bool gebaar::config::Config::config_file_exists() {
  auto true_path = std::filesystem::path(config_file_path);
  return std::filesystem::exists(true_path);
}

/**
 * Load Configuration from TOML file
 */
void gebaar::config::Config::load_config() {
  if (find_config_file()) {
    if (config_file_exists()) {
      try {
        config = cpptoml::parse_file(std::filesystem::path(config_file_path));
        spdlog::get("main")->debug("[{}] at {} - Config parsed", FN, __LINE__);
      } catch (const cpptoml::parse_exception& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
      }
      auto command_swipe_table =
          config->get_table_array_qualified("swipe.commands");
      if (command_swipe_table) {
        spdlog::get("main")->debug("[{}] at {} - Generating SWIPE_COMMANDS", FN,
                                  __LINE__);
        for (const auto& table : *command_swipe_table) {
          auto fingers = table->get_as<size_t>("fingers");
          fingers = fingers.value_or(3);
          auto type = table->get_as<std::string>("type");
          type = type.value_or("GESTURE");
          for (std::pair<size_t, std::string> element : SWIPE_COMMANDS) {
            commands[*fingers][*type][element.second] =
                table->get_qualified_as<std::string>(element.second).value_or("");
          }
        }
      } else {
        spdlog::get("main")->debug("[{}] at {} - No SWIPE_COMMANDS found", FN,
                                  __LINE__);
      }

      auto pinch_command_table =
          config->get_table_array_qualified("pinch.commands");
      if (pinch_command_table) {
        spdlog::get("main")->debug("[{}] at {} - Generating PINCH_COMMANDS", FN,
                                  __LINE__);
        for (const auto& table : *pinch_command_table) {
            auto fingers = table->get_as<size_t>("fingers");
            fingers = fingers.value_or(2);
            auto type = table->get_as<std::string>("type");
            type = type.value_or("ONESHOT");
            for (std::pair<int, std::string> element : PINCH_COMMANDS) {
              pinch_commands[*fingers][*type][element.second] =
                  table->get_qualified_as<std::string>(element.second).value_or("");
            }
        }
      } else {
        spdlog::get("main")->debug("[{}] at {} - No PINCH_COMMANDS found", FN,
                                  __LINE__);
      }

      switch_commands_laptop =
          *config->get_qualified_as<std::string>("switch.commands.laptop");
      switch_commands_tablet =
          *config->get_qualified_as<std::string>("switch.commands.tablet");

      settings.gesture_swipe_threshold =
          config->get_qualified_as<double>("settings.gesture_swipe.threshold")
              .value_or(0.5);
      settings.gesture_swipe_one_shot =
          config->get_qualified_as<bool>("settings.gesture_swipe.one_shot")
              .value_or(true);
      settings.gesture_swipe_trigger_on_release =
          config
              ->get_qualified_as<bool>(
                  "settings.gesture_swipe.trigger_on_release")
              .value_or(true);
      settings.touch_longswipe_screen_percentage =
          config
              ->get_qualified_as<double>(
                  "settings.touch_swipe.longswipe_screen_percentage")
              .value_or(LONGSWIPE_SCREEN_PERCENT_DEFAULT);

      settings.pinch_threshold =
          config->get_qualified_as<double>("settings.pinch.threshold")
              .value_or(0.25);

      settings.rotate_threshold =
          config->get_qualified_as<double>("settings.rotate.threshold")
              .value_or(20);

      settings.interact_type =
          *config->get_qualified_as<std::string>("settings.interact.type");

      loaded = true;
      spdlog::get("main")->debug("[{}] at {} - Config loaded", FN, __LINE__);
    }
  }
}

/**
 * Find the configuration file according to XDG spec
 * @return bool
 */
bool gebaar::config::Config::find_config_file() {
  std::string temp_path =
      gebaar::util::stringFromCharArray(getenv("XDG_CONFIG_HOME"));
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
    spdlog::get("main")->debug("[{}] at {} - config path generated: '{}'", FN,
                               __LINE__, config_file_path);
    return true;
  }
  spdlog::get("main")->debug("[{}] at {} - config path not generated: '{}'", FN,
                             __LINE__, config_file_path);
  return false;
}

gebaar::config::Config::Config() {
  if (!loaded) {
    load_config();
  }
}

/**
 * Given a number of fingers and a swipe type return configured command
 */

std::string gebaar::config::Config::get_swipe_type_name(size_t key) {
  return SWIPE_COMMANDS.at(key);
}

std::string gebaar::config::Config::get_command(size_t fingers,
                                                std::string type,
                                                size_t swipe_type) {
   if (type == "ONESHOT" || type == "CONTINUOUS") {
      return pinch_commands[fingers][type][PINCH_COMMANDS.at(swipe_type)];
   } else if (fingers > 0 && swipe_type >= MIN_DIRECTION &&
      swipe_type <= MAX_DIRECTION) {
    if (commands.count(fingers)) {
      return commands[fingers][type][SWIPE_COMMANDS.at(swipe_type)];
    }
  }
  return "";
}
