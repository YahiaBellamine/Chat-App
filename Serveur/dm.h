#ifndef CHANNEL_H
#define CHANNEL_H

#define MAX_DMS 100

typedef struct 
{
    char name[BUF_SIZE];
    char recipients[2][BUF_SIZE];
}DM;

#endif /* guard */