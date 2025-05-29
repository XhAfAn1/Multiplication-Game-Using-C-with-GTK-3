#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define SIZE 36
#define WIDTH 6
#define HEIGHT 6
#define SAVE_FILE "game_save.dat"

// Board arranged in a 6x6 grid
int board[SIZE] ={
    1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 12, 14,
    15, 16, 18, 20, 21, 24,
    25, 27, 28, 30, 32, 35,
    36, 40, 42, 45, 48, 49,
    54, 56, 63, 64, 72, 81
};

int player_moves[SIZE] = {0}; // 0=none, 1=player, 2=computer
int player_score = 0, computer_score = 0;
int com_choice = -1;
int game_over = 0;

// Simulated CPU Registers
typedef struct
{
    int regA;
    int regB;
    int acc;
} CPU;

CPU cpu;

// Multiplies two numbers using bitwise operations for simulation
int multiplication(int *a, int *b)
{
    cpu.regA = *a;
    cpu.regB = *b;
    cpu.acc = 0;

    int x = *a;
    int y = *b;

    while (y > 0)
    {
        if (y & 1)
            cpu.acc += x;
        x <<= 1;
        y >>= 1;
    }

    return cpu.acc;
}

// Gets the index of a number in the game board
int getIndex(int idx)
{
    for (int i = 0; i < SIZE; i++)
    {
        if (board[i] == idx)
            return i;
    }
    return -1;
}

// Update score for the specified player
void updateScore(int player_id)
{
    if (player_id == 1)
        player_score++;
    if (player_id == 2)
        computer_score++;
}

// Display the game board with colors
void display()
{

    printf("\033[0m"); // Reset color attributes

    printf("\n\n+-----------------------+\n");
    for (int i = 0; i < SIZE; i++)
    {
        printf("|");
        if (player_moves[i] == 1)
            printf(" \033[1;32mP\033[0m ");
        else if (player_moves[i] == 2)
            printf(" \033[1;31mC\033[0m ");
        else if (board[i] > 0 && board[i] < 10)
            printf(" %d ", board[i]);
        else
            printf("%d ", board[i]);

        if ((i + 1) % WIDTH == 0)
        {
            printf("|\n+-----------------------+\n");
        }
    }

    printf("Player Score: \033[1;32m%d\033[0m \t Computer Score: \033[1;31m%d\033[0m\n", player_score, computer_score);
}

// Animation for computer's thinking
void animate_thinking()
{
    printf("\nComputer thinking");
    fflush(stdout);
    for (int i = 0; i < 3; i++)
    {
        printf(".");
        fflush(stdout);
        usleep(500000);
    }
    printf("\n");
}

