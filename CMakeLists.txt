cmake_minimum_required(VERSION 3.26)

project(tiny_bci_ssvep_experiment)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


set(SOURCES
  src/main.c
  src/trial_conductor.c
  src/presentation.c
  src/microsecond_timer.c
  src/storage.c
  src/pipeline.c
  src/triggers.c
  src/inference_logger.c

  src/data/synthetic_eeg_source.c
  src/data/serial_unix.c
  src/data/serial_windows.c
  src/data/serial_utils.c
  src/data/neuropawn_eeg_source.c
)
add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE include)


include(FetchContent)
FetchContent_Declare(
  tiny_bci
  GIT_REPOSITORY https://github.com/BCI-Games/TinyBCI.git
  GIT_TAG 1590909aaba5bdf6d082996b6a4ba9559a55afcc
)
FetchContent_MakeAvailable(tiny_bci)
target_link_libraries(${PROJECT_NAME} tiny_bci)

include(cmake/Raylib.cmake)
include(cmake/LSL.cmake)
include(cmake/CopyAssets.cmake)