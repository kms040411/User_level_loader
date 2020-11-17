CC = gcc
CFLAGS = -std=gnu99 -g

TARGET = apager dpager apager2

all : $(TARGET)

$(TARGET) : common.o apager.o dpager.o apager2.o
	$(CC) $(CFLAGS) -o apager apager.o common.o
	$(CC) $(CFLAGS) -o dpager dpager.o common.o
	$(CC) $(CFLAGS) -static -o apager2 apager2.o common.o

common.o : common.c
	$(CC) $(CFLAGS) -c -o common.o common.c

apager.o : apager.c
	$(CC) $(CFLAGS) -c -o apager.o apager.c

apager2.o : apager2.c
	$(CC) $(CFLAGS) -c -o apager2.o apager2.c

dpager.o : dpager.c
	$(CC) $(CFLAGS) -c -o dpager.o dpager.c

clean:
	rm *.o $(TARGET)