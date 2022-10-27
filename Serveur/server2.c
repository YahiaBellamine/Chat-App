#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"
#include "channel.h"

static Client* getClient(const char * dest_name, Client * clients){
   for(int i=0; i<MAX_CLIENTS; i++){
      if(strcmp(clients[i].name, dest_name)==0){
         return &clients[i];
      }
   }
   return NULL;
}

static Channel* getChannel(const char * channel_name, Channel * channels, Client * client){
   for(int i=0; i<MAX_CHANNELS; i++){
      if(strcmp(channels[i].name, channel_name)==0 && strcmp(client->name, client->channel->owner)==0){
         return &channels[i];
      }
   }
   return NULL;
}

/* Read the command of client */
static void read_command(const char * buffer, Client * expediteur, Client * clients, int * nb_clients, Channel * channels, int * nb_channels){
   char * str = (char *)malloc(BUF_SIZE*sizeof(char));
   strcpy(str, buffer);
   
   const char delim[2] = " ";

   char command[BUF_SIZE];
   char arg[BUF_SIZE];

   char * token = strtok(str, delim);

   int i=0;
   while(token != NULL){
      if(i==0){
         strcpy(command, token);
      }else if(i==1){
         strcpy(arg, token);
         strcat(arg, " ");
      }else{
         strcat(arg, token);
         strcat(arg, " ");
      }
   }
   free(str);
   exec_command(command, arg, expediteur, clients, nb_clients, channels, nb_channels);
}

static void exec_command(const char * command, const char * arg, Client * expediteur,  Client* clients, int * nb_clients, Channel * channels, int * nb_channels){
   if(strcmp(command, "/createchannel")==0){
      Channel channel;
      strcpy(channel.name, arg);
      channel.nb_recipients = 0;
      strcpy(channel.owner, expediteur->name);
      channels[*nb_channels] = channel;
      *nb_channels++;
   } else if (strcmp(command, "/addrecipient")==0) {
      if(strcmp(expediteur->name, expediteur->channel->owner)==0){
         Channel * channel = expediteur->channel;
         strcpy(channel->recipients[channel->nb_recipients], getClient(arg, clients)->name);
         channel->nb_recipients++;
      }
   } else if (strcmp(command, "/setchannel")==0){
      if(strcmp(arg, "public")==0){
         expediteur->channel = NULL;
      } else {
         expediteur->channel = getChannel(arg, channels, expediteur);
      }
   } else if(strcmp(command, "/send")==0)
      {
      Channel * channel = expediteur->channel;
      if(channel != NULL){
         for(int i=0; i<channel->nb_recipients; i++)
         {
            Client * destinataire = getClient(channel->recipients[i], clients);
            char str[BUF_SIZE];
            strcpy(str, expediteur->name);
            strcat(str, " : ");
            strcat(str, arg);
            if(destinataire!=NULL)
            {
                write_client(destinataire->sock, str);
            }
            else
            {
               stock_unread_message(str,channel->recipients[i],expediteur);
            }
         }
      } 
      // else if(strcmp(command, "/private")==0) // cette fonction sert a tester l'historique
      // {
      
      //    for(int i=0; i<channel->nb_recipients; i++)
      //    {
      //       Client * destinataire = getClient(channel->recipients[i], clients);
      //       char str[BUF_SIZE];
      //       strcpy(str, expediteur->name);
      //       strcat(str, " : ");
      //       strcat(str, arg);
      //       if(destinataire!=NULL)
      //       {
      //           write_client(destinataire->sock, str);
      //       }
      //       else
      //       {
      //          stock_unread_message(str,channel->recipients[i]);
      //       }
      //    }
      // } 
      else {
         send_message_to_all_clients(clients, *expediteur, *nb_clients, arg, 0);
      }
   } else {
      const char * help = "Voici la liste des commandes possibles :\n\t/createchannel NomChannel => qui permet de créer un nouveau canal (privé, groupe).\n\t/addrecipient NomDestinataire => qui permet d'ajouter une personne dans le canal.\n\t/setchannel NomChannel => qui permet de choisir de communiquer sur le canal NomChannel.\n\t/send Message => qui permet d'envoyer un message dans le canal choisi avec /setchannel.\n\nPar défault vous communiquez sur le canal public (message envoyé à tous les utilisateurs).\nPour revenir au canal public entrez : /setchannel public\n";
      write_client(expediteur->sock, help);
   }
}

static void stock_unread_message(char str[BUF_SIZE],const char * dest_name,Client* expediteur)
{
   char arg[BUF_SIZE+5];
   strcpy(arg,dest_name);
   strcat(arg,".txt");
    FILE* fp = fopen(arg,"a+");
    if(fp==NULL)
    {     
        fclose(fp);
        //   cree par exemple bob.txt
         fp = fopen(arg,"w");            
         if(fp==NULL)    
         {   
            perror("Error: ");
        // return(-1);
         }
    }
    int len=strlen(expediteur->name);
    int i;
    for(i=0;i<len;i++) fputc(expediteur->name[i],fp);
    fputc(':',fp);
    len=strlen(str);
    for(i=0;i<len;i++) fputc(str[i],fp);
    fputc('\n',fp);
    fclose(fp);
}

static void print_unread_message(Client* client)
{
   char arg[BUF_SIZE+5];
   strcpy(arg,client->name);
   strcat(arg,".txt");
   FILE* fp = fopen(arg,"r");
   if(fp!=NULL)
   {
      char str[BUF_SIZE]={""};
      char symbol;
      int i=0;
      while((symbol=fgetc(fp))!=EOF)
      {
         str[i]=symbol;
         i++;
         if(symbol=='\n')
         {
            write_client(client->sock, str);
            //strcpy(str,"");
            memset(str,0,sizeof(str)/sizeof(char)); 
            i=0;
         }
         
      }
   }
   fclose(fp);
   remove(arg);
   //int ret=remove(arg);
   //    if (ret== 0) {
   //      printf("The file is deleted successfully.");
   //  } else {
   //      printf("The file is not deleted.");
   //  }
   
}
static void print_connection_serveur(Client client, const char * status){
   char message[BUF_SIZE];
   message[0] = 0;
   strncpy(message, client.name, BUF_SIZE - 1);
   strncat(message, status, sizeof message - strlen(message) - 1);
   printf("%s\n", message);
}

static void print_message_serveur(Client client, const char * buffer){
   char message[BUF_SIZE];
   message[0] = 0;
   strncpy(message, client.name, BUF_SIZE - 1);
   strncat(message, " : ", sizeof message - strlen(message) - 1);
   strncat(message, buffer, sizeof message - strlen(message) - 1);
   printf("%s\n", message);
}

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
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

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   /* an array for all channels */
   Channel channels[MAX_CHANNELS];
   int nb_channels = 0;

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, (socklen_t*)&sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         c.channel = NULL;
         clients[actual] = c;
         actual++;
         print_connection_serveur(c, " connected ! ");
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               if(c>0){
                  read_command(buffer, &client, clients, &actual,channels, &nb_channels);
                  //print_message_serveur(client, buffer);
               }
               
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  print_connection_serveur(client, " disconnected !");
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  //send_message_to_all_clients(clients, client, actual, buffer, 0);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   int opt = 1;
   if (setsockopt(sock, SOL_SOCKET,
				SO_REUSEADDR | SO_REUSEPORT, &opt,
				sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
