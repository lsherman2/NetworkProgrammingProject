# Makefile for drone

CC = gcc
OBJCS = drone.c

CFLAGS =  -g -Wall
# setup for system
nLIBS =

all: drone

drone: $(OBJCS)
	$(CC) $(CFLAGS) -o $@ $(OBJCS) $(LIBS)

clean:
	rm -f drone