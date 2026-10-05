cmake_minimum_required(VERSION 3.25)

project(MouseEngine
    VERSION 1.0.0
    LANGUAGES CXX
)

enable_testing()

add_subdirectory(src/MouseEngine.Core)

if(WIN32)
    add_subdirectory(src/MouseEngine.Host.Windows)
endif()
