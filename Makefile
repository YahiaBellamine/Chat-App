all: 
	$(MAKE) -C Client
	$(MAKE) -C Serveur

clean:
	rm ./Client/*.o
	rm ./Serveur/*.o