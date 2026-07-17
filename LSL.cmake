include_guard(GLOBAL)

target_sources(${PROJECT_NAME} PRIVATE
    src/data/lsl_eeg_source.c
    src/data/lsl_trigger_outlet.c
)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(liblsl
        GIT_REPOSITORY https://github.com/sccn/liblsl.git
        GIT_TAG        v1.16.2
)
FetchContent_MakeAvailable(liblsl)

target_link_libraries(${PROJECT_NAME} lsl)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${liblsl_SOURCE_DIR}/include
)