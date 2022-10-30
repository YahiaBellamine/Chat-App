#ifndef CLIENT_H
#define CLIENT_H

#include "channel.h"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   char password[BUF_SIZE];
   int isAuthentified;
   Channel * channel;
}Client;

#endif /* guard */
