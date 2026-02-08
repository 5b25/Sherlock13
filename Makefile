
CC = gcc
CFLAGS = -Wall -g -I./include $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2 -lSDL2_image -lSDL2_ttf -lpthread

SRC_SERVER = src/main_server.c src/server_logic.c src/common.c
SRC_CLIENT = src/main_client.c src/client_logic.c src/gui.c src/resources.c src/common.c

OBJ_SERVER = $(SRC_SERVER:.c=.o)
OBJ_CLIENT = $(SRC_CLIENT:.c=.o)

all: serveur client

serveur: $(OBJ_SERVER)
	$(CC) -o $@ $^ $(LDFLAGS)

client: $(OBJ_CLIENT)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f src/*.o serveur client
