cmake_minimum_required(VERSION 3.10)

project(aseprite-wal)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(USER_CONFIG_DIR $ENV{HOME}/.config/aseprite-wal)

find_package(PNG REQUIRED)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(aseprite-wal
    src/main.cc
    src/image.cc
    src/theme.cc
    src/pywal.cc
)

target_link_libraries(aseprite-wal PRIVATE PNG::PNG nlohmann_json::nlohmann_json)

target_include_directories(aseprite-wal PRIVATE ${PNG_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/include)

install(TARGETS aseprite-wal RUNTIME DESTINATION bin)

install(CODE "
    if(NOT EXISTS ${USER_CONFIG_DIR}/templates/light)
        execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${USER_CONFIG_DIR}/templates/)
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/templates/light ${USER_CONFIG_DIR}/templates/light)
        message(STATUS \"Template moved to ~/.config/aseprite-wal/templates/light\")
    else()
        message(STATUS \"Template already exists in config location, skipping\")
    endif()
    message(STATUS ${USER_CONFIG_DIR}/templates/light)
    ")
