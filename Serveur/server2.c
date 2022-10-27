#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"
#include "channel.h"

static Client* getClient(const char * dest_name, Client * clients, int * nb_clients){
   for(int i=0; i<*nb_clients; i++){
      if(strcmp(clients[i].name, dest_name)==0){
         return &clients[i];
      }
   }
   return NULL;
}

static Channel* getChannel(const char * channel_name, Channel ** channels, int * nb_channels){
   for(int i=0; i<*nb_channels; i++){
      if(strcmp(channels[i]->name, channel_name)==0){
         return channels[i];
      }
   }
   return NULL;
}

static int inChannel(Client * client, Channel * channel){
   if(strcmp(client->name, channel->owner)==0){
      return 1;
   }else{
      for(int i=0; i<channel->nb_recipients; i++){
         if(strcmp(client->name, channel->recipients[i])==0){
            return 1;
         }
      }
   }
   return 0;
}

/* Read the command of client */
static void read_command(const char * buffer, Client * expediteur, Client * clients, int * nb_clients, Channel * channels[], int * nb_channels){
   char str[BUF_SIZE];
   strcpy(str, buffer);
   
   const char delim[2] = " ";

   char command[BUF_SIZE];
   char arg[BUF_SIZE]="";

   char * token = strtok(str, delim);

   int i=0;
   while(token != NULL){
      if(i==0){
         strcpy(command, token);
      }else{
         strcat(arg, token);
         strcat(arg, " ");
      }
      token = strtok(NULL, delim);
      i++;
   }
   arg[strlen(arg)-1] = '\0';

   exec_command(command, arg, expediteur, clients, nb_clients, channels, nb_channels);
}

