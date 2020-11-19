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
#include "utils/string-from-char.h"
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
      spdlog::get("main")->debug("[{}] at {} - Generating SWIPE_COMMANDS", FN,
                                 __LINE__);
      auto swipe_command_table =
          config->get_table_array_qualified("swipe.commands");
      if (swipe_command_table == nullptr) {
        spdlog::get("main")->debug("[{}] at {} - swipe_command_table empty", FN, __LINE__);
      } else {
        for (const auto& table : *swipe_command_table) {
          auto fingers = table->get_as<size_t>("fingers");
          fingers = fingers.value_or(3);
          auto type = table->get_as<std::string>("type");
          type = type.value_or("GESTURE");
          auto one_shot = table->get_as<bool>("one_shot");
          one_shot = one_shot.value_or(true);
          for (std::pair<size_t, std::string> element : SWIPE_COMMANDS) {
            swipe_commands[*fingers][*type][*one_shot][element.second] =
                table->get_qualified_as<std::string>(element.second).value_or("");
          }
        }
      }

      spdlog::get("main")->debug("[{}] at {} - Generating PINCH_COMMANDS", FN,
                                 __LINE__);
      auto pinch_command_table =
          config->get_table_array_qualified("pinch.commands");
      if (pinch_command_table == nullptr) {
        spdlog::get("main")->debug("[{}] at {} - pinch_command_table empty", FN, __LINE__);
      } else {
        for (const auto& table : *pinch_command_table) {
          auto fingers = table->get_as<size_t>("fingers");
          fingers = fingers.value_or(2);
          auto type = table->get_as<std::string>("type");
          type = type.value_or("ONESHOT");
          for (std::pair<size_t, std::string> element : PINCH_COMMANDS) {
            pinch_commands[*fingers][*type][element.second] =
                table->get_qualified_as<std::string>(element.second).value_or("");
          }
        }
      }

      spdlog::get("main")->debug("[{}] at {} - Generating SWITCH_COMMANDS", FN,
                                 __LINE__);
      auto switch_command_table =
          config->get_table_array_qualified("switch.commands");
      if (switch_command_table == nullptr) {
        spdlog::get("main")->debug("[{}] at {} - switch_command_table empty", FN, __LINE__);
      } else {
        for (const auto& table : *switch_command_table) {
          for (std::pair<size_t, std::string> element : SWITCH_COMMANDS) {
            switch_commands[element.second] =
                table->get_qualified_as<std::string>(element.second).value_or("");
          }
        }
      }

      settings.gesture_swipe_threshold =
          config->get_qualified_as<double>("settings.gesture_swipe.threshold")
              .value_or(0.5);
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

std::string gebaar::config::Config::get_swipe_command(size_t fingers,
                                                std::string type,
                                                size_t swipe_type,
                                                bool one_shot) {
    if (fingers > 0 && swipe_type >= MIN_DIRECTION &&
      swipe_type <= MAX_DIRECTION) {
    if (swipe_commands.count(fingers)) {
      return swipe_commands[fingers][type][one_shot][SWIPE_COMMANDS.at(swipe_type)];
    }
  }
  return "";
}

std::string gebaar::config::Config::get_pinch_command(size_t fingers,
                                                std::string type,
                                                size_t swipe_type) {
  if (fingers > 0 && pinch_commands.count(fingers)) {
    return pinch_commands[fingers][type][PINCH_COMMANDS.at(swipe_type)];
  }
  return "";
}

std::string gebaar::config::Config::get_switch_command(size_t key) {
  return switch_commands[SWITCH_COMMANDS.at(key)];
}
