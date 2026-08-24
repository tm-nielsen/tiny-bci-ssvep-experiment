# include "cli/program_mode_selection.h"
# include "cli/eeg_source_selection.h"

# include "raylib.h"

int main(int argc, char *argv[])
{
    ProgramMode mode = promptProgramModeSelection();
    if (mode == Standalone) runEEGSourceSelection();

    initializeProgram(mode);
    awaitPromptedProgramStart();

    while (!WindowShouldClose()) updateProgram();

    cleanUpProgram();
    return EXIT_SUCCESS;
}