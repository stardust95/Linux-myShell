OBJECTS = myshell.o utility.o Trie.o List.o Queue.o
CFLAGS = -g -fno-builtin
myshell: $(OBJECTS)
	gcc $(CFLAGS) -o myshell $(OBJECTS)
shell.o: myshell.c Trie.c List.c Queue.c myshell.h
	gcc $(CFLAGS) -c Trie.c List.c Queue.c myshell.c 
utility.o: utility.c myshell.h
	gcc $(CFLAGS) -c utility.c
