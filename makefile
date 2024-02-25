# Makefile for client and server

CC = gcc
OBJCS = client0.c
OBJCSS = server0.c

CFLAGS =  -g -Wall
# setup for system
nLIBS =

all: client server

client: $(OBJCS)
	$(CC) $(CFLAGS) -o $@ $(OBJCS) $(LIBS)

server: $(OBJCSS)
	$(CC) $(CFLAGS) -o $@ $(OBJCSS) $(LIBS)

clean:
	rm client server