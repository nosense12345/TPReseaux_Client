#ifndef PLAYER_H
#define PLAYER_H

#include "server2.h"

void save_player_data(const Client* client);
int load_player_data(Client* client);

#endif // PLAYER_H
