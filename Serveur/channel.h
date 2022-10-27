#ifndef CHANNEL_H
#define CHANNEL_H

#define MAX_CHANNELS 20

typedef struct 
{
    char name[BUF_SIZE];
    int nb_recipients;
    char owner[BUF_SIZE];
    char recipients[MAX_CLIENTS][BUF_SIZE];
}Channel;

#endif /* guard */