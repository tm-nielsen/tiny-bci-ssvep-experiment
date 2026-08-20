cmake_minimum_required(VERSION 3.26)

project(tiny_bci_ssvep_experiment)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(SUBMODULE_PATH ${CMAKE_SOURCE_DIR}/thirdparty)

option(BUILD_FRONTEND "Build with raylib presentation frontend" OFF)

if (BUILD_FRONTEND)
  set(RAYLIB_VERSION 6.0)
  find_package(raylib ${RAYLIB_VERSION} QUIET)
  if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            raylib
            DOWNLOAD_EXTRACT_TIMESTAMP OFF
            URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    )
    FetchContent_GetProperties(raylib)
    if (NOT raylib_POPULATED)
      set(FETCHCONTENT_QUIET NO)
      FetchContent_MakeAvailable(raylib)
    endif()
  endif()
endif()

set(SOURCES
        src/main.c
        src/trial_conductor.c
        src/microsecond_timer.c
        src/storage.c
        src/pipeline.c
        src/inference_logger.c

        src/data/trigger_source.c
        src/data/synthetic_eeg_source.c
        src/data/serial_unix.c
        src/data/serial_windows.c
        src/data/serial_utils.c
        src/data/neuropawn_eeg_source.c
        src/data/unicorn_eeg_source.c
        thirdparty/tiny_bci/producer/tbci_trigger_generator.c
        thirdparty/tiny_bci/producer/unicorn_producer.c
)

if (BUILD_FRONTEND)
  list(APPEND SOURCES src/presentation.c)
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE producer)

if (BUILD_FRONTEND)
  target_link_libraries(${PROJECT_NAME} raylib)
endif()

add_subdirectory(${SUBMODULE_PATH}/tiny_bci)
target_link_libraries(${PROJECT_NAME} tiny_bci)

option(INCLUDE_LSL "Build with LSL support" ON)
if (${INCLUDE_LSL})
  include(LSL.cmake)
endif()

