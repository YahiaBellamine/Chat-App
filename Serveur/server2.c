#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "server2.h"
#include "client2.h"
#include "channel.h"

int nb_clients = 0;
Client * clients[MAX_CLIENTS];
int nb_channels = 0;
Channel * channels[MAX_CHANNELS];

const char * help = "Voici la liste des commandes possibles :\n\t/sendDM Destinataire Message => qui permet d'envoyer un message privé au Destinataire.\n\t/createchannel NomChannel => qui permet de créer un nouveau canal.\n\t/addrecipient NomDestinataire => qui permet d'ajouter une personne dans le canal choisi avec /setchannel.\n\t/setchannel NomChannel => qui permet de choisir de communiquer sur le canal NomChannel.\n\t/send Message => qui permet d'envoyer un message dans le canal choisi avec /setchannel.\n\nPar défault vous communiquez sur le canal public (message envoyé à tous les utilisateurs).\nPour revenir au canal public entrez : /setchannel public\n";


static Client* getClient(const char * client_name){
   for(int i=0; i< nb_clients; i++){
      if(strcmp(clients[i]->name, client_name)==0){
         return clients[i];
      }
   }
   return NULL;
}

static Channel* getChannel(const char * channel_name){
   for(int i=0; i< nb_channels; i++){
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
static void read_command(const char * buffer, Client * expediteur){
   getState();
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

   exec_command(command, arg, expediteur);
}

static void exec_command(const char * command, char * arg, Client * expediteur){
   if(strcmp(command, "/createchannel")==0){
      if(getChannel(arg) == NULL){
         // On crée le canal
         Channel * channel = (Channel *)malloc(sizeof(Channel));
         strcpy(channel->name, arg);
         channel->nb_recipients = 1;
         strcpy(channel->owner, expediteur->name);
         strcpy(channel->recipients[0], expediteur->name);
         // Puis on l'ajoute dans la liste des caneaux
         channels[nb_channels] = channel;
         nb_channels++;
         // Et on crée le fichier qui va contenir son historique
         char file_name[BUF_SIZE];
         strcpy(file_name, "./Historiques/");
         strcat(file_name, arg);
         strcat(file_name, ".txt");
         FILE * f = fopen(file_name, "a");
         fclose(f);
      }else{
         write_client(expediteur->sock, "Le nom du channel existe déjà !\n");
      }
   } else if (strcmp(command, "/addrecipient")==0) {
      Channel * ch = expediteur->channel;
      if(ch != NULL && strcmp(expediteur->name, ch->owner)==0){
         Client * c = getClient(arg);
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
         Channel * ch = getChannel(arg);
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
            char buf[BUF_SIZE]="";
            fseek(f, 0, SEEK_END);
            long length = ftell(f);
            fseek(f, 0, SEEK_SET);
            fread(buf, 1, length, f);
            write_client(expediteur->sock, buf);
            fclose(f);
         }else{
            write_client(expediteur->sock, "Ce canal n'existe pas ou vous n'y avez pas accès !\n");
         }
      }
   } else if (strcmp(command, "/send")==0){
      Channel * channel = expediteur->channel;
      if(channel != NULL){
         for(int i=0; i<channel->nb_recipients; i++){
            if(strcmp(expediteur->name, channel->recipients[i])!=0){
               Client * destinataire = getClient(channel->recipients[i]);
               char str[BUF_SIZE];
               strcpy(str, "(");
               strcat(str, channel->name);
               strcat(str, ") ");
               strcat(str, expediteur->name);
               strcat(str, " : ");
               strcat(str, arg);
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
               if(destinataire != NULL){
                  write_client(destinataire->sock, str);
               } else {
                  store_unread_message(str, channel->recipients[i]);
               }
            }
         }
      } else {
         send_message_to_all_clients(*expediteur, arg, 0);
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

      Client * destinataire = getClient(dest);
      char str[BUF_SIZE];
      strcpy(str, expediteur->name);
      strcat(str, " : ");
      strcat(str, message);
      write_client(destinataire->sock, str);
   } else {
      write_client(expediteur->sock, help);
   }
}

static void print_unread_messages(Client * client){
   char file_name[BUF_SIZE];
   strcpy(file_name, "./Non_lus/");
   strcat(file_name, client->name);
   strcat(file_name, ".txt");
   FILE * fp = fopen(file_name, "r");
   if(fp==NULL){
      write_client(client->sock, "Vous n'avez aucun message non lu.");
   } else {
      char line[BUF_SIZE]="";
      char c;
      int i=0;
      while ((c=fgetc(fp))!=EOF)
      {
         line[i] = c;
         i++;
         if(c=='\n'){
            line[i-1] = 0;
            write_client(client->sock, line);
            memset(line, 0, BUF_SIZE);
            i=0;
         }
      }
      fclose(fp);
   }
   remove(file_name);
}

static void store_unread_message(const char * unread_message, const char * destinataire){
   char file_name[BUF_SIZE];
   strcpy(file_name, "./Non_lus/");
   strcat(file_name, destinataire);
   strcat(file_name, ".txt");
   FILE * fp = fopen(file_name, "a");
   if(fp == NULL){
      perror("Erreur d'ouverture du fichier.");
   }
   fprintf(fp, "%s\n",unread_message);
   fclose(fp);
}

static void print_connection_serveur(Client * client, const char * status){
   char message[BUF_SIZE];
   message[0] = 0;
   strncpy(message, client->name, BUF_SIZE - 1);
   strncat(message, status, sizeof message - strlen(message) - 1);
   printf("%s\n", message);
}

static void print_message_serveur(Client * client, const char * buffer){
   char message[BUF_SIZE];
   message[0] = 0;
   strncpy(message, client->name, BUF_SIZE - 1);
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
   int max = sock;

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
      for(i = 0; i < nb_clients; i++)
      {
         FD_SET(clients[i]->sock, &rdfs);
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

         Client * c = getClient(buffer);
         if(c==NULL){
            Client * client = (Client *)malloc(sizeof(Client));
            client->sock = csock;
            strncpy(client->name, buffer, BUF_SIZE - 1);
            client->channel = NULL;
            clients[nb_clients] = client;
            nb_clients++;
            write_client(client->sock, help);
            print_connection_serveur(client, " connected ! ");
            print_unread_messages(client);
         }else{
            write_client(c->sock, help);
            print_connection_serveur(c, " connected ! ");
            print_unread_messages(c);
         }
      }
      else
      {
         int i = 0;
         for(i = 0; i < nb_clients; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i]->sock, &rdfs))
            {
               Client * client = clients[i];
               int c = read_client(clients[i]->sock, buffer);
               if(c>0){
                  read_command(buffer, client);
                  //print_message_serveur(client, buffer);
               }
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i]->sock);
                  remove_client(client);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  print_connection_serveur(client, " disconnected !");
                  send_message_to_all_clients(*client, buffer, 1);
               }
               else
               {
                  //send_message_to_all_clients(clients, client, nb_clients, buffer, 0);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients);
   end_connection(sock);
}

