#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "game.h"
#include "board.h"
#include "player.h"
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>


void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

void server_app(void)
{
   SOCKET sock = server_init_connection();
   char buffer[BUF_SIZE];
   char msg[BUF_SIZE * 2];
   char challenge_msg[BUF_SIZE * 2];
   char bio_response[BUF_SIZE * 2];
   char final_msg[BUF_SIZE * 2];
   char acceptance_msg[BUF_SIZE * 2];
   char confirmation_msg[BUF_SIZE * 2];
   char refusal_msg[BUF_SIZE * 2];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()") ;
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         socklen_t sinsize = sizeof(csin);
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()") ;
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* Check for registration command */
         char command[BUF_SIZE];
         char username[BUF_SIZE];
         if (sscanf(buffer, "%s %s", command, username) == 2 && strcmp(command, "REGISTER") == 0)
         {
            Client c = { csock, "", {{0}}, {{0}}, 0, STATE_LOBBY, -1, -1, -1, NULL };
            strncpy(c.name, username, BUF_SIZE - 1);

            if (load_player_data(&c)) {
               write_client(csock, "Welcome back!");
            } else {
               for (int j = 0; j < BIO_MAX_LINES; j++) {
                  c.bio[j][0] = '\0';
               }
               for (int j = 0; j < MAX_FRIENDS; j++) {
                  c.friends[j][0] = '\0';
               }
               c.state = STATE_LOBBY;
               c.opponent = -1;
               c.challenging_who = -1;
               c.num_friends = 0;
               c.challenge_mode = -1;
               save_player_data(&c);
               write_client(csock, "Welcome! You are registered.");
            }

            /* what is the new maximum fd ? */
            max = csock > max ? csock : max;
            FD_SET(csock, &rdfs);

            clients[actual] = c;
            actual++;
         }
         else
         {
            write_client(csock, "Invalid command. Please register with 'REGISTER <username>'\n");
            closesocket(csock);
         }
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               int c = read_client(clients[i].sock, buffer);

               /* client disconnected */
               if(c == 0)
               {
                  handle_client_disconnection(clients, i, &actual);
               }

               /* This if else checks the state of the client to know what imputs to expect */
               if (clients[i].state == STATE_LOBBY) {
               // Handle commands in the lobby state

                  
                  if (strcmp(buffer, "/list") == 0) {
                  // List command : to see online users

                     char user_list[BUF_SIZE] = "Online users:\n";
                     for (int j = 0; j < actual; j++) {
                        strncat(user_list, clients[j].name, BUF_SIZE - strlen(user_list) - 1);
                        strncat(user_list, "\n", BUF_SIZE - strlen(user_list) - 1);
                     }
                     write_client(clients[i].sock, user_list);

                  } else if (strcmp(buffer, "/quit") == 0) {
                  // Quit command : to disconnect from the server

                     handle_client_disconnection(clients, i, &actual);

                  } else if (strncmp(buffer, "/challenge", 10) == 0) {
                  // Challenge command : to challenge another player

                     char challenged_name[BUF_SIZE];
                     char mode[BUF_SIZE];
                     
                     //Checkes if the command is well formatted
                     if (sscanf(buffer, "/challenge %s %s", challenged_name, mode) == 2) {
                        int challenger_idx = i;
                        int challenged_idx = -1;
                        // Search for the challenged player
                        for(int j=0; j<actual; j++) {
                           if(strcmp(clients[j].name, challenged_name) == 0) {
                              challenged_idx = j;
                              break;
                           }
                        }

                        if(challenged_idx != -1 && (strcmp(mode, "PUBLIC") == 0 || strcmp(mode, "PRIVATE") == 0)) {
                        // If the challenged player is found and the mode is correct

                           if (challenged_idx == challenger_idx) {
                              write_client(clients[i].sock, "You can't challenge yourself.");
                           } else if (clients[challenged_idx].state != STATE_LOBBY) {
                              write_client(clients[i].sock, "This player is not available for a challenge.");
                           } else {
                              clients[challenged_idx].state = STATE_CHALLENGED;
                              clients[challenger_idx].state = STATE_CHALLENGING;
                              clients[challenger_idx].challenging_who = challenged_idx;
                              clients[challenger_idx].challenge_mode = (strcmp(mode, "PUBLIC") == 0) ? 0 : 1; // 0 for PUBLIC, 1 for PRIVATE

                              // Notify the challenged player
                              snprintf(challenge_msg, sizeof(challenge_msg) - strlen(challenge_msg), "%s_CHALLENGE_FROM %s\nPlease answer his challenge.", mode, clients[challenger_idx].name);
                              write_client(clients[challenged_idx].sock, challenge_msg);

                              // Change states of both players in their clients
                              char msg[BUF_SIZE];
                              snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_CHALLENGED);
                              write_client(clients[challenged_idx].sock, msg);

                              snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_CHALLENGING);
                              write_client(clients[challenger_idx].sock, msg);

                              // Puts the challenger's CHAT in challenging state and notifies him
                              write_client(clients[challenger_idx].sock, "CLEAR_CHAT");
                              snprintf(msg, sizeof(msg) - strlen(msg), "You have challenged %s in mode %s.", clients[challenged_idx].name, mode);
                              write_client(clients[challenger_idx].sock, msg);
                              
                           }
                        } else if (challenged_idx == -1) {
                           write_client(clients[i].sock, "Player not found");
                        } else {
                           write_client(clients[i].sock, "Usage: /challenge <username> [PUBLIC|PRIVATE]");
                        }
                     } else {
                        write_client(clients[i].sock, "Usage: /challenge <username> [PUBLIC|PRIVATE]");
                     }

                  } else if (strcmp(buffer, "/bio") == 0) {
                  // Bio command : to edit/view your bio

                     clients[i].state = STATE_BIO;
                     char msg[BUF_SIZE];
                     // Clear chat, updates state and show current bio
                     write_client(clients[i].sock, "CLEAR_CHAT");
                     snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_BIO);
                     write_client(clients[i].sock, msg);
                     snprintf(bio_response, sizeof(bio_response), "Your bio:\n");
                     for (int j = 0; j < BIO_MAX_LINES; j++) {
                        if (clients[i].bio[j][0] != '\0') {
                           strncat(bio_response, clients[i].bio[j], BUF_SIZE - strlen(bio_response) - 1);
                           strncat(bio_response, "\n", BUF_SIZE - strlen(bio_response) - 1);
                        }
                     }
                     if (strlen(bio_response) == (10)) { // only the header
                        write_client(clients[i].sock, "You dont have a bio yet. Type your bio:");
                     } else {
                        write_client(clients[i].sock, bio_response);
                     }

                  } else if (strncmp(buffer, "/viewbio", 8) == 0) {
                  // Viewbio command : to view your bio or another player's bio

                     // Searches for the username in the command
                     char username[BUF_SIZE];
                     if (sscanf(buffer, "/viewbio %s", username) == 1) {
                        int user_idx = -1;
                        for (int j = 0; j < actual; j++) {
                           if (strcmp(clients[j].name, username) == 0) {
                              user_idx = j;
                              break;
                           }
                        }

                        // If the user is found, display their bio
                        if (user_idx != -1) {
                           snprintf(bio_response, sizeof(bio_response), "Bio for user %s:\n", clients[user_idx].name);
                           for (int j = 0; j < BIO_MAX_LINES; j++) {
                              if (clients[user_idx].bio[j][0] != '\0') {
                                 strncat(bio_response, clients[user_idx].bio[j], BUF_SIZE - strlen(bio_response) - 1);
                                 strncat(bio_response, "\n", BUF_SIZE - strlen(bio_response) - 1);
                              }
                           }
                           if (strlen(bio_response) == (strlen(clients[user_idx].name) + 15)) {
                           // If only the header is found the user has no bio, so we inform the requester without clearing the chat
                              write_client(clients[i].sock, "This user has no bio.");
                           } else {
                           // Otherwise we clear the chat and show the bio
                              write_client(clients[i].sock, "CLEAR_CHAT");
                              write_client(clients[i].sock, bio_response);
                           }

                        } else {
                           write_client(clients[i].sock, "User not found.");
                        }

                     } else {
                        write_client(clients[i].sock, "Usage: /viewbio <username>");
                     }

                  } else if (strcmp(buffer, "/friends") == 0) {
                  // Friends command : to see your friends list

                     char friend_list[BUF_SIZE] = "Your friends:\n";
                     int friend_count = 0;
                     for (int j = 0; j < clients[i].num_friends; j++) {
                        strncat(friend_list, clients[i].friends[j], BUF_SIZE - strlen(friend_list) - 1);
                        strncat(friend_list, "\n", BUF_SIZE - strlen(friend_list) - 1);
                        friend_count++;
                     }

                     if (friend_count == 0) {
                        write_client(clients[i].sock, "You have no friends.");
                     } else {
                        write_client(clients[i].sock, "CLEAR_CHAT");
                        write_client(clients[i].sock, friend_list);
                     }

                  } else if (strncmp(buffer, "/removefriend", 13) == 0) {
                  // Removefriend command : to remove a player from your friends list

                     // Searches for the friend's username in the command
                     char friend_name[BUF_SIZE];
                     if (sscanf(buffer, "/removefriend %s", friend_name) == 1) {
                        int friend_idx = -1;
                        for (int j = 0; j < clients[i].num_friends; j++) {
                           if (strcmp(clients[i].friends[j], friend_name) == 0) {
                              friend_idx = j;
                              break;
                           }
                        }

                        if (friend_idx != -1) {
                        // If the friend is found, remove them from the list
                           for (int j = friend_idx; j < clients[i].num_friends - 1; j++) {
                              strcpy(clients[i].friends[j], clients[i].friends[j + 1]);
                           }
                           clients[i].num_friends--;
                           write_client(clients[i].sock, "Friend removed.");
                        } else {
                           write_client(clients[i].sock, "User not found in your friend list.");
                        }

                     } else {
                        write_client(clients[i].sock, "Usage: /removefriend <username>");
                     }

                  } else if (strncmp(buffer, "/addfriend", 10) == 0) {
                  // Addfriend command : to add a player to your friends list

                     // Searches for the friend's username in the command
                     char friend_name[BUF_SIZE];
                     if (sscanf(buffer, "/addfriend %s", friend_name) == 1) {
                        int friend_idx = -1;
                        for (int j = 0; j < actual; j++) {
                           if (strcmp(clients[j].name, friend_name) == 0) {
                              friend_idx = j;
                              break;
                           }
                        }

                        if (friend_idx != -1) {
                           if (friend_idx == i) {
                              write_client(clients[i].sock, "You can't add yourself as a friend.");
                           } else {
                              /* Check if already friends */
                              int already_friends = 0;
                              for (int j = 0; j < clients[i].num_friends; j++) {
                                 if (strcmp(clients[i].friends[j], friend_name) == 0) {
                                    already_friends = 1;
                                    break;
                                 }
                              }

                              if (already_friends) {
                                 write_client(clients[i].sock, "Player already in the list of friends");
                              } else {
                                 if (clients[i].num_friends < MAX_FRIENDS) {
                                    strcpy(clients[i].friends[clients[i].num_friends], friend_name);
                                    clients[i].num_friends++;
                                    write_client(clients[i].sock, "Friend added.");
                                 } else {
                                    write_client(clients[i].sock, "Your friend list is full.");
                                 }

                              }

                           }

                        } else {
                           write_client(clients[i].sock, "User not found.");
                        }

                     } else {
                        write_client(clients[i].sock, "Usage: /addfriend <username>");
                     }

                  } else if (strncmp(buffer, "/chat", 5) == 0) {
                  // Chat command : to send a private message to another player

                     char *recipient_name = strtok(buffer + 6, " ");
                     char *msg = strtok(NULL, "");
                     if (recipient_name && msg && strlen(msg) > 0) {
                        int recipient_idx = -1;
                        for (int j = 0; j < actual; j++) {
                        // Search for the recipient player
                           if (strcmp(clients[j].name, recipient_name) == 0) {
                              recipient_idx = j;
                              break;
                           }

                        }

                        if (recipient_idx != -1) {
                           if (recipient_idx == i) {
                              write_client(clients[i].sock, "You can't chat with yourself.");
                           } else {
                              snprintf(final_msg, sizeof(final_msg), "(private) %s: %s", clients[i].name, msg);
                              write_client(clients[recipient_idx].sock, final_msg);
                              snprintf(final_msg, sizeof(final_msg), "(private) From you to %s: %s", clients[recipient_idx].name, msg);
                              write_client(clients[i].sock, final_msg);
                           }

                        } else {
                           write_client(clients[i].sock, "Player not found");
                        }

                     } else {
                        write_client(clients[i].sock, "Usage: /chat <username> <message>");
                     }

                  } else if (strncmp(buffer, "/spectate", 5) == 0) {
                  // Spectate command : to spectate another player's game

                     char spectated_name[BUF_SIZE];

                     if (sscanf(buffer, "/spectate %s", spectated_name) == 1) {
                     // Checks for the username in the command
                        int spectated_idx = -1;
                        for(int j=0; j<actual; j++) {
                           if(strcmp(clients[j].name, spectated_name) == 0) {
                              spectated_idx = j;
                              break;
                           }
                        }

                        if (spectated_idx != -1) {

                           // Check if the spectated player is a friend
                           int friend = 0;
                           for(int j=0; j<clients[spectated_idx].num_friends; j++) {
                              if(strcmp(clients[spectated_idx].friends[j], clients[i].name) == 0) {
                                 friend = 1;
                                 break;
                              }
                           }

                           if (clients[spectated_idx].state != STATE_INGAME) {

                              write_client(clients[i].sock, "This player is not currently in a game.");

                           } else if (clients[spectated_idx].Currentboard->gameRef->gameMode == PUBLIC || friend) {
                           // If the game is public or the requester is a friend, allow spectating

                              // Notify that the client is now spectating
                              clients[i].state = STATE_SPECTATE;
                              struct game* g = clients[spectated_idx].Currentboard->gameRef;
                           
                              write_client(clients[i].sock, "CLEAR_CHAT");
                              char msg[BUF_SIZE];
                              snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_SPECTATE);
                              write_client(clients[i].sock, msg);

                              snprintf(msg, BUF_SIZE, "CHANGE_BOARD%s", convert_board_to_string(clients[spectated_idx].Currentboard));
                              write_client(clients[i].sock, msg);

                              snprintf(msg, sizeof(msg) - strlen(msg), "You are now spectating %s.", clients[spectated_idx].name);
                              write_client(clients[i].sock, msg);

                              clients[i].Currentboard = clients[spectated_idx].Currentboard;

                              /* Notify the players */
                              snprintf(msg, sizeof(msg) - strlen(msg), "%s is now spectating your game.", clients[i].name);
                              write_client(g->player1->sock, msg);
                              write_client(g->player2->sock, msg);

                              /* Notify other spectators */
                              snprintf(msg, sizeof(msg) - strlen(msg), "%s is now spectating the game.", clients[i].name);
                              struct listeChaineeSpectateurs* current = g->spectators->head;
                              while (current != NULL) {
                                 write_client(current->spectator->sock, msg);
                                 current = current->next;
                              }

                              /* Add new spectator to spectator list */
                              add_spectator_to_game(g, &clients[i]);
                           } else {
                              write_client(clients[i].sock, "You can't spectate this player's game because you are not on their friends list and the game is private.");
                           }

                        } else {
                           write_client(clients[i].sock, "Player not found");
                        }

                     }

                  }else if (buffer[0] != '/') {
                  // General message to all players in the lobby

                     snprintf(final_msg, sizeof(final_msg) - strlen(final_msg), "%s: %s", clients[i].name, buffer);
                     for (int j = 0; j < actual; j++) {
                        if (clients[j].state == STATE_LOBBY) {
                           write_client(clients[j].sock, final_msg);
                        }

                     }

                  } else {
                     write_client(clients[i].sock, "Your command was not recognised.\nYou are in the lobby. The available commands are /list, /bio, /viewbio [user], /challenge [user], /addfriend [user], /removefriend [user], /friends, /chat [user] [message], /spectate [user], /clearchat, /quit");
                  }

               } else if (clients[i].state == STATE_CHALLENGING) {
               // Handle commands in the challenging state

                  if (strcmp(buffer, "/cancelchallenge") == 0) {
                  // Cancelchallenge command : to cancel a challenge sent to another player

                     int challenged_idx = clients[i].challenging_who;
                     if (challenged_idx != -1) {
                        clients[challenged_idx].state = STATE_LOBBY;
                        clients[i].state = STATE_LOBBY;
                        clients[i].challenging_who = -1;

                        snprintf(msg, sizeof(msg), "The challenge from %s has been canceled.", clients[i].name);
                        write_client(clients[challenged_idx].sock, msg);
                        snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
                        write_client(clients[challenged_idx].sock, msg);

                        write_client(clients[i].sock, "You have canceled the challenge.");
                        snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
                        write_client(clients[i].sock, msg);
                     } else {
                        write_client(clients[i].sock, "You are not challenging anyone.");
                     }

                  } else {
                     write_client(clients[i].sock, "You are currently challenging a player. Only /cancelchallenge is available.");
                  }

               } else if (clients[i].state == STATE_CHALLENGED) {
               // Handle commands in the challenged state

                  if (strncmp(buffer, "/accept", 7) == 0) {
                  // Accept command : to accept a challenge from another player

                     char challenger_name[BUF_SIZE];
                     if (sscanf(buffer, "/accept %s", challenger_name) == 1) {
                        int challenger_idx = -1;
                        for(int j=0; j<actual; j++) {
                           if(strcmp(clients[j].name, challenger_name) == 0) {
                              challenger_idx = j;
                              break;
                           }
                        }

                        if(challenger_idx != -1 && clients[challenger_idx].challenging_who == i) {
                           clients[challenger_idx].state = STATE_INGAME;
                           clients[i].state = STATE_INGAME;
                           clients[challenger_idx].opponent = i;
                           clients[i].opponent = challenger_idx;
                           clients[challenger_idx].challenging_who = -1;

                           write_client(clients[challenger_idx].sock, "CLEAR_CHAT");
                           write_client(clients[i].sock, "CLEAR_CHAT");

                           snprintf(acceptance_msg, sizeof(acceptance_msg) - strlen(acceptance_msg), "CHALLENGE_ACCEPTED %s. You are now playing with %s.", clients[challenger_idx].name, clients[i].name);
                           write_client(clients[challenger_idx].sock, acceptance_msg);

                           snprintf(confirmation_msg, sizeof(confirmation_msg) - strlen(confirmation_msg), "You have accepted the challenge from %s. You are now playing with %s.", clients[challenger_idx].name, clients[challenger_idx].name);
                           write_client(clients[i].sock, confirmation_msg);

                           char msg[BUF_SIZE];
                           snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_INGAME);
                           write_client(clients[challenger_idx].sock, msg);
                           write_client(clients[i].sock, msg);

                           
                           struct game* g = create_game(&clients[i], &clients[challenger_idx], (clients[challenger_idx].challenge_mode == 0) ? PUBLIC : PRIVATE);
                           struct board* b = create_board(g);
                           clients[i].Currentboard = b;
                           clients[challenger_idx].Currentboard = b;
                           snprintf(msg, BUF_SIZE, "CHANGE_BOARD%s", convert_board_to_string_player_view(b, &clients[challenger_idx]));
                           write_client(clients[challenger_idx].sock, msg);
                           snprintf(msg, BUF_SIZE, "CHANGE_BOARD%s", convert_board_to_string_player_view(b, &clients[i]));
                           write_client(clients[i].sock, msg);


                        } else {
                           write_client(clients[i].sock, "Invalid challenger or challenge not found.");
                        }
                     } else {
                        write_client(clients[i].sock, "Usage: /accept <username>");
                     }
                  } else if (strncmp(buffer, "/refuse", 7) == 0) {
                     char challenger_name[BUF_SIZE];
                     if (sscanf(buffer, "/refuse %s", challenger_name) == 1) {
                        int challenger_idx = -1;
                        for(int j=0; j<actual; j++) {
                           if(strcmp(clients[j].name, challenger_name) == 0) {
                              challenger_idx = j;
                              break;
                           }
                        }

                        if(challenger_idx != -1 && clients[challenger_idx].challenging_who == i) {
                           clients[challenger_idx].state = STATE_LOBBY;
                           clients[challenger_idx].challenging_who = -1;
                           clients[i].state = STATE_LOBBY;

                           snprintf(refusal_msg, sizeof(refusal_msg), "CHALLENGE_REFUSED %s", clients[i].name);
                           write_client(clients[challenger_idx].sock, refusal_msg);

                           snprintf(confirmation_msg, sizeof(confirmation_msg), "You have refused the challenge from %s.", clients[challenger_idx].name);
                           write_client(clients[i].sock, confirmation_msg);

                           char msg[BUF_SIZE];
                           snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
                           write_client(clients[challenger_idx].sock, msg);
                           snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
                           write_client(clients[i].sock, msg);
                        } else {
                           write_client(clients[i].sock, "Invalid challenger or challenge not found.");
                        }
                     } else {
                        write_client(clients[i].sock, "Usage: /refuse <username>");
                     }
                  } else {
                     write_client(clients[i].sock, "You have been challenged. Only /accept <challenger> or /refuse <challenger> are available.");
                  }
               } else if (clients[i].state == STATE_INGAME) {
                  if (strcmp(buffer, "/quitgame") == 0) {
                     int opponent_idx = clients[i].opponent;

                     /* Notify the player who quit */
                     write_client(clients[i].sock, "CLEAR_CHAT");
                     write_client(clients[i].sock, "You have quit the game. You are now back in the lobby.");
                     clients[i].state = STATE_LOBBY;
                     clients[i].opponent = -1;
                     char msg[BUF_SIZE];
                     snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
                     write_client(clients[i].sock, msg);

                     /* Notify the opponent */
                     if (opponent_idx != -1) {
                        write_client(clients[opponent_idx].sock, "CLEAR_CHAT");
                        write_client(clients[opponent_idx].sock, "Your opponent has quit the game. You are now back in the lobby.");
                        clients[opponent_idx].state = STATE_LOBBY;
                        clients[opponent_idx].opponent = -1;
                        clients[opponent_idx].Currentboard = NULL;
                        write_client(clients[opponent_idx].sock, msg);
                        opponent_idx = -1; // Mark that opponent has been handled
                     }

                     /* Notify the spectators */
                     struct game* g = clients[i].Currentboard->gameRef;
                     struct listeChaineeSpectateurs* currentSpec = g->spectators->head;
                     while (currentSpec != NULL) {
                        struct listeChaineeSpectateurs* temp = currentSpec;
                        currentSpec = currentSpec->next;
                        write_client(temp->spectator->sock, "CLEAR_CHAT");
                        write_client(temp->spectator->sock, "A player has quit the game. You are now back in the lobby.");
                        temp->spectator->state = STATE_LOBBY;
                        temp->spectator->Currentboard = NULL;
                        write_client(temp->spectator->sock, msg);
                     }

                     if (opponent_idx == -1 && g->spectators->nbSpectators == 0) {
                        /* No one left in the game, clean up */
                        delete_the_game(clients[i].Currentboard->gameRef);
                        delete_the_board(clients[i].Currentboard);
                     }
                     clients[i].Currentboard = NULL;

                  } else if (strncmp(buffer, "/move", 5) == 0) {
                     char move_letter[BUF_SIZE];
                     if (sscanf(buffer, "/move %s", move_letter) == 1) {
                        Client* client1 = &clients[i];
                        Client* client2;
                        if (client1 == client1->Currentboard->gameRef->player1) {
                           client2 = client1->Currentboard->gameRef->player2;
                        } else {
                           client2 = client1->Currentboard->gameRef->player1;
                        }
                        int res = try_a_move(client1->Currentboard->gameRef, move_letter[0], client1->Currentboard, client1);
                        if (res != 1 && res != 0) {
                           char err_msg[70];
                           switch (res)
                           {
                              case 2:
                                 snprintf(err_msg, sizeof(err_msg), "You can't play this move because you need to feed your opponent.");
                                 break;
                              case 3:
                                 snprintf(err_msg, sizeof(err_msg), "This pit is empty. Choose another pit.");
                                 break;
                              case 4:
                                 snprintf(err_msg, sizeof(err_msg), "This is not your pit. Choose one of your own pits.");
                                 break;
                              case 5:
                                 snprintf(err_msg, sizeof(err_msg), "This is not a pit, please use /move [pit].");
                                 break;
                              case 6:
                                 snprintf(err_msg, sizeof(err_msg), "It's not your turn. Please wait for your turn.");
                                 break;
                              default:
                                 snprintf(err_msg, sizeof(err_msg), "Unknown error.");
                                 break;
                           }
                           write_client(client1->sock, err_msg);
                        } else {
                           /* Notify about the move */
                           char board_update_msg[BUF_SIZE];
                           snprintf(board_update_msg, BUF_SIZE - strlen(board_update_msg), "%s chose to sow from %c", client1->name, move_letter[0]);
                           write_client(client1->sock, board_update_msg);
                           write_client(client2->sock, board_update_msg);

                           /* Update the boards */
                           char msg[BUF_SIZE];
                           snprintf(msg, BUF_SIZE, "CHANGE_BOARD %s", convert_board_to_string_player_view(client1->Currentboard, client1));
                           char msg2[BUF_SIZE];
                           snprintf(msg2, BUF_SIZE, "CHANGE_BOARD %s", convert_board_to_string_player_view(client1->Currentboard, client2));
                           char msg3[BUF_SIZE];
                           snprintf(msg3, BUF_SIZE, "CHANGE_BOARD %s", convert_board_to_string(client1->Currentboard));
                           write_client(client1->sock, msg);
                           write_client(client2->sock, msg2);

                           /* If the game is over, notify */
                           char msgend[BUF_SIZE];
                           if (res == 1) {
                              int diff = client1->Currentboard->gameRef->scoreP1 - client1->Currentboard->gameRef->scoreP2;
                              if (diff == 0) {
                                    snprintf(msgend, sizeof(msgend) - strlen(msgend), "The game is over! It's a tie!");
                                    write_client(client1->sock, msgend);
                                    write_client(client2->sock, msgend);
                              } else if (diff > 0) {
                                    snprintf(msgend, sizeof(msgend) - strlen(msgend), "The game is over! %s wins %d to %d!", client1->Currentboard->gameRef->player1->name, client1->Currentboard->gameRef->scoreP1, client1->Currentboard->gameRef->scoreP2);
                                    write_client(client1->sock, msgend);
                                    write_client(client2->sock, msgend);
                              } else if (diff < 0) {
                                    snprintf(msgend, sizeof(msgend) - strlen(msgend), "The game is over! %s wins %d to %d!", client1->Currentboard->gameRef->player2->name, client1->Currentboard->gameRef->scoreP2, client1->Currentboard->gameRef->scoreP1);
                                    write_client(client1->sock, msgend);
                                    write_client(client2->sock, msgend);
                                    break;
                              }
                              strncat(msgend, "You can quit with /quitgame.", sizeof(msgend) - strlen(msgend) - 1);
                              write_client(client1->sock, "You can quit with /quitgame.");
                              write_client(client2->sock, "You can quit with /quitgame.");

                              
                              
                           }
                           /* Notify the spectators of all previous messages that were sent to the players */
                           struct listeChaineeSpectateurs* currentSpec = client1->Currentboard->gameRef->spectators->head;
                           while (currentSpec != NULL) {
                              struct listeChaineeSpectateurs* temp = currentSpec;
                              currentSpec = currentSpec->next;
                              write_client(temp->spectator->sock, board_update_msg);
                              write_client(temp->spectator->sock, msg3);
                              if (res == 1) {
                                 write_client(temp->spectator->sock, msgend);
                              }
                           }
                        }
                     }
                        
                        
                     
                  } else if (buffer[0] != '/') {
                     char final_msg[BUF_SIZE];
                     snprintf(final_msg, BUF_SIZE - strlen(final_msg), "%s: %s", clients[i].name, buffer);

                     /* Send to players */
                     write_client(clients[clients[i].opponent].sock, final_msg);
                     write_client(clients[i].sock, final_msg);

                     /* Send to spectators */
                     struct listeChaineeSpectateurs* currentSpec = clients[i].Currentboard->gameRef->spectators->head;
                     while (currentSpec != NULL) {
                        struct listeChaineeSpectateurs* temp = currentSpec;
                        currentSpec = currentSpec->next;
                        write_client(temp->spectator->sock, final_msg);
                     }

                  } else {
                     write_client(clients[i].sock, "You are in a game. Only /quitgame, /move <pit_number>, /clearchat and in-game chat are available.");
                  }
               } else if (clients[i].state == STATE_BIO) {
                  if (strcmp(buffer, "/endbio") == 0) {
                     clients[i].state = STATE_LOBBY;
                     char msg[BUF_SIZE];
                     write_client(clients[i].sock, "CLEAR_CHAT");
                     snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
                     write_client(clients[i].sock, msg);
                  } else if (strcmp(buffer, "/clearbio") == 0) {
                     for (int j = 0; j < BIO_MAX_LINES; j++) {
                        clients[i].bio[j][0] = '\0';
                     }
                     write_client(clients[i].sock, "CLEAR_CHAT");
                     write_client(clients[i].sock, "Your bio has been cleared. Write your new bio:");
                  } else {
                     for (int j = 0; j < BIO_MAX_LINES; j++) {
                        if (clients[i].bio[j][0] == '\0') {
                           strncpy(clients[i].bio[j], buffer, BIO_MAX_LENGTH - 1);
                           clients[i].bio[j][BIO_MAX_LENGTH - 1] = '\0';
                           write_client(clients[i].sock, buffer);
                           break;
                        }
                     }
                  }
               } else if (clients[i].state == STATE_SPECTATE) {
                  if (strcmp(buffer, "/quitgame") == 0) {
                     /* Notify the player who quit */
                     write_client(clients[i].sock, "CLEAR_CHAT");
                     write_client(clients[i].sock, "You have quit the game. You are now back in the lobby.");
                     clients[i].state = STATE_LOBBY;
                     char msg[BUF_SIZE];
                     snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
                     write_client(clients[i].sock, msg);

                     /* Notify the players */
                     Client* player1 = clients[i].Currentboard->gameRef->player1;
                     Client* player2 = clients[i].Currentboard->gameRef->player2;
                     char notify_msg[BUF_SIZE];
                     snprintf(notify_msg, sizeof(notify_msg) - strlen(notify_msg), "%s stopped viewing your game.", clients[i].name);
                     write_client(player1->sock, notify_msg);
                     write_client(player2->sock, notify_msg);

                     /* Remove leaving spectator from the game's spectator list */
                     remove_spectator_from_game(clients[i].Currentboard->gameRef, &clients[i]);

                     /* Notify the spectators */
                     struct listeChaineeSpectateurs* currentSpec = clients[i].Currentboard->gameRef->spectators->head;
                     while (currentSpec != NULL) {
                        struct listeChaineeSpectateurs* temp = currentSpec;
                        currentSpec = currentSpec->next;
                        char notify_msg_spec[BUF_SIZE];
                        snprintf(notify_msg_spec, sizeof(notify_msg_spec) - strlen(notify_msg_spec), "%s stopped viewing the game.", clients[i].name);
                        write_client(temp->spectator->sock, notify_msg_spec);
                     }

                     clients[i].Currentboard = NULL;
                  
                  } else if (buffer[0] != '/') {
                     char final_msg[BUF_SIZE];
                     snprintf(final_msg, BUF_SIZE - strlen(final_msg), "%s: %s", clients[i].name, buffer);

                     /* Send to players */
                     write_client(clients[i].Currentboard->gameRef->player1->sock, final_msg);
                     write_client(clients[i].Currentboard->gameRef->player2->sock, final_msg);

                     /* Send to spectators */
                     struct listeChaineeSpectateurs* currentSpec = clients[i].Currentboard->gameRef->spectators->head;
                     while (currentSpec != NULL) {
                        struct listeChaineeSpectateurs* temp = currentSpec;
                        currentSpec = currentSpec->next;
                        write_client(temp->spectator->sock, final_msg);
                     }

                  } else {
                     write_client(clients[i].sock, "You are watching a game. Only /quitgame, /clearchat and in-game chat are available.");
                  }
               } else {
                  /* Fallback for unhandled states or commands */
                  write_client(clients[i].sock, "Unknown command or invalid state.");
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}



