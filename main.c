#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>

// define a few useful macros

// width to height ratio
#define WFAC 2

// board loop
#define BOARD_LOOP(i, j, b)  for(i = b; i < board_size; ++i) \
                            for(j = b; j < (board_size * WFAC); ++j)


// enumerator for the cell state
// cell can be either dead or alive
enum {
    Dead = 0, Alive
};

// array to hold game board
char **game_board = NULL;

// initial board size
size_t board_size = 10;

// generation / population variables
unsigned long generation = 0;
long population = 0;

// variables to track program state
useconds_t timing = 100000;
int running = 1;
int pause_game = 1;

// insertion cursor position
int px, py;

// interface window
WINDOW *wui;

// initializes memory for game_board
void create_board(char ***board) {
    int i;
    *board = calloc(board_size + 1, sizeof(char *));
    for (i = 0; i < board_size + 1; ++i)
        (*board)[i] = calloc((board_size * WFAC) + 1, sizeof(char));
}

// frees memory for game_board
void destroy_board(char ***board) {
    int i;
    for (i = 0; i < board_size; ++i) \

            free((*board)[i]);       \
          free(*board);               \
          *board = NULL;
}

// refreshes the board
void refresh_board(void) {
    int i, j, c = 0;

    BOARD_LOOP(i, j, 0) {
            attron(game_board[i][j] ? COLOR_PAIR(2) : COLOR_PAIR(3));

            if (py == i && px == j)
                attron(COLOR_PAIR(1));

            mvaddstr(i, j, "0");

            if (py == i && px == j)
                attroff(COLOR_PAIR(1));

            attroff(game_board[i][j] ? COLOR_PAIR(2) : COLOR_PAIR(3));
        }

    refresh();
}

// resizes the game board
void resize_board(size_t s) {
    int i, j;
    char **next_board = NULL;

    /* Temp board to save current and resize it */
    create_board(&next_board);

    BOARD_LOOP(i, j, 0)next_board[i][j] = game_board[i][j];

    destroy_board(&game_board);

    board_size = s;

    // create the game board

    create_board(&game_board);

    BOARD_LOOP(i, j, 0)game_board[i][j] = next_board[i][j];

    destroy_board(&next_board);

    clear();

    // update cursor position for the resize
    if (px > (board_size * WFAC) - 1)
        px = (int) (board_size * WFAC) - 1;
    if (py > board_size - 1)
        py = (int) board_size - 1;

}

// initialize ncurses window
void init() {
    // placeholder for background color
    short background = COLOR_BLACK;

    /* Init ncurses */
    initscr();
    noecho();
    raw();
    curs_set(FALSE);
    start_color();
    clear();

    if (use_default_colors() == OK)
        background = -1;

    init_pair(0, COLOR_WHITE, COLOR_BLACK);
    init_pair(1, background, COLOR_GREEN);
    init_pair(2, COLOR_BLACK, COLOR_BLACK);
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    init_pair(4, COLOR_GREEN, background);
    init_pair(5, COLOR_RED, background);

    wui = newwin(1, COLS, LINES - 1, 0);
    nodelay(wui, TRUE);
    keypad(wui, TRUE);
    wmove(wui, 0, 0);
    wrefresh(wui);

    refresh();

    // create the board
    create_board(&game_board);

    // set base position of insertion cursor
    px = py = (int) board_size / 2;
}

// Conway's game of life core cell process
void cell_process(void) {
    int i, j, l = 0;
    int a, b, c;
    char **next_board = NULL;

    if (pause_game)
        return;

    create_board(&next_board);

    BOARD_LOOP(i, j, 1) {
            /* Remove selected cell (game_board[i][j]) of count */
            l = -(game_board[i][j]);

            for (a = -1; a < 2; ++a)
                for (b = -1; b < 2; ++b)
                    if (game_board[i + a][j + b])
                        ++l;

            /* 3 cells living around the selected */
            if (l == 3)
                next_board[i][j] = Alive;     /* Born or keep alive */

                /* 2 cells living around */
            else if (l == 2)
                next_board[i][j] = game_board[i][j]; /* Keep same state */

                /* <2 or >3 cells living around */
            else
                next_board[i][j] = Dead;    /* Die */
        }

    // Copy next board into current and count population */
    population = 0;

    BOARD_LOOP(i, j, 0)population += ((game_board[i][j] = next_board[i][j])) ? 1 : 0;

    destroy_board(&next_board);

    ++generation;

    usleep(timing);

}

// polls the keyboard for user input
void key_event(void) {
    int i, c = 0, j;
    if (wui != NULL)
        c = wgetch(wui);
    switch (c) {
        // cursor movement
        case KEY_UP:
        case 'k':
        case 'K':
            if (py > 0)
                --py;
            break;

        case KEY_DOWN:
        case 'j':
        case 'J':
            if (py < board_size - 1)
                ++py;
            break;

        case KEY_LEFT:
        case 'h':
        case 'H':
            if (px > 0)
                --px;
            break;

        case KEY_RIGHT:
        case 'l':
        case 'L':
            if (px < (board_size * WFAC) - 1)
                ++px;
            break;

            // quit
        case 'q':
        case 'Q':
            running = 0;
            break;

            // pause the game
        case 'p':
        case 'P':
            pause_game = !pause_game;
            break;

            // clean the universe
        case 'c':
        case 'C':
            BOARD_LOOP(i, j, 0)game_board[i][j] = 0;

            generation = population = 0;
            pause_game = 1;
            break;

            // toggle cell state
        case ' ':
            game_board[py][px] = !game_board[py][px];
            break;

            // generation timing
        case '+':
            if (timing - 5000)
                timing -= 5000;
            break;

        case '-':
            if (timing < 600000)
                timing += 5000;
            break;

            // page up or page down to reszie the board
        case KEY_PPAGE:
            if (board_size <= 24)
                resize_board((int) board_size + 1);
            break;

        case KEY_NPAGE:
            if (board_size > 4)
                resize_board((int) board_size - 1);
            break;

        case KEY_RESIZE:
            clear();
            break;

        default:
            break;
    }
}


// draws the user interface
void draw_ui(void) {
    int i;

    /* Clean win */

    werase(wui);

    wattron(wui, COLOR_PAIR(0));

//    wmove(wui, 0, 0);

    // game_board state
    wattron(wui, COLOR_PAIR(pause_game ? 5 : 4) | A_BOLD);
    wprintw(wui, pause_game ? "[PAUSED] " : "[RUNNING] ");
    wattroff(wui, COLOR_PAIR(pause_game ? 5 : 4) | A_BOLD);

    // general info
    wprintw(wui, "[Generation: %.6d  Population: %.5d  Timing(+/-): %.6dµs] ", generation, population, timing);

    // insert with info for current position
    wprintw(wui, "[Insert [%.4d x %.4d] (%s)] ", px, py, ((game_board[py][px]) ? "Alive" : "Dead"));

    wattroff(wui, COLOR_PAIR(0));

    wrefresh(wui);

}


int main(int argc, char **argv) {

    int i;

    // initialize game board
    init();

    // draws the initial user interface
    draw_ui();

    // starts the main loop
    while (running) {

        // computes the cell the processes
        cell_process();

        // grabs the keyboard events
        key_event();

        // refreshes the universe
        refresh_board();

        // draws the user interface
        draw_ui();
    }

    // closes the ncurses window
    endwin();

    // frees board memory
    destroy_board(&game_board);

    return 0;
}