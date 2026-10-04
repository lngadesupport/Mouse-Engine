add_library(MouseEngine.Core INTERFACE)

target_include_directories(MouseEngine.Core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(MouseEngine.Core INTERFACE cxx_std_20)
