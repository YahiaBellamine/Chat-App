all: 
	$(MAKE) -C Client
	$(MAKE) -C Serveur

clean:
	rm ./Client/*.o ./Client/client
	rm ./Serveur/*.o ./Serveur/serveur