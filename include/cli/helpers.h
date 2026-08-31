# pragma once
# ifndef USER_PROMPT_HELPERS
# define USER_PROMPT_HELPERS

uint32_t getCLIIntegerSelection(uint32_t maximum);
bool getCLIYesNo(void);

void flushInput(void);
void awaitCLINewline(void);

# endif