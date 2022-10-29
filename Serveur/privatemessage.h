#ifndef PRIVATEMESSAGE_H
#define PRIVATEMESSAGE_H

#define MAX_PRIVATE_MESSAGES 100

typedef struct 
{
    char name[BUF_SIZE];
    char recipients[2][BUF_SIZE];
}PrivateMessage;

#endif /* guard */