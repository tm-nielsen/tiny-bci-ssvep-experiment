cmake_minimum_required(VERSION 3.26)

option(TBCI_PICO "Build the headless engine for the Raspberry Pi Pico 2 W" OFF)
option(TBCI_MEASURE "Build tick logging and GPIO markers into the headless target" OFF)

if (TBCI_PICO)
  # Must come before project(): selects the board and the ARM toolchain.
  set(PICO_BOARD pico2_w)
  set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk")
  include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

  project(tiny_bci_ssvep_experiment LANGUAGES C CXX ASM)
  pico_sdk_init()
else()
  project(tiny_bci_ssvep_experiment)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include(cmake/TinyBCI.cmake)

# --- sources that build everywhere

set(CORE_SOURCES
    src/microsecond_timer.c

    src/pipeline/pipeline.c
    src/pipeline/initialization.c
    src/pipeline/storage.c
    src/pipeline/triggers.c
    src/pipeline/helpers.c

    src/serial/utils.c
    src/serial/data_source.c
    src/serial/data_source_reading.c

    src/data/unicorn_eeg_source.c
    src/serial/bci_link.c
    src/engine/serial_engine.c

)

if (TBCI_PICO)

  # --- Pico 2 W firmware

  set(PICO_TARGET headless_${PROJECT_NAME}_pico)

  add_executable(${PICO_TARGET}
          ${CORE_SOURCES}
          src/serial/pico_rfcomm.c
          src/program/main_headless_pico.c
          src/serial/bci_link.c
          src/inference_logger.c
          src/engine/usb_link.c
          src/engine/usb_descriptors.c
          src/engine/link_hub.c
          src/engine/link_outputs.c
  )

  target_compile_definitions(${PICO_TARGET} PRIVATE
          TBCI_PICO=1
          CYW43_LWIP=0   # Bluetooth only, no Wi-Fi stack
  )

  target_precompile_headers(${PICO_TARGET} PRIVATE include/pch.h)
  target_include_directories(${PICO_TARGET} PRIVATE
          include
          include/config/pico   # btstack_config.h
  )

  target_link_libraries(${PICO_TARGET}
          pico_stdlib
          pico_cyw43_arch_poll   # radio driven by cyw43_arch_poll() from our loop
          pico_btstack_classic   # Bluetooth Classic: inquiry, SDP, RFCOMM
          pico_btstack_cyw43
          tiny_bci
          tinyusb_device
          pico_unique_id
  )

  # First milestone: printf over the Pico's USB serial port.
  # Replaced by the dual-port setup when we add the presenter protocol.
  pico_enable_stdio_usb(${PICO_TARGET} 0)
  pico_enable_stdio_uart(${PICO_TARGET} 0)

  pico_add_extra_outputs(${PICO_TARGET})  # produces the .uf2

else()

  # --- desktop

  include(cmake/LSL.cmake)
  include(cmake/Raylib.cmake)
  include(cmake/CopyAssets.cmake)

  set(SHARED_SOURCES
      ${CORE_SOURCES}

      src/trial_conductor.c
      src/inference_logger.c

      src/program/helpers.c
      src/program/pipeline_helpers.c

      src/cli/helpers.c
      src/cli/eeg_source_selection.c
      src/cli/eeg_selection_prompts.c
      src/cli/recording_options.c

      src/serial/unix.c
      src/serial/windows.c
      src/serial/unix_port_enumeration.c
      src/serial/windows_port_enumeration.c

      src/data/synthetic_eeg_source.c
      src/data/neuropawn_eeg_source.c
      src/data/neuropawn_board_configuration.c
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
          src/stimuli/stimulus_pattern.c
          src/program/presentation_helpers.c
          src/cli/program_mode_selection.c
  )
  target_link_libraries(${PROJECT_NAME} raylib)
  add_dependencies(${PROJECT_NAME} copy_assets)

  add_target("headless_${PROJECT_NAME}" src/program/main_headless.c)
  if (TBCI_MEASURE)
    if (WIN32)
      message(WARNING "TBCI_MEASURE is not supported on Windows; building without measurement hooks")
    else()
      target_sources("headless_${PROJECT_NAME}" PRIVATE
              src/tick_logger.c
              src/measurements/gpio_marker.c)
      target_compile_definitions("headless_${PROJECT_NAME}" PRIVATE TBCI_MEASURE)
    endif()
  endif()

  set(SSVEP_BOX_TARGET "ssvep_box_${PROJECT_NAME}")
  add_target(${SSVEP_BOX_TARGET} src/program/main_ssvep_box.c)
  target_compile_definitions(${SSVEP_BOX_TARGET} PRIVATE N_FREQS=4 FREQUENCIES={4.5f,5.0f,5.5f,6.0f})
endif()