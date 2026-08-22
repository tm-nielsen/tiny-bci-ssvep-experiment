include_guard(GLOBAL)

set(LSL_SOURCES
    src/lsl/inlet_helpers.c
    src/lsl/outlet_helpers.c
    src/lsl/data_source.c

    src/lsl/eeg_source.c

    src/lsl/trigger_outlet.c
    src/lsl/trigger_source.c
    src/lsl/inference_outlet.c
    src/lsl/inference_source.c
)

target_sources(${PROJECT_NAME} PRIVATE ${LSL_SOURCES})

option(USE_LSL_TIMESTAMPS "Use EEG  timestamps from source, marking triggers with lsl_local_clock" OFF)
if (${USE_LSL_TIMESTAMPS})
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LSL_TIMESTAMPS)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(liblsl
    GIT_REPOSITORY https://github.com/sccn/liblsl.git
    GIT_TAG        v1.16.2
)
FetchContent_MakeAvailable(liblsl)

target_link_libraries(${PROJECT_NAME} lsl)