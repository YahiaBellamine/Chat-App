#ifndef CLIENT_H
#define CLIENT_H

#include "channel.h"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   Channel * channel;
}Client;

#endif /* guard */
