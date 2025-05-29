#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define SIZE 36
#define BOARD_WIDTH 6
#define BOARD_HEIGHT 6
#define SAVE_FILE "game_save.dat"

// Game state
int game_board[SIZE] = {
    1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 12, 14,
    15, 16, 18, 20, 21, 24,
    25, 27, 28, 30, 32, 35,
    36, 40, 42, 45, 48, 49,
    54, 56, 63, 64, 72, 81
};

int player_move_matrix[SIZE] = {0}; // 0=none, 1=player, 2=computer
int player_score = 0, computer_score = 0;
int com_choice = -1;
int game_over = 0;

// Simulated CPU Registers
typedef struct {
    int regA;
    int regB;
    int acc;
} CPU;

CPU cpu; // global simulated CPU

// GTK UI Elements
GtkWidget *window;
GtkWidget *game_grid;
GtkWidget *board_buttons[SIZE];
GtkWidget *number_buttons[9];
GtkWidget *info_label;
GtkWidget *computer_choice_label = NULL;
GtkWidget *score_label = NULL;
GtkWidget *status_label = NULL;
GtkWidget *cpu_state_window = NULL;
GtkWidget *regA_label = NULL;
GtkWidget *regB_label = NULL;
GtkWidget *acc_label = NULL;

// Function declarations
void update_board_ui();
void update_score_label();
void update_status_label(const char *message);
void setup_new_game();
void cpu_state_window_create();
void play_computer_turn(int player_choice);
int check_move(int player_id, int num1, int num2);
int multiply(int *a, int *b);
int getIndex(int idx);
int win_check(int player_id);
void com_move(int player_num);
int evaluate_position(int idx);
int isBoardFull();
int save_game();
int load_game();

// Multiplies two numbers using bitwise operations for simulation
int multiply(int *a, int *b) {
    cpu.regA = *a;
    cpu.regB = *b;
    cpu.acc = 0;

    int x = *a;
    int y = *b;

    while (y > 0) {
        if (y & 1)
            cpu.acc += x;
        x <<= 1;
        y >>= 1;
    }

    return cpu.acc;
}

// Gets the index of a number in the game board
int getIndex(int idx) {
    for (int i = 0; i < SIZE; i++) {
        if (game_board[i] == idx)
            return i;
    }
    return -1;
}

// Check if a move is valid and apply it
int check_move(int player_id, int num1, int num2) {
    int result = multiply(&num1, &num2);
    int idx = getIndex(result);
    if (idx != -1 && player_move_matrix[idx] == 0) {
        player_move_matrix[idx] = player_id;
        return idx;
    }
    return -1;
}

// Check if board is full (draw condition)
int isBoardFull() {
    for (int i = 0; i < SIZE; i++) {
        if (player_move_matrix[i] == 0)
            return 0;
    }
    return 1;
}