static void clear_clients()
{
   int i = 0;
   for(i = 0; i < nb_clients; i++)
   {
      closesocket(clients[i]->sock);
   }
}

static void remove_client(Client * client)
{
   free(client);
   nb_clients--;
}

static void send_message_to_all_clients(Client sender, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < nb_clients; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i]->sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i]->sock, message);
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

static void getState(){
   printf("------------------------------\n");
   for (int i = 0; i < nb_clients; i++)
   {
      printf("client %d\n", i+1);
      printf("*sock : %d\n", clients[i]->sock);
      printf("*name : %s.\n", clients[i]->name);
      printf("*channel : %s\n\n", clients[i]->channel->name);
   }
   for (int i = 0; i < nb_channels; i++)
   {
      printf("Channel %d\n", i+1);
      printf("*name : %s.\n", channels[i]->name);
      printf("*nb_recipients : %d\n", channels[i]->nb_recipients);
      printf("*owner : %s.\n", channels[i]->owner);
      printf("*recipients :\n");
      for (int j = 0; j < channels[i]->nb_recipients; j++)
      {
         printf("\t- recipient %d: %s.\n", j+1, channels[i]->recipients[j]);
      }
   }
   printf("------------------------------\n");  
   printf("\n");
}

static void free_memory(){
   for (int i = 0; i < nb_clients; i++)
   {
      free(clients[i]);
   }
   for (int i = 0; i < nb_channels; i++)
   {
      free(channels[i]);
   }
}

