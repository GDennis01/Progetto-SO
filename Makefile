# dal carattere '#' fino a fine riga, il testo dentro il Makefile e`
# un commento
#
# flags per la compilazione
CC = gcc #compiler
CFLAGS = -std=c89 -Wpedantic #compiler flags
TARGET = master
OBJ = Master_Process.c
all: 
	$(CC) $(OBJ) $(CFLAGS) -o $(TARGET)
	./$(TARGET)
clean:
	rm master