// Check for 4-in-a-row win condition
int win_check(int player_id) {
    // Rows
    for (int row = 0; row < BOARD_HEIGHT; row++) {
        for (int col = 0; col <= BOARD_WIDTH - 4; col++) {
            int idx = row * BOARD_WIDTH + col;
            if (player_move_matrix[idx] == player_id &&
                player_move_matrix[idx + 1] == player_id &&
                player_move_matrix[idx + 2] == player_id &&
                player_move_matrix[idx + 3] == player_id)
                return 1;
        }
    }

    // Columns
    for (int col = 0; col < BOARD_WIDTH; col++) {
        for (int row = 0; row <= BOARD_HEIGHT - 4; row++) {
            int idx = row * BOARD_WIDTH + col;
            if (player_move_matrix[idx] == player_id &&
                player_move_matrix[idx+BOARD_WIDTH] == player_id &&
                player_move_matrix[idx+2* BOARD_WIDTH] == player_id &&
                player_move_matrix[idx + 3 * BOARD_WIDTH] == player_id)
                return 1;
        }
    }

    // Diagonals (Top-Left to Bottom-Right)
    for (int row = 0; row <= BOARD_HEIGHT - 4; row++) {
        for (int col = 0; col <= BOARD_WIDTH - 4; col++) {
            int idx = row * BOARD_WIDTH + col;
            if (player_move_matrix[idx] == player_id &&
                player_move_matrix[idx + BOARD_WIDTH + 1] == player_id &&
                player_move_matrix[idx + 2 * (BOARD_WIDTH + 1)] == player_id &&
                player_move_matrix[idx + 3 * (BOARD_WIDTH + 1)] == player_id)
                return 1;
        }
    }

    // Diagonals (Top-Right to Bottom-Left)
    for (int row = 0; row <= BOARD_HEIGHT - 4; row++) {
        for (int col = 3; col < BOARD_WIDTH; col++) {
            int idx = row * BOARD_WIDTH + col;
            if (player_move_matrix[idx] == player_id &&
                player_move_matrix[idx + BOARD_WIDTH - 1] == player_id &&
                player_move_matrix[idx + 2 * (BOARD_WIDTH - 1)] == player_id &&
                player_move_matrix[idx + 3 * (BOARD_WIDTH - 1)] == player_id)
                return 1;
        }
    }

    return 0;
}

// Calculate a position's strategic value (0-10)
int evaluate_position(int idx) {
    int value = 0;
    int row = idx / BOARD_WIDTH;
    int col = idx % BOARD_WIDTH;

    // Prefer center positions
    int center_row = BOARD_HEIGHT / 2;
    int center_col = BOARD_WIDTH / 2;
    int row_distance = abs(row - center_row);
    int col_distance = abs(col - center_col);
    value += 4 - (row_distance + col_distance);

    // Prefer positions that could form a line
    for (int player_id = 1; player_id <= 2; player_id++) {
        int multiplier = (player_id == 2) ? 1 : -1; // Prefer own lines, avoid opponent's

        // Check rows
        int row_count = 0;
        for (int c = 0; c < BOARD_WIDTH; c++) {
            int check_idx = row * BOARD_WIDTH + c;
            if (player_move_matrix[check_idx] == player_id)
                row_count++;
        }
        value += multiplier * row_count;

        // Check columns
        int col_count = 0;
        for (int r = 0; r < BOARD_HEIGHT; r++) {
            int check_idx = r * BOARD_WIDTH + col;
            if (player_move_matrix[check_idx] == player_id)
                col_count++;
        }
        value += multiplier * col_count;

        // Diagonal checks (simplified)
        if ((row == col) || (row + col == BOARD_WIDTH - 1)) {
            value += multiplier * 2; // Diagonals are valuable
        }
    }

    // Add some randomness to prevent predictable play
    value += rand() % 3;

    return value;
}

