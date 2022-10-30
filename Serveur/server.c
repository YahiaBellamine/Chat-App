#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"
#include "channel.h"
#include "privatemessage.h"

int nb_clients = 0;
Client * clients[MAX_CLIENTS];

const char * welcome = "Bienvenue sur ChatApp !\nConnexion => /signin UserName Password\nInscription => /signup UserName Password\n";
const char * help = "Voici la liste des commandes possibles :\n\t/private Destinataire Message => qui permet d'envoyer un message privé au Destinataire.\n\t/create NomChannel => qui permet de créer un nouveau canal.\n\t/add NomDestinataire => qui permet d'ajouter une personne dans le canal choisi avec /join.\n\t/join NomChannel => qui permet de choisir de communiquer sur le canal NomChannel.\n\t/send Message => qui permet d'envoyer un message dans le canal choisi avec /join.\n\t/which => qui permet de savoir sur quel canal vous vous trouvez.\n\t/list => qui permet de lister vos canaux et messages privés.\n\t/help => pour avoir la liste des commandes possibles.\n\nPar défault vous communiquez sur le canal public (message envoyé à tous les utilisateurs).\nPour revenir au canal public entrez : /join public\n";

static void signUp(char * identifiers, Client * client){
   char name[BUF_SIZE];
   char password[BUF_SIZE];
   char * token = strtok(identifiers, " ");
   if(token!=NULL){
      strcpy(name, token);
   }
   token = strtok(NULL, " ");
   if(token!=NULL){
      strcpy(password, token);
   }

   strcpy(client->name, name);
   strcpy(client->password, password);

   FILE * fp = fopen("./db/users.bin", "ab");
   if(fp == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./db/users.bin");   
      exit(1);
   }
   
   if(getUser(client->name)==NULL){
      client->isAuthentified = 1;
      fwrite(client, sizeof(Client), 1, fp);
      write_client(client->sock, help);
      print_unread_messages(client);
      print_connection_serveur(client, " connected ! ");
   } else {
      write_client(client->sock, "Vous êtes déjà inscrit ! Veuillez vous connecter.\n");
   }

   fclose(fp);
}

static void signIn(char * identifiers, Client * client){
   char name[BUF_SIZE];
   char password[BUF_SIZE];
   char * token = strtok(identifiers, " ");
   if(token!=NULL){
      strcpy(name, token);
   }
   token = strtok(NULL, " ");
   if(token!=NULL){
      strcpy(password, token);
   }

   strcpy(client->name, name);
   strcpy(client->password, password);

   FILE * fp = fopen("./db/users.bin", "rb");
   if(fp == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./db/users.bin");   
      exit(1);
   }
   
   int verified = 0;
   Client * temp = (Client *)malloc(sizeof(Client));
   while(fread(temp, sizeof(Client), 1, fp)){
      if(strcmp(temp->name, client->name)==0 && strcmp(temp->password, client->password)==0){
         verified = 1;
         break;
      }
   }
   free(temp);

   if(verified==1){
      client->isAuthentified = 1;
      write_client(client->sock, help);
      print_unread_messages(client);
      print_connection_serveur(client, " connected ! ");
   } else {
      write_client(client->sock, "Votre identifiant ou votre mot de passe est incorrect.\n");
   }

   fclose(fp);

}

static Client* getUser(const char * user_name){
   FILE * fp = fopen("./db/users.bin", "rb");
   if(fp == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./db/users.bin");   
      exit(1);
   }
   Client * user = (Client *)malloc(sizeof(Client));
   while(fread(user, sizeof(Client), 1, fp)){
      if(strcmp(user->name, user_name)==0){
         fclose(fp);
         return user;
      }
   }
   fclose(fp);
   return NULL;
}

static Client* getClient(const char * client_name){
   for(int i=0; i< nb_clients; i++){
      if(strcmp(clients[i]->name, client_name)==0){
         return clients[i];
      }
   }
   return NULL;
}

static PrivateMessage* getPrivateMessage(const char * pm_name){
   FILE * fp = fopen("./db/private_messages.bin", "rb");
   if(fp != NULL)
   {
      PrivateMessage * private_message = (PrivateMessage *)malloc(sizeof(PrivateMessage));
      while(fread(private_message, sizeof(PrivateMessage), 1, fp)){
         if(strcmp(private_message->name, pm_name)==0){
            fclose(fp);
            return private_message;
         }
      }
      fclose(fp);
   }
   return NULL;
}

