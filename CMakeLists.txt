cmake_minimum_required(VERSION 3.20)
project(LogCleaner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(LogCleaner WIN32
    src/main.cpp
    src/LogCleaner.cpp
    src/LogCleaner.h
    src/Utils.cpp
    src/Utils.h
    src/LogCleanerConsole.cpp
    src/LogCleanerConsole.h
)

target_compile_definitions(LogCleaner PRIVATE UNICODE _UNICODE)
target_link_libraries(LogCleaner PRIVATE comctl32 shlwapi dwmapi uxtheme)