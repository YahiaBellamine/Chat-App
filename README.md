# ChatApp
TP3 &amp; 4 Programmation Réseaux

Les fonctionalités implémentées:

/signup [pseudo] [mdp] 
Cette commande permet à l'utilisateur de s'inscrire

/signin [pseudo] [mdp] 
Cette commande permet à l'utilisateur de se connecter

/private [pseudo] [message] 
Cette commande permet d'envoyer un message privé à un utilisateur existant.

/create [channel_name] 
Cette commande permet de créer un canal de communication (groupe), et un utilisateur peut en créer plusieurs (selon la valeur du max défini).

/add [pseudo]
Cette commande permet d'ajouter d'autres utilisateurs dans le canal rejoint par la commande /join.

/join [channel_name] 
Cette commande permet de passer au canal que l'utilisateur a choisi, et d'afficher l'historique du canal.

/send [message] 
Cette commande permet d'envoyer un message aux utilisateurs dans un des canaux de l'expéditeur, choisi par /join. 
Au cas où l'expéditeur n'a pas rejoint de canal, ce message sera envoyer à tous les utilisateurs (il est sur le canal public par défault).

/which
Cette commande permet de savoir sur quel canal l'utilisateur se trouve.

/help 
Pour avoir la liste des commandes possibles.

Mis à part les fonctionalités décrites par les commandes ci-dessus, le client reçoit les messages non lus (lorsqu'il était déconnecté) une fois 
qu'il se reconnecte, et peut aussi voir l'historique de la conversation d'un canal dès qu'il le rejoint.

L'ensemble des données (utilisateurs, caneaux, messages privé) sont stockés sous le format de fichiers binaires dans le dossier ./db
Les messages nos lus ainsi que l'historique des caneaux sont stockés sous le format de fichiers textuels respectivement dans les dossiers ./Non_lus et ./Historiques

Compilation du programme:

Pour tout compiler tapez make, et make clean pour effacer les fichiers objet, les fichiers binaires et les exécutables.
Dans le dossier Serveur, tapez ./serveur pour lancer le serveur et utilisez ctrl + C pour l'arrêter
Dans le dossier Client, tapez ./client 127.0.0.1 pour qu'un client se connecte et ctrl + C pour qu'il se déconnecte


Binome : Yahia BELLAMINE & Cen LU
Hexanome : H4142