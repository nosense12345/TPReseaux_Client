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
    newBoard->logHistory = malloc(sizeof(struct logBoard));
    newBoard->logHistory->nbMoves = 0;
    newBoard->logHistory->head = NULL;
    newBoard->logHistory->tail = NULL;
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

    // Allocate and initialize logHistory
    copyBoard->logHistory = malloc(sizeof(struct logBoard));
    if (copyBoard->logHistory == NULL) {
        free(copyBoard);
        return NULL;
    }
    
    copyBoard->logHistory->nbMoves = b->logHistory->nbMoves;
    copyBoard->logHistory->head = NULL;
    copyBoard->logHistory->tail = NULL;

    struct listeChaineeBoard* currentSrc = b->logHistory->head;
    for (int i=0; i < b->logHistory->nbMoves; i++) {
        struct listeChaineeBoard* current = malloc(sizeof(struct listeChaineeBoard));
        current->next = NULL;
        current->currentPlayer = currentSrc->currentPlayer;
        for (int j = 0; j < 12; j++) {
            current->stateHoles[j] = currentSrc->stateHoles[j];
        }
        if (i == 0) {
            copyBoard->logHistory->head = current;
            copyBoard->logHistory->tail = current;
        } else {
            copyBoard->logHistory->tail->next = current;
            copyBoard->logHistory->tail = current;
        }
        currentSrc = currentSrc->next;
        
    }
    return copyBoard;
}

int delete_the_board(struct board* b)
{
    if (b == NULL) return -1;
    // Free the log history
    struct listeChaineeBoard* current = b->logHistory->head;
    while (current != NULL) {
        struct listeChaineeBoard* temp = current;
        current = current->next;
        free(temp);
    }
    free(b->logHistory);
    free(b);
    return 0;
}

