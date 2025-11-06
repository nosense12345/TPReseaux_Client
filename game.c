#include <stdio.h>
#include <stdlib.h>
#include "game.h"

struct game* create_game(Client* player1, Client* player2, enum mode gameMode)
{
    struct game* newGame = malloc(sizeof(struct game));
    newGame->player1 = player1;
    newGame->player2 = player2;
    newGame->scoreP1 = 0;
    newGame->scoreP2 = 0;
    struct logMove* leLog = malloc(sizeof(struct logMove));
    leLog->nbMoves = 0;
    leLog->head = NULL;
    newGame->logGame = leLog;
    newGame->gameState = IN_PROGRESS;
    newGame->gameMode = gameMode;
    //newGame->gameChat = create_chat();

    return newGame;
}

int delete_the_game(struct game* g)
{
    if (g == NULL) return -1;

    // Free the logMove
    struct listeChaineeMove* current = g->logGame->head;
    while (current != NULL) {
        struct listeChaineeMove* temp = current;
        current = current->next;
        free(temp);
    }
    free(g->logGame);

    //delete_the_chat(g->gameChat);

    free(g);
    return 0;
}

int try_a_move(struct game* g, char moveChar, struct board* b, Client* currentPlayer)
// Return 0 if move played and game continues
// Return 1 if move played and game over
// Return 2 if move not legal because of starvation
// Return 3 if move not legal because hole empty
// Return 4 if move not legal because hole does not belong to player
// Return 5 if invalid move character
// Return 6 if not this player's turn
{
    int move;
    int res_test = 0;
    if ((currentPlayer == g->player1 && b->currentPlayer != PLAYER1) ||
        (currentPlayer == g->player2 && b->currentPlayer != PLAYER2)) {
        res_test = 6; // Not this player's turn
    } else if (moveChar >= 'a' && moveChar <= 'f') {
        move = 11 - (moveChar - 'a'); // Player 2 holes: 6-11
    } else if (moveChar >= 'A' && moveChar <= 'F') {
        move = moveChar - 'A'; // Player 1 holes: 0-6
    } else {
        res_test = 5; // Invalid move character
    }

    if (res_test == 0){
        res_test = test_a_move(move, b);
        if (res_test == 0) {
            res_test = play_a_move(g, move, b);
        }
    }

    return res_test;
}

int test_a_move(int move, struct board* b)
// Return 0 if legal
// Return 2 if move not legal because of starvation
// Return 3 if move not legal because hole empty
// Return 4 if move not legal because hole does not belong to player
{
    struct board* copyBoard = create_copy_board(b);
    int res_play = play_a_move_on_board(copyBoard, move);
    if (res_play == 0)
    {
        res_play = is_board_legal(copyBoard);
    }
    delete_the_board(copyBoard);
    return res_play;
}

int play_a_move(struct game* g, int move, struct board* b)
// Return 0 if move played and game continues
// Return 1 if move played and game over
{
    play_a_move_on_board(b, move);
    g->scoreP1 = b->capturedSeeds[0];
    g->scoreP2 = b->capturedSeeds[1];
    g->logGame->nbMoves += 1;
    struct listeChaineeMove* newMove = malloc(sizeof(struct listeChaineeMove));
    newMove->move = move;
    newMove->next = NULL;
    if (g->logGame->head == NULL) {
        g->logGame->head = newMove;
        g->logGame->tail = newMove;
    } else {
        g->logGame->tail->next = newMove;
        g->logGame->tail = newMove;
    }
    return is_game_over(b);
}

int end_the_game(struct game* g)
{
    g->gameState = FINISHED;
    return 0;
}

int change_the_mode(struct game* g, enum mode newMode)
{
    g->gameMode = newMode;
    return 0;
}
