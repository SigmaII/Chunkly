CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I.

TARGET = chunkly
SRC = main.c compression.c socket_client.c
OBJ = $(SRC:.c=.o)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(TARGET)
