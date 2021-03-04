cmake_minimum_required(VERSION 3.10.2)
project(gebaar)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -pthread")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(LIBINPUT REQUIRED)
find_package(UDEV REQUIRED)
find_package(XINPUT REQUIRED)
find_package(X11 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCE_FILES RELATIVE ${PROJECT_SOURCE_DIR} src/*.h src/*.cpp)

add_executable(gebaard ${SOURCE_FILES})

target_link_libraries(gebaard
  ${LIBINPUT_LIBRARIES}
  ${UDEV_LIBRARIES}
  ${XINPUT_LIBRARIES}
  ${X11_LIBRARIES}
  stdc++fs
)
target_include_directories(gebaard PUBLIC
  ${LIBINPUT_INCLUDE_DIRS}
  ${UDEV_INCLUDE_DIRS}
  ${XINPUT_INCLUDE_DIRS}
  ${X11_INCLUDE_DIRS}
  libs/cxxopts/include
  libs/cpptoml/include
  libs/spdlog/include
)
target_compile_options(gebaard PUBLIC
  ${LIBINPUT_CFLAGS_OTHER}
  ${UDEV_CFLAGS_OTHER}
)

install(TARGETS gebaard DESTINATION bin)
