# dal carattere '#' fino a fine riga, il testo dentro il Makefile e`
# un commento
#
# flags per la compilazione
CC = gcc #compiler
CFLAGS = -std=c89 -Wpedantic #compiler flags
TARGET = master
OBJ = Master_Process.c
User_Dep = User_Process.c #dependecies
Node_Dep = Node_Process.c #dependecies
Target_User = User 
Target_Node = Node
all: User Node
	$(CC) $(OBJ) $(CFLAGS) -o $(TARGET)
run: $(TARGET)
	./$(TARGET)
User:
	$(CC) $(User_Dep) $(CFLAGS) -o $(Target_User)
Node:
	$(CC) $(Node_Dep) $(CFLAGS) -o $(Target_Node)
clean:
	rm master
	rm User
	rm Node