static void load_data() {
   printf("Loading data...\n");

   // Lecture des nombre de clients et de channels
   char * line = (char *) malloc( BUF_SIZE );

   FILE * fp = fopen("./Backups/backup_params.txt", "r");
   if(fp == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_params.txt");   
      exit(1);
   }

   while (! feof(fp))
   {
      fgets(line, BUF_SIZE, fp);
   }
   
   const char delim[2] = ";";

   char * token = strtok(line, delim);

   int i=0;
   while(token != NULL){
      if(i==0){
         nb_clients = atoi(token);
      }else {
         nb_channels = atoi(token);
      }
      token = strtok(NULL, delim);
      i++;
   }

   fclose(fp);

   // Lecture des channels
   FILE * fch = fopen("./Backups/backup_channels.txt", "r");
   if(fch == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_channels.txt");   
      exit(1);
   }
   
   i = 0;
   while (i < nb_channels && ! feof(fch)) {
      fgets(line, BUF_SIZE, fch);

      Channel * ch = (Channel *)malloc(sizeof(Channel));

      const char delim[2] = "/";

      char * token = strtok(line, delim);

      int j=0;
      while(token != NULL){
         if(j==0){
            strcpy(ch->name, token);
         }else if (j==1) {
            ch->nb_recipients = atoi(token);
         }else if (j==2) {
            strcpy(ch->owner, token);
         }else {
            char buffer[BUF_SIZE];
            strcpy(buffer, token);
            buffer[strcspn(buffer, "\n")] = 0;
            strcpy(ch->recipients[j-3], buffer);
         }
         token = strtok(NULL, delim);
         j++;
      }
      
      channels[i] = ch;
      i++;
   }

   fclose(fch); 

   // Lecture des clients
   FILE * fc = fopen("./Backups/backup_clients.txt", "r");
   if(fc == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_clients.txt");   
      exit(1);
   }
   
   i = 0;
   while (!feof(fc)) {
      fgets(line, BUF_SIZE, fc);

      Client * c = (Client *)malloc(sizeof(Client));

      const char delim[2] = "/";

      char * token = strtok(line, delim);

      int j=0;
      while(token != NULL){
         if(j==0){
            c->sock = atoi(token);
         }else if (j==1) {
            strcpy(c->name, token);
         }else {
            token[strlen(token)-1] = '\0';
            if(strcmp(token, "(null)")==0){
               c->channel = NULL;
            } else {
               c->channel = getChannel(token);
            }
         }
         token = strtok(NULL, delim);
         j++;
      }
      
      clients[i] = c;
      i++;
   }
   
   fclose(fc);

   if (line)
      free(line);
   
   printf("Data loaded.\n");
}

static void store_data(int code) {
   FILE * fp = fopen("./Backups/backup_params.txt", "w");
   if(fp == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_params.txt");   
      exit(1);
   }
   char buf[8];
   sprintf(buf, "%d", nb_clients);
   fprintf(fp, "%s;", buf);
   sprintf(buf, "%d", nb_channels);
   fprintf(fp, "%s", buf);

   FILE * fch = fopen("./Backups/backup_channels.txt", "w");
   if(fch == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_channels.txt");   
      exit(1);
   }

   for (int i = 0; i < nb_channels; i++)
   {
      fprintf(fch, "%s/", channels[i]->name);
      char nb_recipients[8];
      sprintf(nb_recipients, "%d", channels[i]->nb_recipients);
      fprintf(fch, "%s/", nb_recipients);
      fprintf(fch, "%s/", channels[i]->owner);
      for (int j = 0; j < (channels[i]->nb_recipients)-1; j++)
      {
         fprintf(fch, "%s/", channels[i]->recipients[j]);
      }
      fprintf(fch, "%s\n", channels[i]->recipients[(channels[i]->nb_recipients)-1]);      
   }

   fclose(fch);

   FILE * fc = fopen("./Backups/backup_clients.txt", "w");
   if(fc == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_clients.txt");   
      exit(1);
   }

   for (int i = 0; i < nb_clients; i++)
   {
      char sock[8];
      sprintf(sock, "%d", clients[i]->sock);
      fprintf(fc, "%s/", sock);
      fprintf(fc, "%s/", clients[i]->name);
      fprintf(fc, "%s\n", clients[i]->channel->name);
   }
   
   fclose(fc);
   
   free_memory();
}

int main(int argc, char **argv)
{
   signal(SIGINT, store_data);

   init();

   load_data();

   getState();

   app();

   end();

   return EXIT_SUCCESS;
}