#include <stdio.h>
#include <stdlib.h>
#include "game.h"

struct game* create_game(struct client* player1, struct client* player2, enum mode gameMode)
{
    struct game* newGame = malloc(sizeof(struct game));
    newGame->player1 = player1;
    newGame->player2 = player2;
    newGame->scoreP1 = 0;
    newGame->scoreP2 = 0;
    struct log* leLog = malloc(sizeof(struct log));
    leLog->nbMoves = 0;
    leLog->head = NULL;
    newGame->logGame = leLog;
    newGame->gameState = IN_PROGRESS;
    newGame->gameMode = gameMode;
    newGame->gameChat = create_chat();

    return newGame;
}



int main()
{
    printf("Hello, World!\nFrom game.c\n");
    return 0;
}