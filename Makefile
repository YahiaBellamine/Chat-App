all: 
	$(MAKE) -C Client
	$(MAKE) -C Serveur

clean:
	rm ./Client/*.o ./Client/client
	rm ./Serveur/*.o ./Serveur/server
	rm ./Serveur/db/*.bin
	rm ./Serveur/Historiques/*.txt
	rm ./Serveur/Non_lus/*.txt