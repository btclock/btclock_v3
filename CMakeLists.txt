cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
list(APPEND EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/.pio/libdeps/${configName}/esp_littlefs")
project(btclock_espidf)
