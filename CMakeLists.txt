cmake_minimum_required(VERSION 3.12)

project(tiny_bci_ssvep_experiment)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(SUBMODULE_PATH ${CMAKE_SOURCE_DIR}/thirdparty)


set(RAYLIB_VERSION 6.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()


add_executable(${PROJECT_NAME}
    src/main.c
    src/trial_conductor.c
    src/presentation.c
    src/microsecond_timer.c
    src/storage.c
    src/pipeline.c

    src/data/trigger_source.c
    src/data/synthetic_eeg_source.c
    #src/data/serial_unix.c
    src/data/serial_windows.c
    src/data/unicorn_eeg_source.c
    thirdparty/tiny_bci/producer/tbci_trigger_generator.c
    thirdparty/tiny_bci/producer/unicorn_producer.c
    #src/data/neuropawn_eeg_source.c
)
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE producer)


target_link_libraries(${PROJECT_NAME} raylib)

# TODO: migrate to fetch once CMake is fixed
add_subdirectory(${SUBMODULE_PATH}/tiny_bci)
target_link_libraries(${PROJECT_NAME} tiny_bci)