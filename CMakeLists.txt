project(ConwaySDL)
cmake_minimum_required(VERSION 3.20)

if (WIN32)
    add_executable(game
        WIN32
        main.cpp
        cellmap.cpp
        cellmap.h)
    target_include_directories(game
        PRIVATE
        "${CMAKE_SOURCE_DIR}/include/win")
    target_link_libraries(game
        PRIVATE
        "${CMAKE_SOURCE_DIR}/lib/win/SDL2.lib")
    target_compile_definitions(game
        PRIVATE
        Windows
        CIN)
    target_link_options(game
        PRIVATE
        -SUBSYSTEM:CONSOLE
        )
else()
    add_executable(game
        main.cpp
        cellmap.cpp
        cellmap.h)
    target_include_directories(game
        PRIVATE
        "${CMAKE_SOURCE_DIR}/include")
    target_link_libraries(game
        PRIVATE
        "${CMAKE_SOURCE_DIR}/lib/libSDL2.a")
    target_compile_definitions(game
        PRIVATE
        CIN)
endif()

