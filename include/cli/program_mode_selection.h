# pragma once
# ifndef PROGRAM_MODES
# define PROGRAM_MODES

typedef enum {
    STANDALONE,
    PRESENTATION_ONLY
} ProgramMode;

ProgramMode promptProgramModeSelection(void);

void initializeProgram(ProgramMode mode);
void awaitPromptedProgramStart(void);
void updateProgram(void);
void cleanUpProgram(void);

# endif