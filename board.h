#ifndef BOARD_H
#define BOARD_H

#include <stdlib.h>
#include "game.h"
#include "server2.h"

enum joueur
{
    PLAYER1,
    PLAYER2
};

struct board
{
    int stateHoles[12];
    enum joueur currentPlayer;
    int capturedSeeds[2];
    struct game* gameRef;
};

// Function declarations
struct board* create_board(struct game* g);
struct board* create_copy_board(struct board* b);
int delete_the_board(struct board* b);
char* convert_board_to_string(struct board* b);
int is_game_over(struct board* b);
int play_a_move_on_board(struct board* b, int move);
int is_board_legal(struct board* b);

#endif /* BOARD_H */