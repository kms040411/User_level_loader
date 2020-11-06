CC = gcc
CFLAGS = -std=gnu99 -static

TARGET = apager

all : $(TARGET)

$(TARGET) : common.o apager.o 
	$(CC) $(CFLAGS) -o apager apager.o common.o

common.o : common.c
	$(CC) $(CFLAGS) -c -o common.o common.c

apager.o : apager.c
	$(CC) $(CFLAGS) -c -o apager.o apager.c

clean:
	rm *.o $(TARGET)