CC = gcc
CFLAGS = -Wall -Wextra -I.

TARGET = chunkly-server
SRC = main.c decompression.c
OBJDIR = build
OBJ = $(SRC:%.c=$(OBJDIR)/%.o)


$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^


$(OBJDIR):
	mkdir -p $(OBJDIR)


$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)