// Advanced computer move logic
void com_move(int player_num) {
    int comp_num = -1;
    int best_idx = -1;
    int best_value = -1000;

    // 1. First try to find a winning move
    for (int i = 1; i <= 9; i++) {
        int result = multiply(&i, &player_num);
        int idx = getIndex(result);
        if (idx != -1 && player_move_matrix[idx] == 0) {
            // Temporarily make the move
            player_move_matrix[idx] = 2;
            if (win_check(2)) {
                comp_num = i;
                best_idx = idx;
                player_move_matrix[idx] = 0; // Undo temporary move
                break;
            }
            player_move_matrix[idx] = 0; // Undo temporary move
        }
    }

    // 2. If no winning move, try to block player's potential win
    if (comp_num == -1) {
        for (int i = 1; i <= 9; i++) {
            int result = multiply(&i, &player_num);
            int idx = getIndex(result);
            if (idx != -1 && player_move_matrix[idx] == 0) {
                // Check if player would win with this position
                player_move_matrix[idx] = 1;
                if (win_check(1)) {
                    comp_num = i;
                    best_idx = idx;
                    player_move_matrix[idx] = 0; // Undo temporary move
                    break;
                }
                player_move_matrix[idx] = 0; // Undo temporary move
            }
        }
    }

    // 3. Look for 3-in-a-row setups (both offensive and defensive)
    if (comp_num == -1) {
        // Look for possible computer 3-in-a-row
        for (int i = 1; i <= 9; i++) {
            int result = multiply(&i, &player_num);
            int idx = getIndex(result);
            if (idx != -1 && player_move_matrix[idx] == 0) {
                player_move_matrix[idx] = 2;

                // Check rows
                int row = idx / BOARD_WIDTH;
                int count = 0;
                for (int col = 0; col < BOARD_WIDTH; col++) {
                    if (player_move_matrix[row * BOARD_WIDTH + col] == 2)
                        count++;
                }
                if (count >= 3) {
                    comp_num = i;
                    best_idx = idx;
                    player_move_matrix[idx] = 0;
                    break;
                }

                // Check columns
                int col = idx % BOARD_WIDTH;
                count = 0;
                for (int row = 0; row < BOARD_HEIGHT; row++) {
                    if (player_move_matrix[row * BOARD_WIDTH + col] == 2)
                        count++;
                }
                if (count >= 3) {
                    comp_num = i;
                    best_idx = idx;
                    player_move_matrix[idx] = 0;
                    break;
                }

                player_move_matrix[idx] = 0;
            }
        }
    }

    // 4. If still no strategic move found, evaluate all positions
    if (comp_num == -1) {
        for (int i = 1; i <= 9; i++) {
            int result = multiply(&i, &player_num);
            int idx = getIndex(result);
            if (idx != -1 && player_move_matrix[idx] == 0) {
                int value = evaluate_position(idx);
                if (value > best_value) {
                    best_value = value;
                    comp_num = i;
                    best_idx = idx;
                }
            }
        }
    }

    // Apply the computer's move
    if (comp_num != -1) {
        int result = multiply(&comp_num, &player_num);
        int idx = getIndex(result);
        player_move_matrix[idx] = 2;
        com_choice = comp_num;

        // Update UI
        char message[100];
        sprintf(message, "Computer chose: %d → %d × %d = %d", comp_num, comp_num, player_num, result);
        update_status_label(message);

        if (computer_choice_label != NULL) {
            char comp_choice_text[50];
            sprintf(comp_choice_text, "Computer's choice: %d", comp_num);
            gtk_label_set_text(GTK_LABEL(computer_choice_label), comp_choice_text);
        }

        update_board_ui();

        if (win_check(2)) {
            computer_score++;
            update_score_label();
            update_status_label("Computer wins!");
            game_over = 1;
        } else if (isBoardFull()) {
            update_status_label("Game ends in a tie!");
            game_over = 1;
        }
    }
}

// Save game state to file
int save_game() {
    FILE *fp = fopen(SAVE_FILE, "wb");
    if (!fp) {
        update_status_label("Error opening save file!");
        return 0;
    }

    // Save player move matrix
    fwrite(player_move_matrix, sizeof(int), SIZE, fp);

    // Save scores
    fwrite(&player_score, sizeof(int), 1, fp);
    fwrite(&computer_score, sizeof(int), 1, fp);

    // Save computer choice
    fwrite(&com_choice, sizeof(int), 1, fp);

    // Save game state
    fwrite(&game_over, sizeof(int), 1, fp);

    fclose(fp);
    update_status_label("Game saved successfully!");
    return 1;
}

