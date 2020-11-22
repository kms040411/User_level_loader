CC = gcc
CFLAGS = -std=gnu99 -g

TARGET = apager dpager apager2 backtoback uthread

all : $(TARGET)

$(TARGET) : common.o apager.o dpager.o apager2.o backtoback.o uthread.o
	$(CC) $(CFLAGS) -o apager apager.o common.o
	$(CC) $(CFLAGS) -o dpager dpager.o common.o
	$(CC) $(CFLAGS) -static -o apager2 apager2.o common.o
	$(CC) $(CFLAGS) -static -o backtoback backtoback.o common.o
	$(CC) $(CFLAGS) -static -o utrhead uthread.o common.o

common.o : common.c
	$(CC) $(CFLAGS) -c -o common.o common.c

apager.o : apager.c
	$(CC) $(CFLAGS) -c -o apager.o apager.c

apager2.o : apager2.c
	$(CC) $(CFLAGS) -c -o apager2.o apager2.c

dpager.o : dpager.c
	$(CC) $(CFLAGS) -c -o dpager.o dpager.c

backtoback.o : backtoback.c
	$(CC) $(CFLAGS) -c -o backtoback.o backtoback.c

uthread.o : uthread.c
	$(CC) $(CFLAGS) -c -o uthread.o uthread.c

clean:
	rm *.o $(TARGET)