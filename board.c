#include <stdio.h>
#include <string.h>
#include "board.h"

struct board* create_board(struct game* g)
{
    struct board* newBoard = malloc(sizeof(struct board));
    for (int i = 0; i < 12; i++) {
        newBoard->stateHoles[i] = 4; // Initial number of seeds per hole
    }
    newBoard->currentPlayer = PLAYER1;
    newBoard->capturedSeeds[0] = 0;
    newBoard->capturedSeeds[1] = 0;
    newBoard->gameRef = g;
    return newBoard;
}

struct board* create_copy_board(struct board* b)
{
    if (b == NULL) return NULL;

    struct board* copyBoard = malloc(sizeof(struct board));
    for (int i = 0; i < 12; i++) {
        copyBoard->stateHoles[i] = b->stateHoles[i];
    }
    copyBoard->currentPlayer = b->currentPlayer;
    copyBoard->capturedSeeds[0] = b->capturedSeeds[0];
    copyBoard->capturedSeeds[1] = b->capturedSeeds[1];
    copyBoard->gameRef = b->gameRef;
    return copyBoard;
}

int delete_the_board(struct board* b)
{
    if (b == NULL) return -1;
    free(b);
    return 0;
}

char* convert_board_to_string(struct board* b)
{
    if (b == NULL) return NULL;
    char* listeSpaces[12];
    for (int i = 0; i < 12; i++) {
        listeSpaces[i] = malloc(100 * sizeof(char)); // Allocate memory for each string
        strcpy(listeSpaces[i], ""); // Initialize with empty string
    }

    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < b->stateHoles[i]; j++) {
            strcat(listeSpaces[i], " ");
        }
    }

    char* boardString = malloc(2048 * sizeof(char));
    snprintf(boardString, 2048,
        "Score %s: %d | Score %s: %d\n"
        "Current Player: %s\n"
        "Board:\n"
        "a%s| b%s| c%s| d%s| e%s| f\n"
        "%d | %d | %d | %d | %d | %d\n"
        "-------------------\n"
        "%d | %d | %d | %d | %d | %d\n"
        "A%s| B%s| C%s| D%s| E%s| F\n",
        b->gameRef->player1->name, b->capturedSeeds[0],
        b->gameRef->player2->name, b->capturedSeeds[1],
        (b->currentPlayer == PLAYER1) ? b->gameRef->player1->name : b->gameRef->player2->name,
        listeSpaces[11], listeSpaces[10], listeSpaces[9], listeSpaces[8], listeSpaces[7],
        b->stateHoles[11], b->stateHoles[10], b->stateHoles[9], b->stateHoles[8], b->stateHoles[7], b->stateHoles[6],
        b->stateHoles[0], b->stateHoles[1], b->stateHoles[2], b->stateHoles[3], b->stateHoles[4], b->stateHoles[5],
        listeSpaces[0], listeSpaces[1], listeSpaces[2], listeSpaces[3], listeSpaces[4]
    );

    for (int i = 0; i < 12; i++) {
        free(listeSpaces[i]); // Free the allocated memory
    }

    return boardString;
}
