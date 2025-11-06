#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "client2.h"
#include "simple_ui.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
    
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET;
    
#endif




static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      ui_add_message("WSAStartup failed !");
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

static void app(const char *address, const char *name)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];

   fd_set rdfs;
   int bio_mode = 0;
   int challenged_mode = 0;
   char challenger_name[BUF_SIZE];

   ui_init();

   /* send our name */
   char register_command[BUF_SIZE];
   snprintf(register_command, BUF_SIZE, "REGISTER %s", name);
   write_server(sock, register_command);

   while(1)
   {
      ui_redraw_all();

      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the socket */
      FD_SET(sock, &rdfs);

      if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         ui_get_input(buffer, BUF_SIZE);
         
         if (bio_mode) {
            if (strcmp(buffer, "/endbio") == 0) {
               bio_mode = 0;
               write_server(sock, "/endbio");
               ui_add_message("Bio editing finished.");
            } else if (strcmp(buffer, "/clearbio") == 0) {
               write_server(sock, "/clearbio");
            } else {
               char bio_command[BUF_SIZE];
               snprintf(bio_command, BUF_SIZE, "/bio %s", buffer);
               write_server(sock, bio_command);
            }
         } else if (strcmp(buffer, "/endbio") == 0) {
            ui_add_message("You are not in bio editing mode.");
         } else if (strcmp(buffer, "/help") == 0) {
            if (bio_mode) {
               ui_add_message("Available commands:\n/endbio - Exit bio editing mode\n/clearbio - Clear your bio\n");
            } else if (challenged_mode) {
               ui_add_message("Available commands:\n/accept [challenger] - Accept the challenge\n/refuse [challenger] - Refuse the challenge\n");
            } else {
               ui_add_message("Available commands:\n/list - List online users\n/bio - Enter bio editing mode\n/viewbio [user] - View a user's bio\n/challenge [user] - Challenge a user\n/addfriend [user] - Add a user as a friend\n/removefriend [user] - Remove a user from your friends list\n/friends - List your friends\n");
            }
         } else if (strcmp(buffer, "/list") == 0) {
            write_server(sock, "/list");
         }
         else if (strncmp(buffer, "/addfriend", 10) == 0) {
            char command[BUF_SIZE];
            char friend_name[BUF_SIZE];
            if(sscanf(buffer, "%s %s", command, friend_name) == 2){
               snprintf(command, BUF_SIZE, "/addfriend %s", friend_name);
               write_server(sock, command);
            } else {
               ui_add_message("Usage: /addfriend <username>\n");
            }
         }
         else if (strncmp(buffer, "/removefriend", 13) == 0) {
            char command[BUF_SIZE];
            char friend_name[BUF_SIZE];
            if(sscanf(buffer, "%s %s", command, friend_name) == 2){
               snprintf(command, BUF_SIZE, "/removefriend %s", friend_name);
               write_server(sock, command);
            }
            else {
               ui_add_message("Usage: /removefriend <username>\n");
            }
         }
         else if (strcmp(buffer, "/friends") == 0) {
            write_server(sock, "/friends");
         }
         else if (strcmp(buffer, "/bio") == 0) {
            bio_mode = 1;
            write_server(sock, "/bio");
            ui_add_message("You are now in bio editing mode. Type /endbio to finish.\n");
         }
         else if (strncmp(buffer, "/viewbio", 8) == 0) {
            char command[BUF_SIZE];
            char username[BUF_SIZE];
            if(sscanf(buffer, "%s %s", command, username) == 2){
               snprintf(command, BUF_SIZE, "/viewbio %s", username);
               write_server(sock, command);
            } else {
               ui_add_message("Usage: /viewbio <username>\n");
            }
         }
         else if (strncmp(buffer, "/challenge", 10) == 0) {
            char command[BUF_SIZE];
            char challenged_name[BUF_SIZE];
            if(sscanf(buffer, "%s %s", command, challenged_name) == 2){
               snprintf(command, BUF_SIZE, "/challenge %s", challenged_name);
               write_server(sock, command);
            } else {
               ui_add_message("Usage: /challenge <username>\n");
            }
         }
         else if (strncmp(buffer, "/accept", 7) == 0) {
            if (challenged_mode) {
               char command[BUF_SIZE];
               char provided_challenger[BUF_SIZE];
               if(sscanf(buffer, "%*s %s", provided_challenger) == 1 && strcmp(provided_challenger, challenger_name) == 0){
                  snprintf(command, BUF_SIZE, "/accept %s", challenger_name);
                  write_server(sock, command);
                  challenged_mode = 0;
               } else {
                  ui_add_message("Invalid challenger name.\n");
               }
            } else {
               ui_add_message("You are not being challenged.\n");
            }
         }
         else if (strncmp(buffer, "/refuse", 7) == 0) {
            if (challenged_mode) {
               char command[BUF_SIZE];
               char provided_challenger[BUF_SIZE];
               if(sscanf(buffer, "%*s %s", provided_challenger) == 1 && strcmp(provided_challenger, challenger_name) == 0){
                  snprintf(command, BUF_SIZE, "/refuse %s", challenger_name);
                  write_server(sock, command);
                  challenged_mode = 0;
               } else {
                  ui_add_message("Invalid challenger name.\n");
               }
            }
            else {
               ui_add_message("You are not being challenged.\n");
            }
         } else if (strcmp(buffer, "/quit") == 0) {
            break;
         } else {
            write_server(sock, buffer);
         }
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         int n = read_server(sock, buffer);
         /* server down */
         if(n == 0)
         {
            ui_add_message("Server disconnected !\n");
            break;
         }
         if (strncmp(buffer, "CHALLENGE_FROM", 14) == 0) {
            sscanf(buffer, "CHALLENGE_FROM %s", challenger_name);
            ui_add_message(challenger_name);
            char challenge_msg[BUF_SIZE];
            snprintf(challenge_msg, BUF_SIZE, "%s has challenged you! Type /accept %s or /refuse %s.\n", challenger_name, challenger_name, challenger_name);
            ui_add_message(challenge_msg);
            challenged_mode = 1;
         } else if (strncmp(buffer, "CHALLENGE_ACCEPTED", 18) == 0) {
            char opponent_name[BUF_SIZE];
            sscanf(buffer, "CHALLENGE_ACCEPTED %s", opponent_name);
            char acceptance_msg[BUF_SIZE];
            snprintf(acceptance_msg, BUF_SIZE, "Your challenge has been accepted by %s!\n", opponent_name);
            ui_add_message(acceptance_msg);
         } else if (strncmp(buffer, "CHALLENGE_REFUSED", 17) == 0) {
            char opponent_name[BUF_SIZE];
            sscanf(buffer, "CHALLENGE_REFUSED %s", opponent_name);
            char refusal_msg[BUF_SIZE];
            snprintf(refusal_msg, BUF_SIZE, "%s has refused your challenge.\n", opponent_name);
            ui_add_message(refusal_msg);
         } else {
            ui_add_message(buffer);
         }
      }
   }

   ui_cleanup();
   end_connection(sock);
}

static int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };
   struct hostent *hostinfo;

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf (stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(struct in_addr *)hostinfo->h_addr_list[0];
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_server(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }

   buffer[n] = 0;

   return n;
}

static void write_server(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   if(argc < 3)
   {
      printf("Usage : %s [address] [pseudo]\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();

   app(argv[1], argv[2]);

   end();

   return EXIT_SUCCESS;
}