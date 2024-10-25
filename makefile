CC = gcc
CFLAGS = -Wall -g
LIBS = -lsystemd
TARGET = main.out
SRCS = main.c
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LIBS)
clean:
	rm -f $(TARGET)