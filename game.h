#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "server2.h"

enum state
//Game state that coulb be used in the future if we implement save/load features and history tracking
{
    IN_PROGRESS,
    FINISHED
};

enum mode
//Game mode that could be used in the future for different game types (for example to make games visible only to friends)
{
    PRIVATE,
    PUBLIC
};

// Forward declarations if needed
struct chat;
//struct Client;

struct listeChaineeMove
{
    int move;
    struct listeChaineeMove* next;
};

struct logMove
{
    int nbMoves;
    struct listeChaineeMove* head;
    struct listeChaineeMove* tail;

};

struct listeChaineeSpectateurs
{
    Client* spectator;
    struct listeChaineeSpectateurs* next;
};

struct listeSpectateurs
{
    int nbSpectators;
    struct listeChaineeSpectateurs* head;
    struct listeChaineeSpectateurs* tail;

};

struct game
{
    Client* player1;
    Client* player2;
    int scoreP1;
    int scoreP2;
    struct logMove* logGame;
    struct listeSpectateurs* spectators;
    enum state gameState;
    enum mode gameMode;
    struct chat* gameChat;
};

// Fonctions à importer
struct chat* create_chat();
struct chat* delete_the_chat(struct chat* c);

// Function declarations
struct board;

struct game* create_game(Client* player1, Client* player2, enum mode gameMode);
int delete_the_game(struct game* g);
int try_a_move(struct game* g, char moveChar, struct board* b, Client* currentPlayer);
int test_a_move(int move, struct board* b);
int play_a_move(struct game* g, int move, struct board* b);
int end_the_game(struct game* g);
int change_the_mode(struct game* g, enum mode newMode);
int add_spectator_to_game(struct game* g, Client* spectator);
int remove_spectator_from_game(struct game* g, Client* spectator);


#endif /* GAME_H */