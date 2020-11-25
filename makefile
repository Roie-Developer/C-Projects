assambler: firstRead.o secondRead.o helpFunctions.o assembler.o
		gcc -g -ansi -pedantic -Wall firstRead.o secondRead.o helpFunctions.o assembler.o -o assambler
		
firstRead.o: firstRead.c assembler.h
		gcc -c -ansi -pedantic -Wall firstRead.c -o firstRead.o
	
secondRead.o: secondRead.c assembler.h
		gcc -c -ansi -pedantic -Wall secondRead.c -o secondRead.o
		
helpFunctions.o: helpFunctions.c assembler.h
		gcc -c -ansi -pedantic -Wall helpFunctions.c -o helpFunctions.o
		
assembler.o: assembler.c assembler.h
		gcc -c -ansi -pedantic -Wall assembler.c -o assembler.o
		
		