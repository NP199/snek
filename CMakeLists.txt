cmake_minimum_required(VERSION 3.25)

project(ftxui_snake
    VERSION 0.1.0
    DESCRIPTION "Snek in FTXUI"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v7.0.3
    GIT_SHALLOW TRUE
)

# We only use FTXUI as a dependency, not its examples/tests/docs.
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ftxui)

function(add_snake_step target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE ftxui::ftxui)
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/inc)
    target_compile_features(${target} PRIVATE cxx_std_26)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_snake_step(snake             src/main.cpp)
