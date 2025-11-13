#include "player.h"
#include <stdio.h>
#include <string.h>

#define PLAYERS_FILE "players.dat"

void save_player_data(const Client* client) {
    FILE* file = fopen(PLAYERS_FILE, "r");
    FILE* temp_file = fopen("temp.dat", "w");

    if (file && temp_file) {
        char line[BUF_SIZE];
        int skip = 0;
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "name:", 5) == 0) {
                char name[BUF_SIZE];
                sscanf(line + 5, "%s", name);
                if (strcmp(name, client->name) == 0) {
                    skip = 1;
                }
            }
            if (!skip) {
                fputs(line, temp_file);
            }
            if (skip && strncmp(line, "end", 3) == 0) {
                skip = 0;
            }
        }
        fclose(file);
    }

    fprintf(temp_file, "name:%s\n", client->name);
    for (int i = 0; i < BIO_MAX_LINES; i++) {
        if (strlen(client->bio[i]) > 0) {
            fprintf(temp_file, "bio:%s\n", client->bio[i]);
        }
    }
    for (int i = 0; i < client->num_friends; i++) {
        fprintf(temp_file, "friend:%s\n", client->friends[i]);
    }
    fprintf(temp_file, "end\n");
    fclose(temp_file);

    remove(PLAYERS_FILE);
    rename("temp.dat", PLAYERS_FILE);
}

int load_player_data(Client* client) {
    FILE* file = fopen(PLAYERS_FILE, "r");
    if (file) {
        char line[BUF_SIZE];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "name:", 5) == 0) {
                char name[BUF_SIZE];
                sscanf(line + 5, "%s", name);
                if (strcmp(name, client->name) == 0) {
                    client->num_friends = 0;
                    while (fgets(line, sizeof(line), file) && strncmp(line, "end", 3) != 0) {
                        if (strncmp(line, "bio:", 4) == 0) {
                            for (int i = 0; i < BIO_MAX_LINES; i++) {
                                if (strlen(client->bio[i]) == 0) {
                                    strcpy(client->bio[i], line + 4);
                                    // Remove newline character
                                    client->bio[i][strcspn(client->bio[i], "\n")] = 0;
                                    break;
                                }
                            }
                        } else if (strncmp(line, "friend:", 7) == 0) {
                            strcpy(client->friends[client->num_friends], line + 7);
                            client->friends[client->num_friends][strcspn(client->friends[client->num_friends], "\n")] = 0;
                            client->num_friends++;
                        }
                    }
                    fclose(file);
                    return 1; // Player found
                }
            }
        }
        fclose(file);
    }
    return 0; // Player not found
}
