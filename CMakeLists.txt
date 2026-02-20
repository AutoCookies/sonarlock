cmake_minimum_required(VERSION 3.20)
project(sonarlock VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SONARLOCK_BUILD_TESTS "Build tests" ON)
option(SONARLOCK_ENABLE_PORTAUDIO "Enable PortAudio backend if available" ON)

add_library(sonarlock_core
    src/core/logger.cpp
    src/core/sine_generator.cpp
    src/core/dsp_pipeline.cpp
    src/core/dsp_primitives.cpp
    src/core/motion_detection.cpp
    src/core/session_controller.cpp
)
target_include_directories(sonarlock_core PUBLIC include)

target_compile_features(sonarlock_core PUBLIC cxx_std_20)

add_library(sonarlock_audio
    src/audio/fake_audio_backend.cpp
    src/audio/audio_factory.cpp
)
target_include_directories(sonarlock_audio PUBLIC include)
target_link_libraries(sonarlock_audio PUBLIC sonarlock_core)

if(SONARLOCK_ENABLE_PORTAUDIO)
    find_package(PortAudio QUIET)
    if(PortAudio_FOUND)
        target_sources(sonarlock_audio PRIVATE src/audio/portaudio_backend.cpp)
        target_compile_definitions(sonarlock_audio PUBLIC SONARLOCK_HAS_PORTAUDIO=1)
        target_link_libraries(sonarlock_audio PRIVATE PortAudio::PortAudio)
    else()
        message(STATUS "PortAudio not found; real backend unavailable.")
        target_sources(sonarlock_audio PRIVATE src/audio/portaudio_backend.cpp)
    endif()
else()
    target_sources(sonarlock_audio PRIVATE src/audio/portaudio_backend.cpp)
endif()

add_library(sonarlock_app
    src/app/cli_parser.cpp
)
target_include_directories(sonarlock_app PUBLIC include)
target_link_libraries(sonarlock_app PUBLIC sonarlock_core)

add_executable(sonarlock src/app/main.cpp)
target_link_libraries(sonarlock PRIVATE sonarlock_app sonarlock_core sonarlock_audio)

include(CTest)
if(SONARLOCK_BUILD_TESTS AND BUILD_TESTING)
    add_executable(sonarlock_tests tests/test_main.cpp)
    target_link_libraries(sonarlock_tests PRIVATE sonarlock_app sonarlock_core sonarlock_audio)
    add_test(NAME sonarlock_tests COMMAND sonarlock_tests)
endif()
