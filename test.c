#include <stdio.h>
#include <stdlib.h>
#include "game.h"
#include "board.h"
#include "simple_ui.h"

int main()
{   
    Client player1 = { .sock = 23, .name = "Patrick" };
    Client player2 = { .sock = 42, .name = "Spongebob" };
    struct game* g = create_game(&player1, &player2, PUBLIC);
    struct board* b = create_board(g);
    ui_draw_board(convert_board_to_string(b));
    //printf("Hello, World!\n");
    return 0;
}