// Clear screen with delay
void clear_screen(int t)
{
    sleep(t);
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Check if board is full (draw condition)
int isBoardFull()
{
    for (int i = 0; i < SIZE; i++)
    {
        if (player_moves[i] == 0)
            return 0;
    }
    return 1;
}

// Check for 4-in-a-row win condition
int checkWin(int player_id)
{
    // Rows
    for (int row = 0; row < HEIGHT; row++)
    {
        for (int col = 0; col <= WIDTH - 4; col++)
        {
            int idx = row * WIDTH + col;
            if (player_moves[idx] == player_id &&
                    player_moves[idx + 1] == player_id &&
                    player_moves[idx + 2] == player_id &&
                    player_moves[idx + 3] == player_id)
                return 1;
        }
    }
    // Columns
    for (int col = 0; col < WIDTH; col++)
    {
        for (int row = 0; row <= HEIGHT - 4; row++)
        {
            int idx = row * WIDTH + col;
            if (player_moves[idx] == player_id &&
                    player_moves[idx + WIDTH] == player_id &&
                    player_moves[idx + 2 * WIDTH] == player_id &&
                    player_moves[idx + 3 * WIDTH] == player_id)
                return 1;
        }
    }
    // Diagonals (Top-Left to Bottom-Right)
    for (int row = 0; row <= HEIGHT - 4; row++)
    {
        for (int col = 0; col <= WIDTH - 4; col++)
        {
            int idx = row * WIDTH + col;
            if (player_moves[idx] == player_id &&
                    player_moves[idx + WIDTH + 1] == player_id &&
                    player_moves[idx + 2 * (WIDTH + 1)] == player_id &&
                    player_moves[idx + 3 * (WIDTH + 1)] == player_id)
                return 1;
        }
    }
    // Diagonals (Top-Right to Bottom-Left)
    for (int row = 0; row <= HEIGHT - 4; row++)
    {
        for (int col = 3; col < WIDTH; col++)
        {
            int idx = row * WIDTH + col;
            if (player_moves[idx] == player_id &&
                    player_moves[idx + WIDTH - 1] == player_id &&
                    player_moves[idx + 2 * (WIDTH - 1)] == player_id &&
                    player_moves[idx + 3 * (WIDTH - 1)] == player_id)
                return 1;
        }
    }
    return 0;
}

// Check if a move is valid and apply it
int moveCheck(int player_id, int num1, int num2)
{
    int result = multiplication(&num1, &num2);
    int idx = getIndex(result);
    if (idx != -1 && player_moves[idx] == 0)
    {
        player_moves[idx] = player_id;
        return idx;
    }
    return -1;
}

// Calculate a position's strategic value (0-10)
int positionEvaluate(int idx)
{
    int value = 0;
    int row = idx / WIDTH;
    int col = idx % WIDTH;

    // Prefer center positions
    int center_row = HEIGHT / 2;
    int center_col = WIDTH / 2;
    int row_distance = abs(row - center_row);
    int col_distance = abs(col - center_col);
    value += 4 - (row_distance + col_distance);

    // Prefer positions that could form a line
    for (int player_id = 1; player_id <= 2; player_id++)
    {
        int multiplier = (player_id == 2) ? 1 : -1; // Prefer own lines, avoid opponent's

        // Check rows
        int row_count = 0;
        for (int c = 0; c < WIDTH; c++)
        {
            int check_idx = row * WIDTH + c;
            if (player_moves[check_idx] == player_id)
                row_count++;
        }
        value += multiplier * row_count;

        // Check columns
        int col_count = 0;
        for (int r = 0; r < HEIGHT; r++)
        {
            int check_idx = r * WIDTH + col;
            if (player_moves[check_idx] == player_id)
                col_count++;
        }
        value += multiplier * col_count;

        // Diagonal checks (simplified)
        if ((row == col) || (row + col == WIDTH - 1))
        {
            value += multiplier * 2; // Diagonals are valuable
        }
    }

    // Add some randomness to prevent predictable play
    value += rand() % 3;

    return value;
}

// Advanced computer move logic
void compMove(int player_num)
{
    int comp_num = -1;
    int best_idx = -1;
    int best_value = -1000;

    //find a winning move
    for (int i = 1; i <= 9; i++)
    {
        int result = multiplication(&i, &player_num);
        int idx = getIndex(result);
        if (idx != -1 && player_moves[idx] == 0)
        {

            player_moves[idx] = 2;
            if (checkWin(2))
            {
                comp_num = i;
                best_idx = idx;
                player_moves[idx] = 0;
                break;
            }
            player_moves[idx] = 0;
        }
    }

    // block player's potential win
    if (comp_num == -1)
    {
        for (int i = 1; i <= 9; i++)
        {
            int result = multiplication(&i, &player_num);
            int idx = getIndex(result);
            if (idx != -1 && player_moves[idx] == 0)
            {

                player_moves[idx] = 1;
                if (checkWin(1))
                {
                    comp_num = i;
                    best_idx = idx;
                    player_moves[idx] = 0;
                    break;
                }
                player_moves[idx] = 0;
            }
        }
    }

    //Look for 3-in-a-row
    if (comp_num == -1)
    {

        for (int i = 1; i <= 9; i++)
        {
            int result = multiplication(&i, &player_num);
            int idx = getIndex(result);
            if (idx != -1 && player_moves[idx] == 0)
            {
                player_moves[idx] = 2;


                int row = idx / WIDTH;
                int count = 0;
                for (int col = 0; col < WIDTH; col++)
                {
                    if (player_moves[row * WIDTH + col] == 2)
                        count++;
                }
                if (count >= 3)
                {
                    comp_num = i;
                    best_idx = idx;
                    player_moves[idx] = 0;
                    break;
                }


                int col = idx % WIDTH;
                count = 0;
                for (int row = 0; row < HEIGHT; row++)
                {
                    if (player_moves[row * WIDTH + col] == 2)
                        count++;
                }
                if (count >= 3)
                {
                    comp_num = i;
                    best_idx = idx;
                    player_moves[idx] = 0;
                    break;
                }

                player_moves[idx] = 0;
            }
        }
    }

    //evaluate
    if (comp_num == -1)
    {
        for (int i = 1; i <= 9; i++)
        {
            int result = multiplication(&i, &player_num);
            int idx = getIndex(result);
            if (idx != -1 && player_moves[idx] == 0)
            {
                int value = positionEvaluate(idx);
                if (value > best_value)
                {
                    best_value = value;
                    comp_num = i;
                    best_idx = idx;
                }
            }
        }
    }

    // Apply move
    if (comp_num != -1)
    {
        player_moves[best_idx] = 2;
        int result = multiplication(&comp_num, &player_num);
        printf("\nComputer chooses: %d => multiplication result: %d x %d = %d\n",
               comp_num, comp_num, player_num, result);
        com_choice = comp_num;

        if (checkWin(2))
        {
            clear_screen(2);
            display();
            updateScore(2);
            printf("\nComputer Wins by 4 in a line!\n");
            game_over = 1;
        }
    }
}

// Display CPU register state
void display_registers()
{
    printf("CPU State:\n");
    printf("regA: %d\n", cpu.regA);
    printf("regB: %d\n", cpu.regB);
    printf("acc: %d\n", cpu.acc);
}

// Save game state to file
int save_game()
{
    FILE *fp = fopen(SAVE_FILE, "wb");
    if (!fp)
    {
        printf("Error opening save file!\n");
        return 0;
    }

    // Save player move matrix
    fwrite(player_moves, sizeof(int), SIZE, fp);

    // Save scores
    fwrite(&player_score, sizeof(int), 1, fp);
    fwrite(&computer_score, sizeof(int), 1, fp);

    // Save computer choice
    fwrite(&com_choice, sizeof(int), 1, fp);

    // Save game state
    fwrite(&game_over, sizeof(int), 1, fp);

    fclose(fp);
    printf("Game saved successfully!\n");
    return 1;
}

// Load game state from file
int load_game()
{
    FILE *fp = fopen(SAVE_FILE, "rb");
    if (!fp)
    {
        printf("No saved game found.\n");
        return 0;
    }

    // Load player move matrix
    if (fread(player_moves, sizeof(int), SIZE, fp) != SIZE)
    {
        printf("Error reading save file!\n");
        fclose(fp);
        return 0;
    }

    // Load scores
    if (fread(&player_score, sizeof(int), 1, fp) != 1 ||
            fread(&computer_score, sizeof(int), 1, fp) != 1)
    {
        printf("Error reading save file!\n");
        fclose(fp);
        return 0;
    }

    // Load computer choice
    if (fread(&com_choice, sizeof(int), 1, fp) != 1)
    {
        printf("Error reading save file!\n");
        fclose(fp);
        return 0;
    }

    // Load game state
    if (fread(&game_over, sizeof(int), 1, fp) != 1)
    {
        printf("Error reading save file!\n");
        fclose(fp);
        return 0;
    }

    fclose(fp);
    printf("Game loaded successfully!\n");
    return 1;
}

// Play the game
void playGame()
{
    int choice;
    srand(time(NULL));

    // Select initial computer choice if not loaded from save
    if (com_choice == -1)
    {
        com_choice = (rand() % 9) + 1;
    }

    while (!isBoardFull() && !game_over)
    {
        // Reset terminal color at start of each loop
        printf("\033[0m");

        printf("\nWelcome to the Multiplication Game!\n");
        display();

        printf("\nComputer has chosen: %d\n\nPress 0 to see current CPU State\n", com_choice);
        printf("Press -1 to save game\nPress -2 to load game\n");
        printf("Enter a number (1-9): ");

        if (scanf("%d", &choice) != 1)
        {
            // Clear input buffer if invalid input
            while (getchar() != '\n');
            printf("Invalid input. Please enter a number.\n");
            clear_screen(2);
            continue;
        }

        if (choice == 0)
        {
            display_registers();
            printf("\nPress Enter to continue...");
            getchar(); // Consume newline
            getchar(); // Wait for Enter
            clear_screen(1);
            continue;
        }

        if (choice == -1)
        {
            save_game();
            printf("\nPress Enter to continue...");
            getchar(); // Consume newline
            getchar(); // Wait for Enter
            clear_screen(1);
            continue;
        }

        if (choice == -2)
        {
            load_game();
            printf("\nPress Enter to continue...");
            getchar(); // Consume newline
            getchar(); // Wait for Enter
            clear_screen(1);
            continue;
        }

        if (choice < 1 || choice > 9)
        {
            printf("Invalid input. Please enter a number between 1 and 9.\n");
            clear_screen(2);
            continue;
        }

        int idx = moveCheck(1, choice, com_choice);
        if (idx != -1)
        {
            int result = multiplication(&choice, &com_choice);
            printf("You chose: %d => multiplication result: %d x %d = %d\n",
                   choice, choice, com_choice, result);

            if (checkWin(1))
            {
                clear_screen(1);
                display();
                updateScore(1);
                printf("\nYou Win by 4 in a line!\n");
                game_over = 1;
                break;
            }
        }
        else
        {
            printf("Invalid move. The result %d x %d is either not on the board or already taken.\n",
                   choice, com_choice);
            clear_screen(2);
            continue;
        }

        clear_screen(1);

        // Computer's turn
        printf("\nWelcome to the Multiplication Game!\n");
        display();
        animate_thinking();
        compMove(choice);
        clear_screen(2);
    }

    if (!game_over)
    {
        display();
        printf("\n====== GAME OVER ======\nIt's a tie. No 4 in a row or column achieved.\n");
    }

    printf("\nFinal Score - Player: %d  Computer: %d\n", player_score, computer_score);
    printf("\nWould you like to play again? (1=Yes, 0=No): ");
    int play_again;
    scanf("%d", &play_again);

    if (play_again == 1)
    {
        // Reset game state
        memset(player_moves, 0, sizeof(player_moves));
        game_over = 0;
        com_choice = -1;
        clear_screen(1);
        playGame();
    }
}

int main()
{
    // Initialize terminal for color support
#ifdef _WIN32
    // For Windows
    system("color");
#else
    // For UNIX/Linux/MacOS
    printf("\033[0m\033[1m"); // Reset and set bold
    printf("\033[0m");        // Reset again
#endif

    printf("\n===================================\n");
    printf("  MULTIPLICATION STRATEGY GAME");
    printf("\n===================================\n");
    printf("\nRules:\n");
    printf("# The computer will choose a number (1-9)\n");
    printf("# You choose a number (1-9)\n");
    printf("# The product of these numbers will be marked on the board\n");
    printf("# Get 4 in a row, column, or diagonal to win\n");
    printf("# Press 0 to view the CPU simulation state\n");
    printf("# Press -1 to save your game\n");
    printf("# Press -2 to load a saved game\n");
    printf("\nDo you want to load a saved game? (1=Yes, 0=No): ");

    int load_choice;
    scanf("%d", &load_choice);

    if (load_choice == 1)
    {
        if (!load_game())
        {
            printf("Starting new game instead.\n");
            sleep(2);
        }
    }

    printf("\nPress Enter to start...");
    getchar(); // Consume newline
    getchar(); // Wait for Enter

    playGame();

    return 0;
}
