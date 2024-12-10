all: mypipeline myshell

mypipeline: mypipeline.o
	gcc -m32 -g -Wall -o mypipeline mypipeline.o

mypipeline.o: mypipeline.c
	gcc -m32 -g -Wall -c -o mypipeline.o mypipeline.c

myshell: myshell.o LineParser.o
	gcc -m32 -g -Wall -o myshell myshell.o LineParser.o

myshell.o: myshell.c
	gcc -m32 -g -Wall -c myshell.c

LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c LineParser.c

.PHONY: clean

clean:
	rm -f *.o mypipeline myshell
