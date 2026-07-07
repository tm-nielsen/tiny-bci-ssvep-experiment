

int main(int argc, char *argv[])
{
    printf("SSVEP Experiment.\n");
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Tiny BCI SSVEP Experiment");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(DARKGRAY);
        DrawRectangle(50, 50, 100, 80, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return EXIT_SUCCESS;
}