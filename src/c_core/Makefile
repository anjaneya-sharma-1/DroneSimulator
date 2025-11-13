CC = gcc
CFLAGS = -Wall -pthread -g
TARGET = drone_scheduler
OBJS = main.o drone_scheduler.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c drone_scheduler.h
	$(CC) $(CFLAGS) -c main.c

drone_scheduler.o: drone_scheduler.c drone_scheduler.h
	$(CC) $(CFLAGS) -c drone_scheduler.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
