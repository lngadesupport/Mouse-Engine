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

add_executable(MouseEngine.ObservationAnalysisTest
    ObservationAnalysisTest.cpp
)
target_link_libraries(MouseEngine.ObservationAnalysisTest PRIVATE MouseEngine.Core)
target_compile_features(MouseEngine.ObservationAnalysisTest PRIVATE cxx_std_20)
add_test(NAME MouseEngine.ObservationAnalysisTest COMMAND MouseEngine.ObservationAnalysisTest)

add_executable(MouseEngine.WorkspaceStoreTest
    WorkspaceStoreTest.cpp
)
target_link_libraries(MouseEngine.WorkspaceStoreTest PRIVATE MouseEngine.Core)
target_compile_features(MouseEngine.WorkspaceStoreTest PRIVATE cxx_std_20)
add_test(NAME MouseEngine.WorkspaceStoreTest COMMAND MouseEngine.WorkspaceStoreTest)

add_executable(MouseEngine.SessionRecorderTest
    SessionRecorderTest.cpp
)
target_link_libraries(MouseEngine.SessionRecorderTest PRIVATE MouseEngine.Core)
target_compile_features(MouseEngine.SessionRecorderTest PRIVATE cxx_std_20)
add_test(NAME MouseEngine.SessionRecorderTest COMMAND MouseEngine.SessionRecorderTest)

add_executable(MouseEngine.SessionSerializerTest
    SessionSerializerTest.cpp
)
target_link_libraries(MouseEngine.SessionSerializerTest PRIVATE MouseEngine.Core)
target_compile_features(MouseEngine.SessionSerializerTest PRIVATE cxx_std_20)
add_test(NAME MouseEngine.SessionSerializerTest COMMAND MouseEngine.SessionSerializerTest)
