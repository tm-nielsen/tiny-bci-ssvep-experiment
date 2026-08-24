cmake_minimum_required(VERSION 3.26)

project(tiny_bci_ssvep_experiment)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include(cmake/TinyBCI.cmake)
include(cmake/LSL.cmake)
include(cmake/Raylib.cmake)
include(cmake/CopyAssets.cmake)

set(SHARED_SOURCES
  src/trial_conductor.c
  src/microsecond_timer.c
  src/storage.c
  src/pipeline.c
  src/triggers.c
  src/inference_logger.c

  src/cli/eeg_source_selection.c
  src/cli/eeg_selection_prompts.c

  src/data/synthetic_eeg_source.c
  src/data/serial_port_enumeration_unix.c
  src/data/serial_port_enumeration_windows.c
  src/data/serial_unix.c
  src/data/serial_windows.c
  src/data/serial_utils.c
  src/data/neuropawn_eeg_source.c
  src/data/unicorn_eeg_source.c
)

function(ADD_TARGET TARGET_NAME SOURCES)
  add_executable(${TARGET_NAME} ${SOURCES} ${ARGN} ${SHARED_SOURCES} ${LSL_SOURCES})

  set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
  target_precompile_headers(${TARGET_NAME} PRIVATE include/pch.h)
  target_include_directories(${TARGET_NAME} PRIVATE include)

  target_link_libraries(${TARGET_NAME} tiny_bci)
  target_link_libraries(${TARGET_NAME} lsl)
endfunction()

add_target(${PROJECT_NAME} src/main.c src/presentation.c)
target_link_libraries(${PROJECT_NAME} raylib)
add_dependencies(${PROJECT_NAME} copy_assets)

add_target("headless_${PROJECT_NAME}" src/main_headless.c)

set(PRESENTATION_TARGET "gutless_${PROJECT_NAME}")
add_target(${PRESENTATION_TARGET} src/main_gutless.c src/presentation.c)
target_link_libraries(${PRESENTATION_TARGET} raylib)
add_dependencies(${PRESENTATION_TARGET} copy_assets)