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
    char selection = 0;
    while (selection != 'y')
    {
        printf("[y/n]: ");
        scanf("%c", &selection);
        if (selection == 'y' || selection == 'Y') return true;
        if (selection == 'n' || selection == 'N') return false;
        printf(" - invalid selection\n");
    }
    return false;
}