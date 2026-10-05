if(NOT WIN32)
    return()
endif()

set(WEBVIEW2_SDK_DIR "" CACHE PATH "Pinned Microsoft WebView2 SDK directory")

add_executable(MouseEngine.Host.Windows WIN32
    main.cpp
    InputTiming.cpp
    InputTiming.h
    RawInputClassification.cpp
    RawInputClassification.h
)

target_compile_features(MouseEngine.Host.Windows PRIVATE cxx_std_20)
target_compile_definitions(MouseEngine.Host.Windows PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(MouseEngine.Host.Windows PRIVATE
    MouseEngine.Core
    user32
    shell32
    ole32
    shlwapi
    setupapi
    hid
    cfgmgr32
)

if(WEBVIEW2_SDK_DIR)
    set(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_SDK_DIR}/build/native/include")
    set(WEBVIEW2_LIB "${WEBVIEW2_SDK_DIR}/build/native/x64/WebView2LoaderStatic.lib")
    if(EXISTS "${WEBVIEW2_INCLUDE_DIR}/WebView2.h" AND EXISTS "${WEBVIEW2_LIB}")
        target_include_directories(MouseEngine.Host.Windows PRIVATE "${WEBVIEW2_INCLUDE_DIR}")
        target_link_libraries(MouseEngine.Host.Windows PRIVATE "${WEBVIEW2_LIB}")
        target_compile_definitions(MouseEngine.Host.Windows PRIVATE MOUSE_ENGINE_WEBVIEW2_SDK=1)
    endif()
endif()

set_target_properties(MouseEngine.Host.Windows PROPERTIES
    OUTPUT_NAME "MouseEngine.Host.Windows"
    WIN32_EXECUTABLE TRUE
)

add_executable(MouseEngine.InputTimingTest
    InputTimingTest.cpp
    InputTiming.cpp
    InputTiming.h
)
target_compile_features(MouseEngine.InputTimingTest PRIVATE cxx_std_20)
target_compile_definitions(MouseEngine.InputTimingTest PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
add_test(NAME MouseEngine.InputTimingTest COMMAND MouseEngine.InputTimingTest)


add_executable(MouseEngine.RawInputClassificationTest
    RawInputClassificationTest.cpp
    RawInputClassification.cpp
    RawInputClassification.h
)
target_compile_features(MouseEngine.RawInputClassificationTest PRIVATE cxx_std_20)
target_compile_definitions(MouseEngine.RawInputClassificationTest PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
add_test(NAME MouseEngine.RawInputClassificationTest COMMAND MouseEngine.RawInputClassificationTest)
