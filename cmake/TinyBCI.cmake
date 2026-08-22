include_guard(GLOBAL)

include(FetchContent)
FetchContent_Declare(
  tiny_bci
  GIT_REPOSITORY https://github.com/BCI-Games/TinyBCI.git
  GIT_TAG 1590909aaba5bdf6d082996b6a4ba9559a55afcc
)
FetchContent_MakeAvailable(tiny_bci)