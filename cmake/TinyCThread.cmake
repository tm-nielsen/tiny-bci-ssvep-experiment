include_guard(GLOBAL)

include(FetchContent)
FetchContent_Declare(
  tinycthread
  GIT_REPOSITORY https://github.com/gyrovorbis/tinycthread
)
FetchContent_MakeAvailable(tinycthread)