static void savePrivateMessage(PrivateMessage * private_message){
   FILE * fp = fopen("./db/private_messages.bin", "ab");
   if(fp == NULL)
   {
      printf("Erreur d'ouverture du fichier: ./db/private_messages.bin");   
      exit(1);
   }
   
   fwrite(private_message, sizeof(PrivateMessage), 1, fp);

   fclose(fp);
}

static Channel* getChannel(const char * channel_name){
   FILE * fp = fopen("./db/channels.bin", "rb");
   if(fp!=NULL){
      Channel * channel = (Channel *)malloc(sizeof(Channel));
      while(fread(channel, sizeof(Channel), 1, fp)){
         if(strcmp(channel->name, channel_name)==0){
            fclose(fp);
            return channel;
         }
      }
      fclose(fp);
   }
   return NULL;
}

static void saveChannel(Channel * channel){
   Channel * temp = getChannel(channel->name);
   if(temp==NULL){
      FILE * fp = fopen("./db/channels.bin", "ab+");
      if(fp == NULL)
      {
         printf("Erreur d'ouverture du fichier: ./db/channels.bin");   
         exit(1);
      }
      fwrite(channel, sizeof(Channel), 1, fp);
      fclose(fp);
   } else {
      FILE * fp = fopen("./db/channels.bin", "rb+");
      if(fp == NULL)
      {
         printf("Erreur d'ouverture du fichier: ./db/channels.bin");   
         exit(1);
      }
      while (fread(temp, sizeof(Channel), 1, fp))
      {
         if(strcmp(temp->name, channel->name)==0){
            fseek(fp, -sizeof(Channel), SEEK_CUR);
            fwrite(channel, sizeof(Channel), 1, fp);
            break;
         }
      }
      fclose(fp);
   }
   free(temp);
}

