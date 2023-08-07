all: utclient utserver
client: utclient.c
	gcc utclient.c -o utclient
server: utserver.c
	gcc utserver.c -o utserver
clean:
	$(RM) utclient
	$(RM) utserver