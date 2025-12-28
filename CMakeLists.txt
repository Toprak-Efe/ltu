cmake_minimum_required(VERSION 4.0.0)
project(ltu
    VERSION 0.0.1
    DESCRIPTION "A terminal application launcher"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
include(GNUInstallDirs)

find_package(ftxui REQUIRED)
find_package(Boost REQUIRED)

add_executable(${CMAKE_PROJECT_NAME}
    source/main.cpp
    source/menu.cpp
    source/desktop.cpp
    source/terminal.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${ftxui_INCLUDE_DIRS}
    include
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    ${Boost_LIBRARIES}
    INIReader
    ftxui::screen
    ftxui::component
)

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION bin)

