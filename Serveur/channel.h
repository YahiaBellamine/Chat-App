#ifndef CHANNEL_H
#define CHANNEL_H

#define MAX_CHANNELS 20

typedef struct 
{
    char name[BUF_SIZE];
    int nb_recipients;
    char * owner;
    char * recipients[MAX_CLIENTS];
}Channel;

#endif /* guard */