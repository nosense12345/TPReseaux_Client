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
    ui_draw_board(convert_board_to_string(b), STATE_LOBBY);
    int turn = 0;
    while(1) {
        char input[100];
        ui_get_input(input, sizeof(input));
        if (input[0] == 'q') {
            break;
        } else if (input[0] == 'p') {
            printf("Current board state:\n%s\n", convert_board_to_string(b));
        } else{
            int res;
            if (turn == 0) {
                ui_add_message("Patrick's turn", STATE_INGAME);
                res = try_a_move(g, input[0], b, &player1);
                turn = 1;
            } else {
                ui_add_message("Spongebob's turn", STATE_INGAME);
                res = try_a_move(g, input[0], b, &player2);
                turn = 0;
            }
            ui_draw_board(convert_board_to_string(b), STATE_INGAME);
            ui_add_message("Return code:", STATE_INGAME);
            char err_msg[50];
            snprintf(err_msg, sizeof(err_msg), "%d", res);
            ui_add_message(err_msg, STATE_INGAME);
        }
    }

    //printf("Hello, World!\n");
    return 0;
}