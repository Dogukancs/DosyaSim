CC      = gcc
CFLAGS  = -Wall -Wextra -g
OBJS    = main.o fs.o

all: simplefs

simplefs: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c fs.h
	$(CC) $(CFLAGS) -c main.c

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) -c fs.c

clean:
	rm -f $(OBJS) simplefs disk.sim fs.log