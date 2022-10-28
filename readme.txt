TODO: Vérification des signatures de commande

Les fonctionalités implémentées:

Une fois qu' un client se connecte, il va voir automatiquement s'il a des messages non lus quand il n'est pas en ligne. Si oui, ces messages sont affichés sur l'écran.

Le client peut créer plusieurs canaux, ajouter des utilisateur dans un canal, ou envoyer un message en public, dans un canal désigné ou en mode message privé. Si un destinaire de message n'est pas en ligne, ce message est stocké et à envoyer quand il se connecte.

TODO: On a implémenté d'autres fonctionalités??

Compilation du programmes:

Entrez dans le dossier ChatApp et tapez make pour tout compiler
Dans Serveur, tapez ./serveur pour lancer le serveur et utilisez control + c 
pour terminer le serveur
Dans Client, tapez ./client 127.0.0.1 [pseudo] pour qu'un client se connecte

Les commandes:

/createchannel [channel_name] 
Cette commande permet de créer un canal de communication, et un utilisateur peut posséder plusieur canaux en mêm

/addrecipient [pseudo]
Cette commande permet d'ajouter d'autres utilisateurs dans un canal (comme discord)

/setchannel [channel_name] 
Cette commande permet de passer au canal que l'utilisateur a choisi,
et d'afficher l'historique du canal

/send [message] 
Cette commande permet d'envoyer un message aux utilisateurs dans un des canaux de l'expéditeur, choisi par /setchannel. Au cas où l'expéditeur n'a créé aucun channel, ce message sera envoyer à tous les utilisateurs.

/sendDM [pseudo]  [message] 
Cette commande permet d'envoyer un message privé à un utilisateur désigné.

TODO: Mets un exemple complet permettant de tester toutes les fonctions au prof