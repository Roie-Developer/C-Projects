/*
 The main file.
 This file manages the assembling process.
 It calls the first and second read methods, and then creates the output files.

 ******************************
 ROI TURGEMAN & YOAV SHECHTER
 2017B Open University
 © All Rights Reserved.
 ******************************
 */

/* ******** Includes ******** */
#include "assembler.h"

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

/* ******** Global Data Structures ******** */
/* Labels */
labelInfo g_labelArr[MAX_LABELS_NUM];
int g_labelNum = 0;
/* Entry Lines */
lineInfo *g_entryLines[MAX_LABELS_NUM]; /**/
int g_entryLabelsNum = 0;
/* Data */
int g_dataArr[MAX_DATA_NUM];

/* ******** Methods ******** */

/* Print an error with the line number. */
void printError(int lineNum, const char *format, ...) {
	va_list args;
	va_start(args, format);
	printf("[Error] At line %d: ", lineNum);
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

int intToBase4(int num, char *buf, int index) {
	const int base = 4;
	const char digits[] = "0123";

	if (num) {
		index = intToBase4(num / base, buf, index);
		buf[index] = digits[num % base] - '0' + 'a';
		return ++index;
	}

	return 0;
}

void printStrWithZeros(char *str, int strMinWidth) {
	int i, numOfZeros = strMinWidth - strlen(str);

	for (i = 0; i < numOfZeros; i++) {
		printf("0");
	}
	printf("%s", str);
}

void fprintfBase4(FILE *file, int num, int strMinWidth) {
	int numOfZeros, i;
	char buf[4] = { 0 };

	intToBase4(num, buf, 0);

	/* Add zeros first, to make the length at least strMinWidth */
	numOfZeros = strMinWidth - strlen(buf);
	for (i = 0; i < numOfZeros; i++) {
		fprintf(file, "a");
	}
	fprintf(file, "%s", buf);
}

/* Creates a file (for writing) from a given name and ending, and returns a pointer to it. */
FILE *openFile(char *name, char *ending, const char *mode) {
	FILE *fd;
	char *mallocStr = (char *) malloc(strlen(name) + strlen(ending) + 1),
			*fileName = mallocStr;
	sprintf(fileName, "%s%s", name, ending);

	fd = fopen(fileName, mode);
	free(mallocStr);

	return fd;
}

/* Creates the .obj file, which contains the assembled lines in base 32. */
void createObjectFile(char *name, int IC, int DC, int *memoryArr) {
	int i;
	FILE *fd;
	fd = openFile(name, ".ob", "w");

	/* Print IC and DC */
	fprintfBase4(fd, IC, 1);
	fprintf(fd, "\t\t");
	fprintfBase4(fd, DC, 1);

	/* Print all of memoryArr */
	for (i = 0; i < IC + DC; i++) {
		fprintf(fd, "\n");
		fprintfBase4(fd, FIRST_ADDRESS + i, 3);
		fprintf(fd, "\t\t");
		fprintfBase4(fd, memoryArr[i], 3);
	}

	fclose(fd);
}

/* Creates the .ent file, which contains the addresses for the .entry labels in base 32. */
void createEntriesFile(char *name) {
	int i;
	FILE *fd;

	/* Don't create the entries file if there aren't entry lines */
	if (!g_entryLabelsNum) {
		return;
	}

	fd = openFile(name, ".ent", "w");

	for (i = 0; i < g_entryLabelsNum; i++) {
		fprintf(fd, "%s\t\t", g_entryLines[i]->lineStr);
		fprintfBase4(fd, getLabel(g_entryLines[i]->lineStr)->address, 1);

		if (i != g_entryLabelsNum - 1) {
			fprintf(fd, "\n");
		}
	}

	fclose(fd);
}

/* Creates the .ext file, which contains the addresses for the extern labels operands in base 32. */
void createExternFile(char *name, lineInfo *linesArr, int linesFound) {
	int i;
	labelInfo *label;
	bool firstPrint = TRUE; /* This bool meant to prevent the creation of the file if there aren't any externs */
	FILE *fd = NULL;

	for (i = 0; i < linesFound; i++) {
		/* Check if the 1st operand is extern label, and print it. */
		if (linesArr[i].cmd && linesArr[i].cmd->numOfParams >= 2
				&& linesArr[i].op1.type == LABEL) {
			label = getLabel(linesArr[i].op1.str);
			if (label && label->isExtern) {
				if (firstPrint) {
					/* Create the file only if there is at least 1 extern */
					fd = openFile(name, ".ext", "w");
				} else {
					fprintf(fd, "\n");
				}

				fprintf(fd, "%s\t\t", label->name);
				fprintfBase4(fd, linesArr[i].op1.address, 1);
				firstPrint = FALSE;
			}
		}

		/* Check if the 2nd operand is extern label, and print it. */
		if (linesArr[i].cmd && linesArr[i].cmd->numOfParams >= 1
				&& linesArr[i].op2.type == LABEL) {
			label = getLabel(linesArr[i].op2.str);
			if (label && label->isExtern) {
				if (firstPrint) {
					/* Create the file only if there is at least 1 extern */
					fd = openFile(name, ".ext", "w");
				} else {
					fprintf(fd, "\n");
				}

				fprintf(fd, "%s\t\t", label->name);
				fprintfBase4(fd, linesArr[i].op2.address, 1);
				firstPrint = FALSE;
			}
		}
	}

	if (fd) {
		fclose(fd);
	}
}

/* Resets all the globals and free all the malloc blocks. */
void clearData(lineInfo *linesArr, int linesFound, int dataCount) {
	int i;

	/* --- Reset Globals --- */

	/* Reset global labels */
	for (i = 0; i < g_labelNum; i++) {
		g_labelArr[i].address = 0;
		g_labelArr[i].isData = 0;
		g_labelArr[i].isExtern = 0;
	}
	g_labelNum = 0;

	/* Reset global entry lines */
	for (i = 0; i < g_entryLabelsNum; i++) {
		g_entryLines[i] = NULL;
	}
	g_entryLabelsNum = 0;

	/* Reset global data */
	for (i = 0; i < dataCount; i++) {
		g_dataArr[i] = 0;
	}

	/* Free malloc blocks */
	for (i = 0; i < linesFound; i++) {
		free(linesArr[i].originalString);
	}
}

/* Parsing a file, and creating the output files. */
void parseFile(char *fileName) {
	FILE *fd = openFile(fileName, ".as", "r");
	lineInfo linesArr[MAX_LINES_NUM];
	int memoryArr[MAX_DATA_NUM] = { 0 }, IC = 0, DC = 0, numOfErrors = 0,
			linesFound = 0;

	/* Open File */
	if (fd == NULL) {
		printf("[Info] Can't open the file \"%s.as\".\n", fileName);
		return;
	}
	printf("[Info] Successfully opened the file \"%s.as\".\n", fileName);

	/* First Read */
	numOfErrors += firstFileRead(fd, linesArr, &linesFound, &IC, &DC);
	/* Second Read */
	numOfErrors += secondFileRead(memoryArr, linesArr, linesFound, IC, DC);

	/* Create Output Files */
	if (numOfErrors == 0) {
		/* Create all the output files */
		createObjectFile(fileName, IC, DC, memoryArr);
		createExternFile(fileName, linesArr, linesFound);
		createEntriesFile(fileName);
		printf("[Info] Created output files for the file \"%s.as\".\n",
				fileName);
	} else {
		/* print the number of errors. */
		printf("[Info] A total of %d error%s found throughout \"%s.as\".\n",
				numOfErrors, (numOfErrors > 1) ? "s were" : " was", fileName);
	}

	/* Free all malloc pointers, and reset the globals. */
	clearData(linesArr, linesFound, IC + DC);

	/* Close File */
	fclose(fd);
}

/* Main method. Calls the "parsefile" method for each file name in argv. */
int main(int argc, char *argv[]) {
	int i;

	if (argc < 2) {
		printf("[Info] no file names were observed.\n");
		return 1;
	}

	for (i = 1; i < argc; i++) {
		parseFile(argv[i]);
		printf("\n");
	}

	return 0;
}
