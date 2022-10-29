#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "server.h"
#include "client.h"
#include "channel.h"
#include "privatemessage.h"

int nb_clients = 0;
Client * clients[MAX_CLIENTS];
int nb_channels = 0;
Channel * channels[MAX_CHANNELS];
int nb_private_messages = 0;
PrivateMessage * private_messages[MAX_PRIVATE_MESSAGES];

const char * help = "Voici la liste des commandes possibles :\n\t/private Destinataire Message => qui permet d'envoyer un message privé au Destinataire.\n\t/create NomChannel => qui permet de créer un nouveau canal.\n\t/add NomDestinataire => qui permet d'ajouter une personne dans le canal choisi avec /join.\n\t/join NomChannel => qui permet de choisir de communiquer sur le canal NomChannel.\n\t/send Message => qui permet d'envoyer un message dans le canal choisi avec /join.\n\t/which => qui permet de savoir sur quel canal vous vous trouvez.\n\t/list => qui permet de lister vos canaux et messages privés.\n\nPar défault vous communiquez sur le canal public (message envoyé à tous les utilisateurs).\nPour revenir au canal public entrez : /join public\n";

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

static PrivateMessage* getPrivateMessage(const char * pm_name){
   for(int i=0; i< nb_private_messages; i++){
      if(strcmp(private_messages[i]->name, pm_name)==0){
         return private_messages[i];
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

static int inPrivateMessage(Client * client, PrivateMessage * private_message){
   for(int i=0; i<2; i++){
      if(strcmp(client->name, private_message->recipients[i])==0){
         return 1;
      }
   }
   return 0;
}

/* Read the command of client */
static void read_command(const char * buffer, Client * expediteur){
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
   if(strcmp(command, "/create")==0){
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
   } else if (strcmp(command, "/add")==0) {
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
   } else if (strcmp(command, "/join")==0){
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
               strcpy(str, "[");
               strcat(str, channel->name);
               strcat(str, "] ");
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
         send_message_to_all_clients(expediteur, arg, 0);
      }
   } else if (strcmp(command, "/private")==0){
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

      char pm_name[BUF_SIZE];
      if (strcmp(expediteur->name, dest)<0)
      {
         strcpy(pm_name, expediteur->name);
         strcat(pm_name, dest);
      }else {
         strcpy(pm_name, dest);
         strcat(pm_name, expediteur->name);
      }
      
      PrivateMessage * private_message = getPrivateMessage(pm_name);

      if(private_message == NULL) {
         private_message = (PrivateMessage *)malloc(sizeof(PrivateMessage));
         strcpy(private_message->name, pm_name);
         strcpy(private_message->recipients[0], expediteur->name);
         strcpy(private_message->recipients[1], dest);
         private_messages[nb_private_messages] = private_message;
         nb_private_messages++;
      }

      Client * destinataire = getClient(dest);
      char str[BUF_SIZE];
      strcpy(str, "🔒 ");
      strcat(str, expediteur->name);
      strcat(str, " : ");
      strcat(str, message);
      if(destinataire != NULL){
         write_client(destinataire->sock, str);
      } else {
         store_unread_message(str, dest);
      }
      
   } else if (strcmp(command, "/which")==0){
      if(expediteur->channel!=NULL){
         char str[BUF_SIZE];
         strcpy(str, "[");
         strcat(str, expediteur->channel->name);
         strcat(str, "]");
         write_client(expediteur->sock, str);
      } else {
         write_client(expediteur->sock, "[public]");
      }
   } else if (strcmp(command, "/list")==0){
      list_conversations(expediteur);
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
      write_client(client->sock, "🔔 Vous n'avez aucun message non lu.\n");
   } else {
      int nb_messages_non_lus = 0;
      char line[BUF_SIZE]="";
      char c; 
      while ((c=fgetc(fp))!=EOF)
      {
         if(c=='\n'){
            nb_messages_non_lus++;
         }
      }
      char nbNonLus[8];
      sprintf(nbNonLus, "%d", nb_messages_non_lus);
      char notif[BUF_SIZE];
      strcpy(notif, "🔔 Vous avez ");
      strcat(notif, nbNonLus);
      strcat(notif, " message(s) non lu(s):\n");
      write_client(client->sock, notif);
      fseek(fp, 0, SEEK_SET);
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
      remove(file_name);
   }
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

static void list_conversations(Client *client){
   write_client(client->sock, "Liste de vos conversations :\n");
   write_client(client->sock, "👪 Groupe\n");
   write_client(client->sock, "🔒 Privé\n\n");
   write_client(client->sock, "-------------------------\n");
   int exists = 0;
   for (int i = 0; i < nb_channels; i++)
   {
      if(inChannel(client, channels[i])==1){
         exists = 1;
         char channel_name[BUF_SIZE];
         strcpy(channel_name, "👪 ");
         strcat(channel_name, channels[i]->name);
         strcat(channel_name, "\n");
         write_client(client->sock, channel_name);
      }
   }

   for (int i = 0; i < nb_private_messages; i++)
   {
      if(inPrivateMessage(client, private_messages[i])==1){
         exists = 1;
         char channel_name[BUF_SIZE];
         strcpy(channel_name, "🔒 ");
         strcat(channel_name, private_messages[i]->name);
         strcat(channel_name, "\n");
         write_client(client->sock, channel_name);
      }
   }

   if(exists == 0){
      write_client(client->sock, "(vide)\n");
   }
   write_client(client->sock, "-------------------------\n");
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
            print_unread_messages(client);
            print_connection_serveur(client, " connected ! ");
         }else{
            c->sock = csock;
            write_client(c->sock, help);
            print_unread_messages(c);
            print_connection_serveur(c, " connected ! ");
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
                  getState();
                  //print_message_serveur(client, buffer);
               }
               /* client disconnected */
               if(c == 0)
               {
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(client, buffer, 1);
                  print_connection_serveur(client, " disconnected !");
                  closesocket(client->sock);
                  remove_client(client);
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

static void send_message_to_all_clients(Client * sender, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < nb_clients; i++)
   {
      /* we don't send message to the sender */
      if(sender->sock != clients[i]->sock)
      {
         if(from_server == 0)
         {
            
            strncpy(message, "[public] ", BUF_SIZE - 1);
            strncat(message, sender->name, sizeof message - strlen(message) - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         /* We don't send message to other client if deconnected */
         if(clients[i]->sock != 0){
            write_client(clients[i]->sock, message);
         }
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
   for (int i = 0; i < nb_private_messages; i++)
   {
      printf("- Private message %d\n", i+1);
      printf("*name : %s\n", private_messages[i]->name);
      printf("*recipient 1 : %s.\n", private_messages[i]->recipients[0]);
      printf("*recipient 2 : %s.\n\n", private_messages[i]->recipients[1]);
   }
   for (int i = 0; i < nb_clients; i++)
   {
      printf("- Client %d\n", i+1);
      printf("*sock : %d\n", clients[i]->sock);
      printf("*name : %s.\n", clients[i]->name);
      printf("*channel : %s\n\n", clients[i]->channel->name);
   }
   for (int i = 0; i < nb_channels; i++)
   {
      printf("- Channel %d\n", i+1);
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
   int i;
   for (i = 0; i < nb_clients; i++)
   {
      free(clients[i]);
   }
   for (i = 0; i < nb_channels; i++)
   {
      free(channels[i]);
   }
   for (i = 0; i < nb_private_messages; i++)
   {
      free(private_messages[i]);
   }
}

static void load_data() {
   printf("Loading data...\n");

   // Reading the number of clients, the number of channels and the number of private messages
   char * line = (char *) malloc( BUF_SIZE );

   FILE * fp = fopen("./Backups/backup_params.txt", "r");
   if(fp == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_params.txt");   
      exit(1);
   }

   while (!feof(fp))
   {
      fgets(line, BUF_SIZE, fp);
      const char delim[2] = ";";
      char * token = strtok(line, delim);
      int j=0;
      while(token != NULL){
         if(j==0){
            nb_clients = atoi(token);
         }else if(j==1) {
            nb_channels = atoi(token);
         }else{
            nb_private_messages = atoi(token);
         }
         token = strtok(NULL, delim);
         j++;
      }  
   }

   fclose(fp);

   // Reading existing channels 
   FILE * fch = fopen("./Backups/backup_channels.txt", "r");
   if(fch == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_channels.txt");   
      exit(1);
   }
   
   int i = 0;
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

   // Reading existing clients
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
      c->sock = 0;
      const char delim[2] = "/";
      char * token = strtok(line, delim);
      int j=0;
      while(token != NULL){
         if (j==0) {
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

   // Reading existing private messages
   FILE * fpm = fopen("./Backups/backup_pms.txt", "r");
   if(fpm == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_pms.txt");   
      exit(1);
   }
   
   i = 0;
   while (!feof(fpm)) {
      fgets(line, BUF_SIZE, fpm);

      PrivateMessage * pm = (PrivateMessage *)malloc(sizeof(PrivateMessage));

      const char delim[2] = "/";

      char * token = strtok(line, delim);

      int j=0;
      while(token != NULL){
         if(j==0){
            strcpy(pm->name, token);
         }else if (j==1) {
            strcpy(pm->recipients[0], token);
         }else {
            token[strlen(token)-1] = '\0';
            strcpy(pm->recipients[1], token);
         }
         token = strtok(NULL, delim);
         j++;
      }
      
      private_messages[i] = pm;
      i++;
   }
   
   fclose(fpm);

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
   fprintf(fp, "%s;", buf);
   sprintf(buf, "%d", nb_private_messages);
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
      fprintf(fc, "%s/", clients[i]->name);
      fprintf(fc, "%s\n", clients[i]->channel->name);
   }
   
   fclose(fc);

   FILE * fpm = fopen("./Backups/backup_pms.txt", "w");
   if(fpm == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./Backups/backup_pms.txt");   
      exit(1);
   }

   for (int i = 0; i < nb_private_messages; i++)
   {
      fprintf(fpm, "%s/", private_messages[i]->name);
      fprintf(fpm, "%s/", private_messages[i]->recipients[0]);
      fprintf(fpm, "%s\n", private_messages[i]->recipients[1]);
   }
   
   fclose(fpm);
   
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