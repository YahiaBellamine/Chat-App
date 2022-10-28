#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100

#define BUF_SIZE    1024

#define SEND "/send"
#define SET_CANAL "/set_canal"

#include "client2.h"
#include "channel.h"
#include "dm.h"

static Client* getClient(const char * client_name);
static Channel* getChannel(const char * channel_name);
static int inChannel(Client * client, Channel * channel);
static int inDm(Client * client, Dm * direct_message);
static void read_command(const char * buffer, Client * expediteur);
static void exec_command(const char * command, char * arg, Client * expediteur);
static void print_unread_messages(Client * client);
static void store_unread_message(const char * unread_message, const char * destinataire);
static void print_message_serveur(Client * client, const char * buffer);
static void print_connection_serveur(Client * client, const char * status);
static void list_conversations(Client *client);
static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client * client, const char *buffer, char from_server);
static void remove_client();
static void clear_clients();
static void free_memory();
static void load_data();
static void store_data(int code);
static void getState();

#endif /* guard */