static int inPrivateMessage(Client * client, PrivateMessage * private_message){
   for(int i=0; i<2; i++){
      if(strcmp(client->name, private_message->recipients[i])==0){
         return 1;
      }
   }
   return 0;
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
   if(expediteur->isAuthentified==0){
      if (strcmp(command, "/signup")==0)
      {
         signUp(arg, expediteur);
      } else if (strcmp(command, "/signin")==0)
      {
         signIn(arg, expediteur);
      } else {
         write_client(expediteur->sock, "Veuillez vous inscrire ou vous connecter.\n");
      }
   } else {
      if(strcmp(command, "/create")==0){
         // On crée le canal
         Channel * channel = (Channel *)malloc(sizeof(Channel));
         strcpy(channel->name, arg);
         channel->nb_recipients = 1;
         strcpy(channel->owner, expediteur->name);
         strcpy(channel->recipients[0], expediteur->name);
         saveChannel(channel);
         free(channel);
         // Et on crée le fichier qui va contenir son historique
         char file_name[BUF_SIZE];
         strcpy(file_name, "./Historiques/");
         strcat(file_name, arg);
         strcat(file_name, ".txt");
         FILE * f = fopen(file_name, "a");
         fclose(f);
      } else if (strcmp(command, "/add")==0) {
         Channel * channel = expediteur->channel;
         if(channel != NULL){
            Client * dest = getUser(arg);
            if(dest!=NULL && inChannel(dest, channel)==0){
               strcpy(channel->recipients[channel->nb_recipients], arg);
               channel->nb_recipients++;
               saveChannel(channel);
            }else{
               write_client(expediteur->sock, "/!\\ Ce destinataire n'existe pas ou vous avez essayé d'ajouter un destinataire existant dans le canal /!\\\n");
            }
         } else {
            write_client(expediteur->sock, "/!\\ Vous devez rejoindre un canal pour ajouter des membres /!\\\n");
         }
      } else if (strcmp(command, "/join")==0){
         if(strcmp(arg, "public")==0){
            expediteur->channel = NULL;
         } else {
            Channel * channel = getChannel(arg);
            if(channel != NULL && inChannel(expediteur, channel)==1){
               // On enregistre l'utilisateur sur le canal qu'il a choisi
               expediteur->channel = channel;
               // Puis on lui affiche tout l'historique du canal
               char file_name[BUF_SIZE];
               strcpy(file_name, "./Historiques/");
               strcat(file_name, arg);
               strcat(file_name, ".txt");
               FILE * fp = fopen(file_name, "rb");
               if(fp == NULL)
               {
                  printf("Erreur d'ouverture du fichier: ./Historiques/%s.txt", arg);   
                  exit(1);
               }
               fseek(fp, 0, SEEK_END);
               long length = ftell(fp);
               fseek(fp, 0, SEEK_SET);
               char buf[BUF_SIZE]="";
               fread(buf, 1, length, fp);
               write_client(expediteur->sock, buf);
               fclose(fp);
            }else{
               write_client(expediteur->sock, "/!\\ Ce canal n'existe pas ou vous n'y avez pas accès /!\\\n");
            }
         }
      } else if (strcmp(command, "/send")==0){
         Channel * channel = expediteur->channel;
         if(channel != NULL){
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
            for(int i=0; i<channel->nb_recipients; i++){
               if(strcmp(expediteur->name, channel->recipients[i])!=0){
                  Client * destinataire = getClient(channel->recipients[i]);
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

         if(getUser(dest)!=NULL){

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
               savePrivateMessage(private_message);
            }
            free(private_message);

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
         } else {
            write_client(expediteur->sock, "/!\\ Ce destinataire n'existe pas /!\\\n");
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
      } else if (strcmp(command, "/help")==0) {
         write_client(expediteur->sock, help);
      } else {
         write_client(expediteur->sock, "/!\\ Commande erronée /!\\ Utilisez /help vous avoir la liste des commandes possibles.\n");
      }
   }
}

static void print_unread_messages(Client * client){
   char file_name[BUF_SIZE];
   strcpy(file_name, "./Non_lus/");
   strcat(file_name, client->name);
   strcat(file_name, ".txt");
   FILE * fp = fopen(file_name, "rb");
   if(fp==NULL){
      write_client(client->sock, "🔔 Vous n'avez aucun message non lu.\n");
   } else {
      char notif[BUF_SIZE];
      strcpy(notif, "🔔 Vous avez des messages non lus:\n");
      write_client(client->sock, notif);

      fseek(fp, 0, SEEK_END);
      long length = ftell(fp);
      fseek(fp, 0, SEEK_SET);
      char buf[BUF_SIZE]="";
      fread(buf, 1, length, fp);
      write_client(client->sock, buf);
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
   
   int exists = 0;

   FILE * fpm = fopen("./db/private_messages.bin", "rb");
   if(fpm != NULL)
   {
      PrivateMessage * tempPm = (PrivateMessage *)malloc(sizeof(PrivateMessage));
      while(fread(tempPm, sizeof(PrivateMessage), 1, fpm))
      {
         if(inPrivateMessage(client, tempPm)==1){
            exists = 1;
            char channel_name[BUF_SIZE];
            strcpy(channel_name, "🔒 ");
            strcat(channel_name, tempPm->name);
            strcat(channel_name, "\n");
            write_client(client->sock, channel_name);
         }
      }
      free(tempPm);
      fclose(fpm);
   }
   
   FILE * fch = fopen("./db/channels.bin", "rb");
   if(fch != NULL)
   {
      Channel * tempCh = (Channel *)malloc(sizeof(Channel));
      while (fread(tempCh, sizeof(Client), 1, fch))
      {
         if(inChannel(client, tempCh)==1){
            exists = 1;
            char channel_name[BUF_SIZE];
            strcpy(channel_name, "👪 ");
            strcat(channel_name, tempCh->name);
            strcat(channel_name, "\n");
            write_client(client->sock, channel_name);
         }
      }
      free(tempCh);
      fclose(fch);
   }

   if(exists == 0){
      write_client(client->sock, "(vide)\n");
   }
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

         /* after connecting we send to the client message for either sign in or sign up */
         write_client(csock, welcome);

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client * client = (Client *)malloc(sizeof(Client));
         client->sock = csock;
         client->isAuthentified = 0;
         client->channel=NULL;
         clients[nb_clients] = client;
         nb_clients++;
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
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(client, buffer, 1);
                  print_connection_serveur(client, " disconnected !");
                  closesocket(client->sock);
                  remove_client(client, i);
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
      free(clients[i]->channel);
      free(clients[i]);
   }
}

static void remove_client(Client * client, int to_remove)
{
   /* We free the memory space allocated for client */
   free(client);
   /* we remove the client pointer in the array of clients */
   memmove(clients + to_remove, clients + to_remove + 1, (nb_clients - to_remove - 1) * sizeof(Client *));
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

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}