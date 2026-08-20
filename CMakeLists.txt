cmake_minimum_required(VERSION 3.26)

project(tiny_bci_ssvep_experiment)

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(SOURCES
    src/demo/presenter_main.c
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
)

if (BUILD_FRONTEND)
  list(APPEND SOURCES src/presentation.c)
endif ()

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(${PROJECT_NAME} PRIVATE producer)


include(FetchContent)
set(TINY_BCI_GIT_TOKEN "" CACHE STRING "GitHub PAT for cloning private TinyBCI repo")

if (TINY_BCI_GIT_TOKEN)
  set(TINY_BCI_REPO_URL "https://${TINY_BCI_GIT_TOKEN}@github.com/BCI-Games/TinyBCI.git")
else()
  set(TINY_BCI_REPO_URL "https://github.com/BCI-Games/TinyBCI.git")
endif()

FetchContent_Declare(
        tiny_bci
        GIT_REPOSITORY ${TINY_BCI_REPO_URL}
        GIT_TAG 1590909aaba5bdf6d082996b6a4ba9559a55afcc
)
FetchContent_MakeAvailable(tiny_bci)
target_link_libraries(${PROJECT_NAME} tiny_bci)
target_include_directories(${PROJECT_NAME} PRIVATE ${tiny_bci_SOURCE_DIR}/producer)
target_sources(${PROJECT_NAME} PRIVATE
        ${tiny_bci_SOURCE_DIR}/producer/unicorn_producer.c
        ${tiny_bci_SOURCE_DIR}/producer/tbci_trigger_generator.c
)

option(INCLUDE_LSL "Build with LSL support" ON)
if (${INCLUDE_LSL})
  include(LSL.cmake)
endif()

option(BUILD_FRONTEND "Build with raylib presentation frontend" OFF)
if (BUILD_FRONTEND)
  include(raylib.cmake)
  target_link_libraries(${PROJECT_NAME} raylib)
endif()


set(CONFIGURATION_PATH_SUFFIX_EXPRESSION "$<$<BOOL:${CMAKE_CONFIGURATION_TYPES}>:/$<CONFIG>>")
set(FULL_RUNTIME_OUTPUT_PATH_EXPRESSION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}${CONFIGURATION_PATH_SUFFIX_EXPRESSION}")
set(RUNTIME_ASSET_OUTPUT_PATH_EXPRESSION "${FULL_RUNTIME_OUTPUT_PATH_EXPRESSION}/assets")

add_custom_target(copy_assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    "${CMAKE_SOURCE_DIR}/assets/"
    "${RUNTIME_ASSET_OUTPUT_PATH_EXPRESSION}"
  COMMENT "Copying assets to output folder: ${RUNTIME_ASSET_OUTPUT_PATH_EXPRESSION}"
)
add_dependencies(${PROJECT_NAME} copy_assets)