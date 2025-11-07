#ifndef SERVER_H
#define SERVER_H


#include <sys/types.h>
#ifdef _WIN32
    // Windows-specific headers
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")  // Automatically link Winsock library
#else
    // POSIX / Linux / macOS headers
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR   -1
    #define closesocket(s) close(s)
    typedef int SOCKET;
#endif
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;


#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100

#define BUF_SIZE 2048

// In server2.h or a separate client_struct.h
#define BIO_MAX_LINES 10
#define BIO_MAX_LENGTH 80

#define MAX_FRIENDS 10

#include "common.h"

struct game;

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   char bio[BIO_MAX_LINES][BIO_MAX_LENGTH];
   int friends[MAX_FRIENDS];
   ClientState state;
   int opponent;
   int challenging_who;
   struct board* Currentboard;
} Client;



static void init(void);
static void end(void);
static void server_app(void);
static int server_init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);


#endif /* guard */
