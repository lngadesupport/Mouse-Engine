add_library(MouseEngine.Core INTERFACE)

target_include_directories(MouseEngine.Core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(MouseEngine.Core INTERFACE cxx_std_20)

add_executable(MouseEngine.WorkspaceModelTest
    WorkspaceModelTest.cpp
)
target_link_libraries(MouseEngine.WorkspaceModelTest PRIVATE MouseEngine.Core)
target_compile_features(MouseEngine.WorkspaceModelTest PRIVATE cxx_std_20)
add_test(NAME MouseEngine.WorkspaceModelTest COMMAND MouseEngine.WorkspaceModelTest)
