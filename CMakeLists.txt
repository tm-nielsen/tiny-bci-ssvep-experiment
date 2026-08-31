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
  src/inference_logger.c

  src/pipeline/pipeline.c
  src/pipeline/initialization.c
  src/pipeline/storage.c
  src/pipeline/triggers.c
  src/pipeline/helpers.c

  src/program/helpers.c
  src/program/pipeline_helpers.c

  src/cli/helpers.c
  src/cli/eeg_source_selection.c
  src/cli/eeg_selection_prompts.c
  
  src/serial/unix.c
  src/serial/windows.c
  src/serial/utils.c
  src/serial/data_source.c
  src/serial/data_source_reading.c
  src/serial/unix_port_enumeration.c
  src/serial/windows_port_enumeration.c

  src/data/synthetic_eeg_source.c
  src/data/neuropawn_eeg_source.c
  src/data/neuropawn_board_configuration.c
  src/data/unicorn_eeg_source.c
  src/data/dsi_eeg_source.c
  src/data/DSI_API_Loader.c
)

function(ADD_TARGET TARGET_NAME SOURCES)
  add_executable(${TARGET_NAME} ${SOURCES} ${ARGN} ${SHARED_SOURCES} ${LSL_SOURCES})

  set_target_properties(${TARGET_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
  target_precompile_headers(${TARGET_NAME} PRIVATE include/pch.h)
  target_include_directories(${TARGET_NAME} PRIVATE include)

  target_link_libraries(${TARGET_NAME} tiny_bci)
  target_link_libraries(${TARGET_NAME} lsl)
endfunction()

add_target(${PROJECT_NAME}
  src/program/main.c
  src/presentation.c
  src/program/presentation_helpers.c
  src/cli/program_mode_selection.c
)
target_link_libraries(${PROJECT_NAME} raylib)
add_dependencies(${PROJECT_NAME} copy_assets)

add_target("headless_${PROJECT_NAME}" src/program/main_headless.c)