CC = gcc
C_FLAGS = -Wall
C_SERVER  = crc8.c udpserver.c soldier.c utils.c
C_CLIENT = crc8.c udpclient.c utils.c conf.c
EXE_SERVER = server
EXE_CLIENT = client 

all:

server:
	$(CC) $(C_FLAGS) $(C_SERVER) -o $(EXE_SERVER) -l pthread
client:
	$(CC) $(C_FLAGS) $(C_CLIENT) -o $(EXE_CLIENT) -l pthread

clean:
	rm -rf *~ *.o $(EXE_SERVER) $(EXE_CLIENT)
