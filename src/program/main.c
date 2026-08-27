# include "cli/program_mode_selection.h"
# include "cli/eeg_source_selection.h"

# include "raylib.h"

# ifndef putenv
#   define putenv _putenv
# endif

int main(int argc, char *argv[])
{
    putenv("MESA_GL_VERSION_OVERRIDE=3.3");
    putenv("MESA_GLSL_VERSION_OVERRIDE=330");

    ProgramMode mode = promptProgramModeSelection();
    if (mode == Standalone) runEEGSourceSelection();

    initializeProgram(mode);
    awaitPromptedProgramStart();

    while (!WindowShouldClose()) updateProgram();

    cleanUpProgram();
    return EXIT_SUCCESS;
}