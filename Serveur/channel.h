#ifndef CHANNEL_H
#define CHANNEL_H

#include "client2.h"

#define MAX_CHANNELS 20

typedef struct 
{
    char name[BUF_SIZE];
    int nb_recipients;
    Client recipients[MAX_CLIENTS];
}Channel;

#endif /* guard */