#ifndef UI_H
#define UI_H

#include "types.h"

// ANSI Colors
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_BRIGHT_RED "\033[91m"
#define COLOR_BRIGHT_GREEN "\033[92m"
#define COLOR_BRIGHT_YELLOW "\033[93m"

// ANSI Styles
#define STYLE_BOLD "\033[1m"
#define STYLE_DIM "\033[2m"
#define STYLE_UNDERLINE "\033[4m"

// Cursor control
#define CURSOR_HOME "\033[H"
#define CURSOR_CLEAR "\033[2J"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"

// UI Functions
void clear_screen();
void move_cursor(int row, int col);
void print_colored(const char *text, const char *color);
void print_box(int x, int y, int width, int height, const char *title);
void print_progress_bar(int x, int y, int width, float progress);

// Rarity colors
const char *get_rarity_color(SkinRarity rarity);

#endif // UI_H
