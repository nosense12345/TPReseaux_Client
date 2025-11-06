#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "server2.h"

enum state
{
    IN_PROGRESS,
    FINISHED
};

enum mode
{
    PRIVATE,
    PUBLIC
};

// Forward declarations if needed
struct chat;
//struct Client;

struct listeChainee
{
    int move;
    struct listeChainee* next;
};

struct log
{
    int nbMoves;
    struct listeChainee* head;
    struct listeChainee* tail;

};

struct game
{
    Client* player1;
    Client* player2;
    int scoreP1;
    int scoreP2;
    struct log* logGame;
    enum state gameState;
    enum mode gameMode;
    struct chat* gameChat;
};

// Fonctions à importer
struct chat* create_chat();
struct chat* delete_the_chat(struct chat* c);

// Function declarations
struct game* create_game(Client* player1, Client* player2, enum mode gameMode);
int delete_the_game(struct game* g);
int try_a_move(struct game* g, int move, struct board* b);
int test_a_move(int move, struct board* b);
int play_a_move(struct game* g, int move, struct board* b);
int end_the_game(struct game* g);
int change_the_mode(struct game* g, enum mode newMode);


#endif /* GAME_H */