char* convert_board_to_string(struct board* b)
// Returns a string representation of the board
// Example:
// Score Patrick: 0 | Score Spongebob: 0
// Current Player: Patrick
// Board:
// a | b | c | d | e | f
// 4 | 4 | 4 | 4 | 4 | 4
// -------------------
// 4 | 4 | 4 | 4 | 4 | 4
// A | B | C | D | E | F
{
    if (b == NULL) return NULL;
    char* listeSpaces[12];
    for (int i = 0; i < 12; i++) {
        listeSpaces[i] = malloc(100 * sizeof(char)); // Allocate memory for each string
        strcpy(listeSpaces[i], ""); // Initialize with empty string
    }

    for (int i = 0; i < 12; i++) {
        for (int j = 0; j <= b->stateHoles[i]; j+=10) {
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

int is_game_over(struct board* b)
// Return 1 if game over, 0 otherwise
// Return -1 if board is NULL
{
    if (b == NULL) return -1;

    int res = 0;

    if (b->capturedSeeds[0] >= 25 || b->capturedSeeds[1] >= 25 || (b->capturedSeeds[0] + b->capturedSeeds[1] == 48)) {
        res = 1;
    } else
    {
        int side1_empty = 1;
        for (int i = 0; i < 6 && side1_empty == 1; i++) {
            if (b->stateHoles[i] > 0) {
                side1_empty = 0;
            }
        }

        int side2_empty = 1;
        for (int i = 6; i < 12 && side2_empty == 1; i++) {
            if (b->stateHoles[i] > 0) {
                side2_empty = 0;
            }
        }

        if (side1_empty) {
            res = 1;
            for (int i = 6; i < 12 && res == 1; i++) {
                if (test_a_move(i, b) == 0) {
                    res = 0;
                }
            }
            if (res == 1) {
                for (int i = 6; i < 12; i++) {
                    b->capturedSeeds[1] += b->stateHoles[i];
                    b->stateHoles[i] = 0;
                }
            }
        } else if (side2_empty) {
            res = 1;
            for (int i = 0; i < 6 && res == 1; i++) {
                if (test_a_move(i, b) == 0) {
                    res = 0;
                }
            }
            if (res == 1) {
                for (int i = 0; i < 6; i++) {
                    b->capturedSeeds[0] += b->stateHoles[i];
                    b->stateHoles[i] = 0;
                }
            }
        }

        struct listeChaineeBoard* current = b->logHistory->head;
        while (res == 0 && current != NULL)
        {
            struct listeChaineeBoard* temp = current;
            current = current->next;
            if (memcmp(temp->stateHoles, b->stateHoles, 12 * sizeof(int)) == 0 &&
                temp->currentPlayer == b->currentPlayer) {
                res = 1;
            }
        }
    }

    return res;
}

int play_a_move_on_board(struct board* b, int move)
// Return 0 if move legal (and playes the move)
// Return 3 if move not legal because hole empty
// Return 4 if move not legal because hole does not belong to player
// Return -1 if board is NULL
{
    if (b == NULL) return -1;

    if (((b->currentPlayer == PLAYER1) && (move > 5)) ||
        ((b->currentPlayer == PLAYER2) && (move < 6))) {
        return 4; // Hole does not belong to player
    }

    if (b->stateHoles[move] == 0) return 3; // Hole empty

    b->logHistory->nbMoves++;
    struct listeChaineeBoard* newLogEntry = malloc(sizeof(struct listeChaineeBoard));
    for (int i = 0; i < 12; i++) {
        newLogEntry->stateHoles[i] = b->stateHoles[i];
    }
    newLogEntry->currentPlayer = b->currentPlayer;
    newLogEntry->next = NULL;
    if (b->logHistory->head == NULL) {
        b->logHistory->head = newLogEntry;
        b->logHistory->tail = newLogEntry;
    } else {
        b->logHistory->tail->next = newLogEntry;
        b->logHistory->tail = newLogEntry;
    }

    int seedsToSow = b->stateHoles[move];
    b->stateHoles[move] = 0;

    int index = move;
    while (seedsToSow > 0) {
        index = (index + 1) % 12;
        if (index != move)
        {
            b->stateHoles[index]++;
            seedsToSow--;
        }
    }

    struct board* copyBoard = create_copy_board(b);
    while ((copyBoard->stateHoles[index] == 2 || 
            copyBoard->stateHoles[index] == 3) &&
           ((copyBoard->currentPlayer == PLAYER1 && index >=6) ||
           (copyBoard->currentPlayer == PLAYER2 && index <6))) {
        if (copyBoard->currentPlayer == PLAYER1) {
            copyBoard->capturedSeeds[0] += copyBoard->stateHoles[index];
        } else {
            copyBoard->capturedSeeds[1] += copyBoard->stateHoles[index];
        }
        copyBoard->stateHoles[index] = 0;
        index = (index + 11) % 12;
    }
    
    int side1_empty = 0;
    int side2_empty = 0;
    if (copyBoard->currentPlayer == PLAYER1) {
        side2_empty = 1;
        for (int i = 6; i < 12 && side2_empty == 1; i++) {
            if (copyBoard->stateHoles[i] > 0) {
                side2_empty = 0;
            }
        }
    } else {
        side1_empty = 1;
        for (int i = 0; i < 6 && side1_empty == 1; i++) {
            if (copyBoard->stateHoles[i] > 0) {
                side1_empty = 0;
            }
        }
    }
    if (side1_empty || side2_empty) {
        delete_the_board(copyBoard);
    } else {
        b->capturedSeeds[0] = copyBoard->capturedSeeds[0];
        b->capturedSeeds[1] = copyBoard->capturedSeeds[1];
        for (int i = 0; i < 12; i++) {
            b->stateHoles[i] = copyBoard->stateHoles[i];
        }
        delete_the_board(copyBoard);
    }

    // Change current player
    b->currentPlayer = (b->currentPlayer == PLAYER1) ? PLAYER2 : PLAYER1;

    

    return 0;
}

int is_board_legal(struct board* b)
// Return 0 if legal
// Return 2 if move not legal because of starvation
// Return -1 if board is NULL
{
    if (b == NULL) return -1;

    int side1_empty = 0;
    int side2_empty = 0;
    if (b->currentPlayer == PLAYER1) {
        side1_empty = 1;
        for (int i = 0; i < 6 && side1_empty == 1; i++) {
            if (b->stateHoles[i] > 0) {
                side1_empty = 0;
            }
        }
    } else {
        side2_empty = 1;
        for (int i = 6; i < 12 && side2_empty == 1; i++) {
            if (b->stateHoles[i] > 0) {
                side2_empty = 0;
            }
        }
    }
    if (side1_empty || side2_empty) {
        return 2; // Not legal because of starvation
    }

    return 0; // Legal
}
