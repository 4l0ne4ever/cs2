// ui.c - Terminal UI Implementation (Phase 9)

#include "../include/ui.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __unix__
#include <termios.h>
#endif

// Clear screen
void clear_screen()
{
    printf("%s%s", CURSOR_CLEAR, CURSOR_HOME);
    fflush(stdout);
}

// Move cursor to position
void move_cursor(int row, int col)
{
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

// Print colored text
void print_colored(const char *text, const char *color)
{
    if (!text || !color)
        return;
    
    printf("%s%s%s", color, text, COLOR_RESET);
    fflush(stdout);
}

// Print box with title
void print_box(int x, int y, int width, int height, const char *title)
{
    if (width < 3 || height < 3)
        return;
    
    move_cursor(y, x);
    
    // Top border
    printf("┌");
    if (title && strlen(title) > 0)
    {
        int title_len = strlen(title);
        int padding = (width - 2 - title_len) / 2;
        if (padding < 0) padding = 0;
        
        for (int i = 0; i < padding; i++)
            printf("─");
        
        printf(" %s%s%s ", STYLE_BOLD, title, COLOR_RESET);
        
        for (int i = 0; i < width - 2 - title_len - padding - 2; i++)
            printf("─");
    }
    else
    {
        for (int i = 0; i < width - 2; i++)
            printf("─");
    }
    printf("┐\n");
    
    // Middle rows
    for (int i = 1; i < height - 1; i++)
    {
        move_cursor(y + i, x);
        printf("│");
        for (int j = 0; j < width - 2; j++)
            printf(" ");
        printf("│\n");
    }
    
    // Bottom border
    move_cursor(y + height - 1, x);
    printf("└");
    for (int i = 0; i < width - 2; i++)
        printf("─");
    printf("┘\n");
    
    fflush(stdout);
}

// Print progress bar
void print_progress_bar(int x, int y, int width, float progress)
{
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    
    move_cursor(y, x);
    
    int filled = (int)(progress * (width - 2));
    int empty = width - 2 - filled;
    
    printf("[");
    
    // Filled portion
    printf("%s", COLOR_GREEN);
    for (int i = 0; i < filled; i++)
        printf("█");
    printf("%s", COLOR_RESET);
    
    // Empty portion
    for (int i = 0; i < empty; i++)
        printf("░");
    
    printf("] %.1f%%", progress * 100.0f);
    
    fflush(stdout);
}

// Get rarity color (CS2 color scheme)
const char *get_rarity_color(SkinRarity rarity)
{
    switch (rarity)
    {
    case RARITY_CONSUMER:
        return COLOR_BLUE; // Blue
    case RARITY_INDUSTRIAL:
        return COLOR_CYAN; // Light Blue
    case RARITY_MIL_SPEC:
        return COLOR_BLUE; // Blue/Purple
    case RARITY_RESTRICTED:
        return COLOR_MAGENTA; // Purple/Pink
    case RARITY_CLASSIFIED:
        return COLOR_BRIGHT_RED; // Pink/Red
    case RARITY_COVERT:
        return COLOR_YELLOW; // Gold
    case RARITY_CONTRABAND:
        return COLOR_BRIGHT_YELLOW; // Gold (knife/glove)
    default:
        return COLOR_WHITE;
    }
}

// Print skin with rarity color
void print_skin(const Skin *skin, int x, int y)
{
    if (!skin)
        return;
    
    const char *rarity_color = get_rarity_color(skin->rarity);
    const char *stattrak = skin->is_stattrak ? "StatTrak™ " : "";
    const char *wear = wear_to_string(skin->wear);
    
    // Print skin name and details on first line
    move_cursor(y, x);
    printf("%s[%s]%s %s%s%s%s (%s, Pattern #%d)",
           rarity_color, rarity_to_string(skin->rarity), COLOR_RESET,
           skin->is_stattrak ? COLOR_BRIGHT_GREEN : "", stattrak, COLOR_RESET,
           skin->name, wear, skin->pattern_seed);
    
    // Print price at bottom right (assuming box width ~60, price at x+50)
    move_cursor(y + 1, x + 50);
    printf("%s$%.2f%s", COLOR_BRIGHT_GREEN, skin->current_price, COLOR_RESET);
    
    fflush(stdout);
}

// Print menu item with selection highlight
void print_menu_item(const char *text, int selected, int x, int y)
{
    move_cursor(y, x);
    
    if (selected)
    {
        printf("%s%s> %s%s%s", COLOR_BRIGHT_GREEN, STYLE_BOLD, text, COLOR_RESET, STYLE_BOLD);
    }
    else
    {
        printf("  %s", text);
    }
    printf("%s", COLOR_RESET); // Ensure reset at end
    fflush(stdout);
}

// Print header
void print_header(const char *title)
{
    clear_screen();
    
    int width = 80;
    int title_len = strlen(title);
    int padding = (width - title_len) / 2;
    
    printf("%s", STYLE_BOLD);
    for (int i = 0; i < padding; i++)
        printf("═");
    printf(" %s ", title);
    for (int i = 0; i < width - title_len - padding - 2; i++)
        printf("═");
    printf("%s\n\n", COLOR_RESET);
    
    fflush(stdout);
}

// Print separator line
void print_separator(int width)
{
    for (int i = 0; i < width; i++)
        printf("─");
    printf("\n");
    fflush(stdout);
}

// Print centered text
void print_centered(const char *text, int width)
{
    int text_len = strlen(text);
    int padding = (width - text_len) / 2;
    
    for (int i = 0; i < padding; i++)
        printf(" ");
    printf("%s", text);
    for (int i = 0; i < width - text_len - padding; i++)
        printf(" ");
    printf("\n");
    fflush(stdout);
}

// Print error message
void print_error(const char *message)
{
    printf("%s%sERROR: %s%s\n", COLOR_RED, STYLE_BOLD, message, COLOR_RESET);
    fflush(stdout);
}

// Print success message
void print_success(const char *message)
{
    printf("%s%sSUCCESS: %s%s\n", COLOR_GREEN, STYLE_BOLD, message, COLOR_RESET);
    fflush(stdout);
}

// Print info message
void print_info(const char *message)
{
    printf("%s%sINFO: %s%s\n", COLOR_CYAN, STYLE_BOLD, message, COLOR_RESET);
    fflush(stdout);
}

// Wait for key press
void wait_for_key()
{
    printf("\nPress Enter to continue...");
    fflush(stdout);
    getchar();
}

// Get user input with prompt
int get_user_input(char *buffer, size_t buffer_size, const char *prompt)
{
    if (!buffer || buffer_size == 0)
        return -1;
    
    if (prompt)
        printf("%s", prompt);
    fflush(stdout);
    
    if (fgets(buffer, buffer_size, stdin) == NULL)
        return -1;
    
    // Remove newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = '\0';
    
    return 0;
}

// Get password input (hidden with *)
int get_password_input(char *buffer, size_t buffer_size, const char *prompt)
{
    if (!buffer || buffer_size == 0)
        return -1;
    
#ifdef __unix__
    // Use termios to disable echo on Unix systems
    struct termios old_termios, new_termios;
    
    // Get current terminal settings
    if (tcgetattr(STDIN_FILENO, &old_termios) != 0)
        return -1;
    
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
    
    // Set new terminal settings
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) != 0)
        return -1;
#endif
    
    if (prompt)
        printf("%s", prompt);
    fflush(stdout);
    
    // Read password character by character
    size_t i = 0;
    int c;
    while (i < buffer_size - 1 && (c = getchar()) != EOF && c != '\n')
    {
        buffer[i++] = (char)c;
        printf("*");
        fflush(stdout);
    }
    buffer[i] = '\0';
    printf("\n");
    
#ifdef __unix__
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
#endif
    
    return 0;
}

