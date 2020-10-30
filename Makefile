CC = gcc
CFLAGS = -std=c11 -g

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