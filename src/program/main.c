# include "cli/program_mode_selection.h"
# include "cli/eeg_source_selection.h"

# include "raylib.h"

int main(int argc, char *argv[])
{
# if defined(_WIN32) || defined(_WIN64)
# else
    printf("Setting GL Version overrides for Raspberry Pi...\n");
    putenv("MESA_GL_VERSION_OVERRIDE=3.3");
    putenv("MESA_GLSL_VERSION_OVERRIDE=330");
# endif

    ProgramMode mode = promptProgramModeSelection();
    if (mode == STANDALONE) runEEGSourceSelection();

    initializeProgram(mode);
    awaitPromptedProgramStart();

    while (!WindowShouldClose()) updateProgram();

    cleanUpProgram();
    return EXIT_SUCCESS;
}