int server_init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()") ;
      exit(errno);
   }

   int reuse = 1;
   if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < -0)
   {
      perror("setsockopt()") ;
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()") ;
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()") ;
      exit(errno);
   }

   return sock;
}

void end_connection(int sock)
{
   closesocket(sock);
}

int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()") ;
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

void write_client(SOCKET sock, const char *buffer)
{
   char new_buffer[BUF_SIZE];
   snprintf(new_buffer, BUF_SIZE, "%s\n", buffer);
   if(send(sock, new_buffer, strlen(new_buffer), 0) < 0)
   {
      perror("send()") ;
      exit(errno);
   }
}

int main(void)
{
   init();

   server_app();

   end();

   return EXIT_SUCCESS;
}

void handle_client_disconnection(Client *clients, int i, int *actual)
{
   char msg[BUF_SIZE * 3];
   /* If client was in game, notify opponent */
   if (clients[i].state == STATE_INGAME) {
      int opponent_idx = clients[i].opponent;
      if (opponent_idx != -1) {
         clients[opponent_idx].state = STATE_LOBBY;
         clients[opponent_idx].opponent = -1;

         snprintf(msg, sizeof(msg), "Your opponent %s has disconnected. You are back in the lobby.", clients[i].name);
         write_client(clients[opponent_idx].sock, msg);

         snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
         write_client(clients[opponent_idx].sock, msg);
         opponent_idx = -1;
      }

      /* If there were other spectators, notify them */
      if (clients[i].Currentboard->gameRef->spectators->nbSpectators != 0) {
         struct game* g = clients[i].Currentboard->gameRef;
         struct listeChaineeSpectateurs* currentSpec = g->spectators->head;

         while (currentSpec != NULL) {
            struct listeChaineeSpectateurs* temp = currentSpec;
            currentSpec = currentSpec->next;
            snprintf(msg, sizeof(msg), "The player %s has disconnected. You are back in the lobby.", clients[i].name);
            write_client(temp->spectator->sock, msg);

            snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
            write_client(temp->spectator->sock, msg);
         }
      }

      if (opponent_idx == -1 && clients[i].Currentboard->gameRef->spectators->nbSpectators == 0) {
         /* No one left in the game, clean up */
         delete_the_game(clients[i].Currentboard->gameRef);
         delete_the_board(clients[i].Currentboard);
      }
   } else if (clients[i].state == STATE_CHALLENGING) {
      int challenged_idx = clients[i].challenging_who;
      if (challenged_idx != -1) {
         snprintf(msg, sizeof(msg), "The challenge from %s has been canceled.", clients[i].name);
         write_client(clients[challenged_idx].sock, msg);
         snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
         write_client(clients[challenged_idx].sock, msg);
      }
   } else if (clients[i].state == STATE_CHALLENGED) {
      /* Find the challenger */
      for (int j = 0; j < *actual; j++) {
         if (clients[j].challenging_who == i) {
            clients[j].state = STATE_LOBBY;
            clients[j].challenging_who = -1;
            snprintf(msg, sizeof(msg), "%s is no longer available for a challenge.", clients[i].name);
            write_client(clients[j].sock, msg);
            snprintf(msg, BUF_SIZE, "STATE_UPDATE %d", STATE_LOBBY);
            write_client(clients[j].sock, msg);
            break;
         }
      }
   } else if (clients[i].state == STATE_SPECTATE) {
      /* Remove leaving spectator from the game's spectator list */
      remove_spectator_from_game(clients[i].Currentboard->gameRef, &clients[i]);

      /* Notify the players */
      Client* player1 = clients[i].Currentboard->gameRef->player1;
      Client* player2 = clients[i].Currentboard->gameRef->player2;
      char notify_msg[BUF_SIZE];
      snprintf(notify_msg, sizeof(notify_msg) - strlen(notify_msg), "%s stopped viewing your game.", clients[i].name);
      write_client(player1->sock, notify_msg);
      write_client(player2->sock, notify_msg);

      /* Notify the other spectators */
      struct listeChaineeSpectateurs* currentSpec = clients[i].Currentboard->gameRef->spectators->head;
      while (currentSpec != NULL) {
         struct listeChaineeSpectateurs* temp = currentSpec;
         currentSpec = currentSpec->next;
         char notify_msg_spec[BUF_SIZE];
         snprintf(notify_msg_spec, sizeof(notify_msg_spec) - strlen(notify_msg_spec), "%s stopped viewing the game.", clients[i].name);
         write_client(temp->spectator->sock, notify_msg_spec);
      }
   }

   char disconnected_client_name[BUF_SIZE];
   strncpy(disconnected_client_name, clients[i].name, BUF_SIZE - 1);

   closesocket(clients[i].sock);
   save_player_data(&clients[i]);
   int removed_index = i;
   remove_client(clients, i, actual);

   /* Adjust opponent and challenging_who indices */
   for (int j = 0; j < *actual; j++) {
      if (clients[j].opponent > removed_index) {
         clients[j].opponent--;
      } else if (clients[j].opponent == removed_index) {
         clients[j].state = STATE_LOBBY;
         clients[j].opponent = -1;
      }
      if (clients[j].challenging_who > removed_index) {
         clients[j].challenging_who--;
      } else if (clients[j].challenging_who == removed_index) {
         clients[j].state = STATE_LOBBY;
         clients[j].challenging_who = -1;
      }
   }

   snprintf(msg, sizeof(msg), "%s disconnected !", disconnected_client_name);
   for (int j = 0; j < *actual; j++) {
      if (clients[j].state == STATE_LOBBY) {
         write_client(clients[j].sock, msg);
      }
   }
}