static void exec_command(const char * command, char * arg, Client * expediteur, Client* clients, int * nb_clients, Channel ** channels, int * nb_channels){
   if(strcmp(command, "/createchannel")==0){
      if(getChannel(arg, channels, nb_channels) == NULL){
         // On crée le canal
         Channel * channel = (Channel *)malloc(sizeof(Channel));
         strcpy(channel->name, arg);
         channel->nb_recipients = 1;
         strcpy(channel->owner, expediteur->name);
         strcpy(channel->recipients[0], expediteur->name);
         // Puis on l'ajoute dans la liste des caneaux
         channels[*nb_channels] = channel;
         *nb_channels = *nb_channels + 1;
      }else{
         write_client(expediteur->sock, "Le nom du channel existe déjà !\n");
      }
   } else if (strcmp(command, "/addrecipient")==0) {
      Channel * ch = expediteur->channel;
      if(ch != NULL && strcmp(expediteur->name, ch->owner)==0){
         Client * c = getClient(arg, clients, nb_clients);
         if(c != NULL){
            strcpy(ch->recipients[ch->nb_recipients], c->name);
            ch->nb_recipients++;
         }else{
            write_client(expediteur->sock, "Ce destinataire n'existe pas !\n");
         }
      } else {
         write_client(expediteur->sock, "/!\\ Vous n'êtes pas le propriétaire du canal ou n'avez pas choisi de canal /!\\\n");
      }
   } else if (strcmp(command, "/setchannel")==0){
      if(strcmp(arg, "public")==0){
         expediteur->channel = NULL;
      } else {
         Channel * ch = getChannel(arg, channels, nb_channels);
         if(ch != NULL && inChannel(expediteur, ch)==1){
            // On enregistre l'utilisateur sur le canal qu'il a choisi
            expediteur->channel = ch;
            // Puis on lui affiche tout l'historique du canal
            char file_name[BUF_SIZE];
            strcpy(file_name, "./Historiques/");
            strcat(file_name, arg);
            strcat(file_name, ".txt");
            FILE * f = fopen(file_name, "rb");
            if(f == NULL)
            {
               printf("Erreur d'ouverture du fichier: ./Historiques/%s.txt", arg);   
               exit(1);
            }
            char str[BUF_SIZE]="";
            char * buf = 0;
            fseek (f, 0, SEEK_END);
            long length = ftell (f);
            fseek (f, 0, SEEK_SET);
            buf = malloc(length);
            if (buf)
            {
               fread(buf, 1, length, f);
               strcat(str, buf);
            }
            fclose(f);
            write_client(expediteur->sock, str);
         }else{
            write_client(expediteur->sock, "Ce canal n'existe pas !\n");
         }
      }
   } else if (strcmp(command, "/send")==0){
      Channel * channel = expediteur->channel;
      if(channel != NULL){
         for(int i=0; i<channel->nb_recipients; i++){
            if(strcmp(expediteur->name, channel->recipients[i])!=0){
               Client * destinataire = getClient(channel->recipients[i], clients, nb_clients);
               char str[BUF_SIZE];
               strcpy(str, "(");
               strcat(str, channel->name);
               strcat(str, ") ");
               strcat(str, expediteur->name);
               strcat(str, " : ");
               strcat(str, arg);
               write_client(destinataire->sock, str);
               char file_name[BUF_SIZE];
               strcpy(file_name, "./Historiques/");
               strcat(file_name, channel->name);
               strcat(file_name, ".txt");
               FILE * f = fopen(file_name, "a");
               if(f == NULL)
               {
                  printf("Erreur d'ouverture du fichier: ./Historiques/%s.txt", channel->name);   
                  exit(1);
               }
               fprintf(f, "%s\n", str);
               fclose(f);
            }
         }
      } else {
         send_message_to_all_clients(clients, *expediteur, *nb_clients, arg, 0);
      }
   } else if (strcmp(command, "/sendDM")==0){
      const char delim[2] = " ";

      char dest[BUF_SIZE];
      char message[BUF_SIZE]="";

      char * token = strtok(arg, delim);

      int i=0;
      while(token != NULL){
         if(i==0){   
            strcpy(dest, token);
         }else{
            strcat(message, token);
            strcat(message, " ");
         }
         token = strtok(NULL, delim);
         i++;
      }
      message[strlen(message)-1] = '\0';

      Client * destinataire = getClient(dest, clients, nb_clients);
      char str[BUF_SIZE];
      strcpy(str, expediteur->name);
      strcat(str, " : ");
      strcat(str, message);
      write_client(destinataire->sock, str);

   } else {
      const char * help = "Voici la liste des commandes possibles :\n\t/sendDM Destinataire Message => qui permet d'envoyer un message privé au Destinataire.\n\t/createchannel NomChannel => qui permet de créer un nouveau canal.\n\t/addrecipient NomDestinataire => qui permet d'ajouter une personne dans le canal choisi avec /setchannel.\n\t/setchannel NomChannel => qui permet de choisir de communiquer sur le canal NomChannel.\n\t/send Message => qui permet d'envoyer un message dans le canal choisi avec /setchannel.\n\nPar défault vous communiquez sur le canal public (message envoyé à tous les utilisateurs).\nPour revenir au canal public entrez : /setchannel public\n";
      write_client(expediteur->sock, help);
   }
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
   Channel * channels[MAX_CHANNELS];
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
         const char * help = "Voici la liste des commandes possibles :\n\t/sendDM Destinataire Message => qui permet d'envoyer un message privé au Destinataire.\n\t/createchannel NomChannel => qui permet de créer un nouveau canal.\n\t/addrecipient NomDestinataire => qui permet d'ajouter une personne dans le canal choisi avec /setchannel.\n\t/setchannel NomChannel => qui permet de choisir de communiquer sur le canal NomChannel.\n\t/send Message => qui permet d'envoyer un message dans le canal choisi avec /setchannel.\n\nPar défault vous communiquez sur le canal public (message envoyé à tous les utilisateurs).\nPour revenir au canal public entrez : /setchannel public\n";
         write_client(c.sock, help);
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client * client = &clients[i];
               int c = read_client(clients[i].sock, buffer);
               if(c>0){
                  read_command(buffer, client, clients, &actual,channels, &nb_channels);
                  //print_message_serveur(client, buffer);
               }
               
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  print_connection_serveur(*client, " disconnected !");
                  send_message_to_all_clients(clients, *client, actual, buffer, 1);
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