// Load game state from file
int load_game() {
    FILE *fp = fopen(SAVE_FILE, "rb");
    if (!fp) {
        update_status_label("No saved game found.");
        return 0;
    }

    // Load player move matrix
    if (fread(player_move_matrix, sizeof(int), SIZE, fp) != SIZE) {
        update_status_label("Error reading save file!");
        fclose(fp);
        return 0;
    }

    // Load scores
    if (fread(&player_score, sizeof(int), 1, fp) != 1 ||
        fread(&computer_score, sizeof(int), 1, fp) != 1) {
        update_status_label("Error reading save file!");
        fclose(fp);
        return 0;
    }

    // Load computer choice
    if (fread(&com_choice, sizeof(int), 1, fp) != 1) {
        update_status_label("Error reading save file!");
        fclose(fp);
        return 0;
    }

    // Load game state
    if (fread(&game_over, sizeof(int), 1, fp) != 1) {
        update_status_label("Error reading save file!");
        fclose(fp);
        return 0;
    }

    fclose(fp);

    // Update UI
    update_board_ui();
    update_score_label();

    if (computer_choice_label != NULL) {
        char comp_choice_text[50];
        sprintf(comp_choice_text, "Computer's choice: %d", com_choice);
        gtk_label_set_text(GTK_LABEL(computer_choice_label), comp_choice_text);
    }

    update_status_label("Game loaded successfully!");
    return 1;
}

// Update the UI representation of the game board
void update_board_ui() {
    for (int i = 0; i < SIZE; i++) {
        GtkWidget *button = board_buttons[i];

        // Set button label
        char label[10];
        sprintf(label, "%d", game_board[i]);
        gtk_button_set_label(GTK_BUTTON(button), label);

        // Set button color based on player
        GtkStyleContext *context = gtk_widget_get_style_context(button);

        // Remove all previous classes
        gtk_style_context_remove_class(context, "player-cell");
        gtk_style_context_remove_class(context, "computer-cell");
        gtk_style_context_remove_class(context, "empty-cell");

        // Apply appropriate style
        if (player_move_matrix[i] == 1) {
            gtk_style_context_add_class(context, "player-cell");
        } else if (player_move_matrix[i] == 2) {
            gtk_style_context_add_class(context, "computer-cell");
        } else {
            gtk_style_context_add_class(context, "empty-cell");
        }

        // Enable/disable based on game state
        gtk_widget_set_sensitive(button, !game_over);
    }
}

// Update the score display
void update_score_label() {
    if (score_label != NULL) {
        char score_text[100];
        sprintf(score_text, "Player: %d | Computer: %d", player_score, computer_score);
        gtk_label_set_text(GTK_LABEL(score_label), score_text);
    }
}

// Update the status message
void update_status_label(const char *message) {
    if (status_label != NULL) {
        gtk_label_set_text(GTK_LABEL(status_label), message);
    }
}

// Update the CPU register display
void update_cpu_state() {
    if (cpu_state_window && regA_label != NULL && regB_label != NULL && acc_label != NULL) {
        char reg_text[20];

        sprintf(reg_text, "Register A: %d", cpu.regA);
        gtk_label_set_text(GTK_LABEL(regA_label), reg_text);

        sprintf(reg_text, "Register B: %d", cpu.regB);
        gtk_label_set_text(GTK_LABEL(regB_label), reg_text);

        sprintf(reg_text, "Accumulator: %d", cpu.acc);
        gtk_label_set_text(GTK_LABEL(acc_label), reg_text);
    }
}

// Handler for player's number selection
void on_number_clicked(GtkWidget *widget, gpointer data) {
    if (game_over) {
        update_status_label("Game is over. Start a new game.");
        return;
    }

    int player_choice = GPOINTER_TO_INT(data);
    int idx = check_move(1, player_choice, com_choice);

    if (idx != -1) {
        int result = multiply(&player_choice, &com_choice);
        char message[100];
        sprintf(message, "You chose: %d → %d × %d = %d", player_choice, player_choice, com_choice, result);
        update_status_label(message);

        update_board_ui();
        update_cpu_state();

        if (win_check(1)) {
            player_score++;
            update_score_label();
            update_status_label("Congratulations, You win!");
            game_over = 1;
        } else if (isBoardFull()) {
            update_status_label("Game ends in a tie!");
            game_over = 1;
        } else {
            // Computer's turn
            play_computer_turn(player_choice);
        }
    } else {
        int result = multiply(&player_choice, &com_choice);
        char message[100];
        sprintf(message, "Invalid move. %d × %d = %d is not available.",
                player_choice, com_choice, result);
        update_status_label(message);
    }
}

