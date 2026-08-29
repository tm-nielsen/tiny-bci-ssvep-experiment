# include "cli/helpers.h"

uint32_t getCLIIntegerSelection(uint32_t maximum)
{
    unsigned int selection = 0;
    scanf("%u", &selection);

    while (selection > maximum)
    {
        printf(" - invalid selection\n");
        scanf("%u", &selection);
    }
    
    return selection;
}

bool getCLIYesNo()
{
    while (true)
    {
        printf("[y/n]: ");
        flushInput();
        int selection = 0;
        while ((selection = getchar()) == '\n');

        if (selection == 'y' || selection == 'Y' || selection == '1') return true;
        if (selection == 'n' || selection == 'N' || selection == '0') return false;

        printf(" - invalid selection\n");
    }
    return false;
}

void flushInput()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void awaitCLINewline()
{
    flushInput();
    getchar();
}