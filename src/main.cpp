/*
    gebaar
    Copyright (C) 2019  coffee2code

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

// ---------------------------------Version------------------------------------
#define GB_VERSION_MAJOR 1    // For breaking interface changes
#define GB_VERSION_MINOR 0    // For new (non-breaking) interface capabilities
#define GB_VERSION_RELEASE 0  // For tweaks, bug fixes or development

#include <libinput.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>
#include "config/config.h"
#include "daemonizer.h"
#include "debug/debug.h"
#include "io/input.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_sinks.h"

gebaar::io::Input* input;


std::string get_proc_name() {
    std::ifstream comm("/proc/self/comm");
    std::string name;
    getline(comm, name);
    return name;
}

int main(int argc, char* argv[]) {
    auto logger = spdlog::stdout_logger_mt("main");

    cxxopts::Options options(argv[0], "Gebaard Gestures Daemon");

    bool should_daemonize = false;

    options.add_options()("b,background", "Daemonize",
                           cxxopts::value(should_daemonize))(
            "h,help", "Prints this help text")(
            "v,verbose", "Prints verbose output during runtime");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (result.count("verbose")) {
        std::cout << "verbose mode" << std::endl;
        spdlog::set_level(spdlog::level::debug);
    }

    if (should_daemonize) {
        gebaar::daemonizer::Daemonizer().daemonize();
    }

    std::shared_ptr<gebaar::config::Config> config =
            std::make_shared<gebaar::config::Config>();
    input = new gebaar::io::Input(config);    //, debug);
    if (input->initialize()) {
        spdlog::get("main")->info("Running {} v{}",
                    get_proc_name(),
                    std::to_string(GB_VERSION_MAJOR) + "." +
                    std::to_string(GB_VERSION_MINOR) + "." +
                    std::to_string(GB_VERSION_RELEASE));
        input->start_loop();
    }

    return 0;
}
