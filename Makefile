# Makefile

CC = gcc
CFLAGS = -Wall

all: scheduler worker client

scheduler: scheduler.c task.h
	$(CC) $(CFLAGS) -o scheduler scheduler.c

worker: worker.c task.h
	$(CC) $(CFLAGS) -o worker worker.c

client: client.c task.h
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f scheduler worker client