// Computer's turn to play
void play_computer_turn(int player_choice) {
    // Create a "thinking" effect
    update_status_label("Computer is thinking...");

    // Use a timeout to delay the computer's move
    g_timeout_add(1000, (GSourceFunc)com_move, GINT_TO_POINTER(player_choice));
}

// Handler for New Game button
void on_new_game_clicked(GtkWidget *widget, gpointer data) {
    setup_new_game();
}

// Handler for Save Game button
void on_save_game_clicked(GtkWidget *widget, gpointer data) {
    save_game();
}

// Handler for Load Game button
void on_load_game_clicked(GtkWidget *widget, gpointer data) {
    load_game();
}

// Handler for CPU State button
void on_cpu_state_clicked(GtkWidget *widget, gpointer data) {
    if (!cpu_state_window) {
        cpu_state_window_create();
    } else {
        gtk_widget_show_all(cpu_state_window);
    }
    update_cpu_state();
}

// Create the CPU state window
void cpu_state_window_create() {
    // Create window
    cpu_state_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(cpu_state_window), "CPU State");
    gtk_window_set_default_size(GTK_WINDOW(cpu_state_window), 300, 200);
    gtk_window_set_transient_for(GTK_WINDOW(cpu_state_window), GTK_WINDOW(window));
    gtk_window_set_position(GTK_WINDOW(cpu_state_window), GTK_WIN_POS_CENTER_ON_PARENT);

    // Connect destroy signal
    g_signal_connect(cpu_state_window, "destroy", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

    // Create container
    GtkWidget *cpu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(cpu_box), 20);
    gtk_container_add(GTK_CONTAINER(cpu_state_window), cpu_box);

    // Create labels for CPU state
    GtkWidget *title_label = gtk_label_new("CPU Registers");
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(cpu_box), title_label, FALSE, FALSE, 5);

    regA_label = gtk_label_new("Register A: 0");
    gtk_widget_set_halign(regA_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(cpu_box), regA_label, FALSE, FALSE, 5);

    regB_label = gtk_label_new("Register B: 0");
    gtk_widget_set_halign(regB_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(cpu_box), regB_label, FALSE, FALSE, 5);

    acc_label = gtk_label_new("Accumulator: 0");
    gtk_widget_set_halign(acc_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(cpu_box), acc_label, FALSE, FALSE, 5);

    // Close button
    GtkWidget *close_button = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(close_button, "clicked", G_CALLBACK(gtk_widget_hide), cpu_state_window);
    gtk_box_pack_start(GTK_BOX(cpu_box), close_button, FALSE, FALSE, 10);

    gtk_widget_show_all(cpu_state_window);
}

// Setup a new game
void setup_new_game() {
    // Reset game state
    memset(player_move_matrix, 0, sizeof(player_move_matrix));
    game_over = 0;

    // Random computer choice
    com_choice = (rand() % 9) + 1;

    if (computer_choice_label != NULL) {
        char comp_choice_text[50];
        sprintf(comp_choice_text, "Computer's choice: %d", com_choice);
        gtk_label_set_text(GTK_LABEL(computer_choice_label), comp_choice_text);
    }

    // Update UI
    update_board_ui();
    update_status_label("New game started. Choose a number (1-9).");
}

// Handler for rules button
void on_rules_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Game Rules",
                                                   GTK_WINDOW(window),
                                                   GTK_DIALOG_MODAL,
                                                   "Close",
                                                   GTK_RESPONSE_CLOSE,
                                                   NULL);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget *rules_label = gtk_label_new(
        "Multiplication Strategy Game Rules:\n\n"
        "1. The computer chooses a number from 1-9.\n"
        "2. You choose a number from 1-9.\n"
        "3. The product of these numbers is marked on the board.\n"
        "4. Your goal is to get 4 marks in a row, column, or diagonal.\n"
        "5. After your move, the computer plays using your number as its multiplier.\n"
        "6. The game continues until someone gets 4 in a row or the board is full.\n\n"
        "Use the CPU State button to see the simulated CPU registers.\n"
        "Save and load your game progress with the Save/Load buttons."
    );

    gtk_container_set_border_width(GTK_CONTAINER(content_area), 20);
    gtk_container_add(GTK_CONTAINER(content_area), rules_label);

    gtk_widget_show_all(dialog);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Create CSS provider for styling
