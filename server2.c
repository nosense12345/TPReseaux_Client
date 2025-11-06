#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>


static void init(void)
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

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void server_app(void)
{
   SOCKET sock = server_init_connection();
   char buffer[BUF_SIZE];
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
         perror("select()");
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
            perror("accept()");
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
            int username_taken = 0;
            for (int i = 0; i < actual; i++)
            {
               if (strcmp(clients[i].name, username) == 0)
               {
                  username_taken = 1;
                  break;
               }
            }

            if (username_taken)
            {
               write_client(csock, "Username is already taken\n");
               closesocket(csock);
            }
            else
            {
               /* what is the new maximum fd ? */
               max = csock > max ? csock : max;
               FD_SET(csock, &rdfs);

               Client c = { .sock = csock, .name = "" };
               strncpy(c.name, username, BUF_SIZE - 1);
               for (int j = 0; j < BIO_MAX_LINES; j++) {
                  c.bio[j][0] = '\0';
               }
               for (int j = 0; j < MAX_FRIENDS; j++) {
                  c.friends[j] = -1;
               }
               c.state = STATE_LOBBY;
               c.opponent = -1;
               clients[actual] = c;
               actual++;
               write_client(csock, "Welcome! You are registered.\n");
            }
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
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  for (int j = 0; j < actual; j++) {
                     if (clients[j].state == STATE_LOBBY) {
                        write_client(clients[j].sock, buffer);
                     }
                  }
               }
               else if (strcmp(buffer, "/list") == 0) {
                  char user_list[BUF_SIZE] = "Online users:\n";
                  for (int j = 0; j < actual; j++) {
                     strncat(user_list, clients[j].name, BUF_SIZE - strlen(user_list) - 1);
                     strncat(user_list, "\n", BUF_SIZE - strlen(user_list) - 1);
                  }
                  write_client(clients[i].sock, user_list);
               }
               else if (strncmp(buffer, "/challenge", 10) == 0) {
                  char challenged_name[BUF_SIZE];
                  sscanf(buffer, "CHALLENGE %s", challenged_name);

                  int challenger_idx = -1;
                  for(int j=0; j<actual; j++) {
                     if(clients[j].sock == client.sock) {
                        challenger_idx = j;
                        break;
                     }
                  }

                  int challenged_idx = -1;
                  for(int j=0; j<actual; j++) {
                     if(strcmp(clients[j].name, challenged_name) == 0) {
                        challenged_idx = j;
                        break;
                     }
                  }

                  if(challenged_idx != -1) {
                     char challenge_msg[BUF_SIZE];
                     snprintf(challenge_msg, BUF_SIZE, "CHALLENGE_FROM %s", clients[challenger_idx].name);
                     write_client(clients[challenged_idx].sock, challenge_msg);
                  } else {
                     write_client(client.sock, "Player not found");
                  }
               }
               else if (strncmp(buffer, "/accept", 7) == 0) {
                  char challenger_name[BUF_SIZE];
                  sscanf(buffer, "ACCEPT %s", challenger_name);

                  int challenger_idx = -1;
                  for(int j=0; j<actual; j++) {
                     if(strcmp(clients[j].name, challenger_name) == 0) {
                        challenger_idx = j;
                        break;
                     }
                  }

                  int challenged_idx = -1;
                  for(int j=0; j<actual; j++) {
                     if(clients[j].sock == client.sock) {
                        challenged_idx = j;
                        break;
                     }
                  }

                  if(challenger_idx != -1) {
                     char acceptance_msg[BUF_SIZE];
                     snprintf(acceptance_msg, BUF_SIZE, "CHALLENGE_ACCEPTED %s", clients[challenged_idx].name);
                     write_client(clients[challenger_idx].sock, acceptance_msg);
                     write_client(clients[challenged_idx].sock, "You have accepted the challenge.");
                  }
               }
               else if (strncmp(buffer, "/refuse", 7) == 0) {
                  char challenger_name[BUF_SIZE];
                  sscanf(buffer, "REFUSE %s", challenger_name);

                  int challenger_idx = -1;
                  for(int j=0; j<actual; j++) {
                     if(strcmp(clients[j].name, challenger_name) == 0) {
                        challenger_idx = j;
                        break;
                     }
                  }

                  if(challenger_idx != -1) {
                     char refusal_msg[BUF_SIZE];
                     snprintf(refusal_msg, BUF_SIZE, "CHALLENGE_REFUSED %s", client.name);
                     write_client(clients[challenger_idx].sock, refusal_msg);
                     write_client(client.sock, "You have refused the challenge.");
                  }
               }
               else if (strncmp(buffer, "/viewbio", 8) == 0) {
                  char username[BUF_SIZE];
                  if (sscanf(buffer, "/viewbio %s", username) == 1) {
                     int user_idx = -1;
                     for (int j = 0; j < actual; j++) {
                        if (strcmp(clients[j].name, username) == 0) {
                           user_idx = j;
                           break;
                        }
                     }

                     if (user_idx != -1) {
                        char bio_response[BUF_SIZE] = "";
                        for (int j = 0; j < BIO_MAX_LINES; j++) {
                           if (clients[user_idx].bio[j][0] != '\0') {
                              strncat(bio_response, clients[user_idx].bio[j], BUF_SIZE - strlen(bio_response) - 1);
                              strncat(bio_response, "\n", BUF_SIZE - strlen(bio_response) - 1);
                           }
                        }
                        if (strlen(bio_response) == 0) {
                           write_client(clients[i].sock, "This user has no bio.");
                        } else {
                           write_client(clients[i].sock, bio_response);
                        }
                     } else {
                        write_client(clients[i].sock, "User not found.");
                     }
                  } else {
                     write_client(clients[i].sock, "Usage: /viewbio <username>");
                  }
               }
               else if (strcmp(buffer, "/friends") == 0) {
                  char friend_list[BUF_SIZE] = "Your friends:\n";
                  int friend_count = 0;
                  for (int j = 0; j < MAX_FRIENDS; j++) {
                     if (clients[i].friends[j] != -1) {
                        strncat(friend_list, clients[clients[i].friends[j]].name, BUF_SIZE - strlen(friend_list) - 1);
                        strncat(friend_list, "\n", BUF_SIZE - strlen(friend_list) - 1);
                        friend_count++;
                     }
                  }
                  if (friend_count == 0) {
                     write_client(clients[i].sock, "You have no friends.");
                  } else {
                     write_client(clients[i].sock, friend_list);
                  }
               }
               else if (strncmp(buffer, "/removefriend", 13) == 0) {
                  char friend_name[BUF_SIZE];
                  if (sscanf(buffer, "/removefriend %s", friend_name) == 1) {
                     int friend_idx = -1;
                     for (int j = 0; j < actual; j++) {
                        if (strcmp(clients[j].name, friend_name) == 0) {
                           friend_idx = j;
                           break;
                        }
                     }

                     if (friend_idx != -1) {
                        int removed = 0;
                        for (int j = 0; j < MAX_FRIENDS; j++) {
                           if (clients[i].friends[j] == friend_idx) {
                              clients[i].friends[j] = -1;
                              removed = 1;
                              break;
                           }
                        }
                        if (removed) {
                           write_client(clients[i].sock, "Friend removed.");
                        } else {
                           write_client(clients[i].sock, "User not found in your friend list.");
                        }
                     } else {
                        write_client(clients[i].sock, "User not found.");
                     }
                  } else {
                     write_client(clients[i].sock, "Usage: /removefriend <username>");
                  }
               }
               else if (strncmp(buffer, "/addfriend", 10) == 0) {
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
                        int added = 0;
                        for (int j = 0; j < MAX_FRIENDS; j++) {
                           if (clients[i].friends[j] == -1) {
                              clients[i].friends[j] = friend_idx;
                              added = 1;
                              break;
                           }
                        }
                        if (added) {
                           write_client(clients[i].sock, "Friend added.");
                        } else {
                           write_client(clients[i].sock, "Your friend list is full.");
                        }
                     } else {
                        write_client(clients[i].sock, "User not found.");
                     }
                  } else {
                     write_client(clients[i].sock, "Usage: /addfriend <username>");
                  }
               }
               else if (strcmp(buffer, "/clearbio") == 0) {
                  for (int j = 0; j < BIO_MAX_LINES; j++) {
                     clients[i].bio[j][0] = '\0';
                  }
                  write_client(clients[i].sock, "Your bio has been cleared.");
               }
               else if (strncmp(buffer, "/bio", 4) == 0) {
                  char *bio_line = buffer + 5;
                  for (int j = 0; j < BIO_MAX_LINES; j++) {
                     if (clients[i].bio[j][0] == '\0') {
                        strncpy(clients[i].bio[j], bio_line, BIO_MAX_LENGTH - 1);
                        clients[i].bio[j][BIO_MAX_LENGTH - 1] = '\0';
                        break;
                     }
                  }
               }
               else if (strcmp(buffer, "/endbio") == 0) {
                  write_client(clients[i].sock, "Your bio has been updated.");
               }
               else
               {
                  send_message(clients, i, actual, buffer, 0);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

void send_message(Client *clients, int sender_idx, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;

   if (clients[sender_idx].state == STATE_LOBBY) {
      for(i = 0; i < actual; i++)
      {
         if(clients[i].state == STATE_LOBBY && i != sender_idx)
         {
            if(from_server == 0)
            {
               strncpy(message, clients[sender_idx].name, BUF_SIZE - 1);
               strncat(message, " : ", sizeof message - strlen(message) - 1);
            }
            strncat(message, buffer, sizeof message - strlen(message) - 1);
            write_client(clients[i].sock, message);
         }
      }
   } else if (clients[sender_idx].state == STATE_CHALLENGED || clients[sender_idx].state == STATE_INGAME) {
      if (clients[sender_idx].opponent != -1) {
         if(from_server == 0)
         {
            strncpy(message, clients[sender_idx].name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[clients[sender_idx].opponent].sock, message);
      }
   }
}

static int server_init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   server_app();

   end();

   return EXIT_SUCCESS;
}