void setup_css() {
    GtkCssProvider *provider = gtk_css_provider_new();

    const gchar *css_data =
        "button { font-size: 16px; padding: 10px; }"
        ".player-cell { background-color: #4CAF50; font-weight: bold; color: green; }"
        ".computer-cell { background-color: #E53935; font-weight: bold; color: red; }"
        ".empty-cell { background-color: #E0E0E0; }"
        ".number-button { font-size: 18px; font-weight: bold; background-color: #FFF9C4; }"
        ".action-button { background-color: #81D4FA; }"
        ".header-label { font-size: 16px; font-weight: bold; color: #3E2723; }"
        ".status-label { font-style: italic; color: #616161; }";

    GError *error = NULL;
    gtk_css_provider_load_from_data(provider, css_data, -1, &error);

    if (error) {
        g_printerr("CSS load error: %s\n", error->message);
        g_error_free(error);
    }

    GdkScreen *screen = gdk_screen_get_default();
    if (screen) {
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    } else {
        g_printerr("Error: GDK screen not available.\n");
    }

    g_object_unref(provider);
}

// Create and setup window
static void activate(GtkApplication *app, gpointer user_data) {
    setup_css();
    // Create window
    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Multiplication Strategy Game");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);

    // Main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    // Title label
    GtkWidget *title_label = gtk_label_new("Multiplication Strategy Game");
    GtkStyleContext *context = gtk_widget_get_style_context(title_label);
    gtk_style_context_add_class(context, "header-label");
    gtk_box_pack_start(GTK_BOX(main_box), title_label, FALSE, FALSE, 10);

    // Game area (board + controls)
    GtkWidget *game_area = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_box_pack_start(GTK_BOX(main_box), game_area, TRUE, TRUE, 0);

    // Game board grid
    GtkWidget *board_frame = gtk_frame_new("Game Board");
    gtk_box_pack_start(GTK_BOX(game_area), board_frame, TRUE, TRUE, 0);

    GtkWidget *board_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(board_box), 10);
    gtk_container_add(GTK_CONTAINER(board_frame), board_box);

    game_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(game_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(game_grid), 5);
    gtk_widget_set_halign(game_grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(board_box), game_grid, TRUE, TRUE, 0);

    // Create board buttons
    for (int row = 0; row < BOARD_HEIGHT; row++) {
        for (int col = 0; col < BOARD_WIDTH; col++) {
            int idx = row * BOARD_WIDTH + col;
            board_buttons[idx] = gtk_button_new_with_label("");

            // Make buttons non-clickable for board positions
            gtk_widget_set_sensitive(board_buttons[idx], FALSE);

            // Set minimum size
            gtk_widget_set_size_request(board_buttons[idx], 60, 60);

            gtk_grid_attach(GTK_GRID(game_grid), board_buttons[idx], col, row, 1, 1);

            // Add default style
            context = gtk_widget_get_style_context(board_buttons[idx]);
            gtk_style_context_add_class(context, "empty-cell");
        }
    }

    // Controls area
    GtkWidget *controls_frame = gtk_frame_new("Controls");
    gtk_box_pack_start(GTK_BOX(game_area), controls_frame, FALSE, FALSE, 0);

    GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(controls_box), 10);
    gtk_container_add(GTK_CONTAINER(controls_frame), controls_box);

   // Computer's choice label
    computer_choice_label = gtk_label_new("Computer's choice: ");
    gtk_box_pack_start(GTK_BOX(controls_box), computer_choice_label, FALSE, FALSE, 5);

    // Score label
    score_label = gtk_label_new("Player: 0 | Computer: 0");
    context = gtk_widget_get_style_context(score_label);
    gtk_style_context_add_class(context, "header-label");
    gtk_box_pack_start(GTK_BOX(controls_box), score_label, FALSE, FALSE, 5);

    // Number buttons grid
    GtkWidget *numbers_label = gtk_label_new("Choose a number (1-9):");
    gtk_box_pack_start(GTK_BOX(controls_box), numbers_label, FALSE, FALSE, 5);

    GtkWidget *numbers_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(numbers_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(numbers_grid), 5);
    gtk_widget_set_halign(numbers_grid, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(controls_box), numbers_grid, FALSE, FALSE, 5);

    // Create number buttons (1-9)
    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;
        int num = i + 1;

        char label[2];
        sprintf(label, "%d", num);
        number_buttons[i] = gtk_button_new_with_label(label);

        // Set button size
        gtk_widget_set_size_request(number_buttons[i], 50, 50);

        // Add style
        context = gtk_widget_get_style_context(number_buttons[i]);
        gtk_style_context_add_class(context, "number-button");

        // Connect signal
        g_signal_connect(number_buttons[i], "clicked", G_CALLBACK(on_number_clicked), GINT_TO_POINTER(num));

        gtk_grid_attach(GTK_GRID(numbers_grid), number_buttons[i], col, row, 1, 1);
    }

    // Action buttons
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(controls_box), button_box, FALSE, FALSE, 10);

    // New Game button
    GtkWidget *new_game_button = gtk_button_new_with_label("New Game");
    context = gtk_widget_get_style_context(new_game_button);
    gtk_style_context_add_class(context, "action-button");
    g_signal_connect(new_game_button, "clicked", G_CALLBACK(on_new_game_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), new_game_button, FALSE, FALSE, 0);

    // Save Game button
    GtkWidget *save_game_button = gtk_button_new_with_label("Save Game");
    context = gtk_widget_get_style_context(save_game_button);
    gtk_style_context_add_class(context, "action-button");
    g_signal_connect(save_game_button, "clicked", G_CALLBACK(on_save_game_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), save_game_button, FALSE, FALSE, 0);

    // Load Game button
    GtkWidget *load_game_button = gtk_button_new_with_label("Load Game");
    context = gtk_widget_get_style_context(load_game_button);
    gtk_style_context_add_class(context, "action-button");
    g_signal_connect(load_game_button, "clicked", G_CALLBACK(on_load_game_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), load_game_button, FALSE, FALSE, 0);

    // CPU State button
    GtkWidget *cpu_state_button = gtk_button_new_with_label("CPU State");
    context = gtk_widget_get_style_context(cpu_state_button);
    gtk_style_context_add_class(context, "action-button");
    g_signal_connect(cpu_state_button, "clicked", G_CALLBACK(on_cpu_state_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), cpu_state_button, FALSE, FALSE, 0);

    // Rules button
    GtkWidget *rules_button = gtk_button_new_with_label("Rules");
    context = gtk_widget_get_style_context(rules_button);
    gtk_style_context_add_class(context, "action-button");
    g_signal_connect(rules_button, "clicked", G_CALLBACK(on_rules_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), rules_button, FALSE, FALSE, 0);

    // Status label
    status_label = gtk_label_new("Welcome to the Multiplication Strategy Game!");
    context = gtk_widget_get_style_context(status_label);
    gtk_style_context_add_class(context, "status-label");
    gtk_widget_set_margin_top(status_label, 10);
    gtk_box_pack_start(GTK_BOX(main_box), status_label, FALSE, FALSE, 0);

    // Show all widgets
    gtk_widget_show_all(window);

    // Initialize game
    setup_new_game();
}

int main(int argc, char **argv) {
    // Seed random number generator
    srand(time(NULL));

    // Initialize GTK
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.multiplication.game", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
