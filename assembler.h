/*
 ***************************
 ROI TURGEMAN & YOAV SHECHTER
 2017B Open University
 © All Rights Reserved.
 ***************************
 Header file of the assembler:
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <stdio.h>
#include <string.h>

/* Macros */
#define FOREVER				for(;;)
#define BYTE_SIZE			8
#define FALSE				0
#define TRUE				1

/* Given Constants */
#define MAX_DATA_NUM		1000
#define MAX_MAT_NUM			100
#define MAX_LINE_LENGTH		80
#define MAX_LABEL_LENGTH	30
#define FIRST_ADDRESS		100
#define MEMORY_WORD_LENGTH	10
#define MAX_REGISTER_DIGIT	7

/* Defining Constants */
#define MAX_LINES_NUM		700
#define MAX_LABELS_NUM		MAX_LINES_NUM 

/* Data Structures */
typedef unsigned int bool; /* Get TRUE or FALSE */

/* First Read */

/* Labels Management */
typedef struct {
	int address; /* The address it contains */
	char name[MAX_LABEL_LENGTH]; /* The name of the label */
	bool isExtern; /* Extern flag */
	bool isData; /* Data flag  */
	bool isMat; /* Mat flag  */
	struct {
		int x;
		int y;
	} matStruct;
} labelInfo;

/* Directive And Commands */
typedef struct {
	char *name;
	void (*parseFunc)();
} directive;

typedef struct {
	char *name;
	unsigned int opcode :4;
	int numOfParams;
} command;

/* Operands */
typedef enum {
	NUMBER, LABEL, MAT, REGISTER, INVALID = -1
} opType;

typedef struct {
	int value; /* Value */
	char *str; /* String */
	opType type; /* Type */
	int address; /* The adress of the operand in the memory */
} operandInfo;

/* Line */
typedef struct {
	int lineNum; /* The number of the line in the file */
	int address; /* The address of the first word in the line */
	char *originalString; /* The original pointer, allocated by malloc */
	char *lineStr; /* The text it contains (changed while using parseLine) */
	bool isError; /* Represent whether there is an error or not */
	labelInfo *label; /* A poniter to the lines label in labelArr */

	char *commandStr; /* The string of the command or directive */

	/* Command line */
	const command *cmd; /* A pointer to the command in g_cmdArr */
	operandInfo op1; /* The 1st operand */
	operandInfo op2; /* The 2nd operand */
} lineInfo;

/* Second Read */
typedef enum {
	ABSOLUTE = 0, EXTENAL = 1, RELOCATABLE = 2
} eraType;


/* Memory Word */
typedef struct /* 10 bits */
{
	unsigned int era :2;

	union /* 8 bits */
	{
		/* Commands (only 8 bits) */
		struct {
			unsigned int dest :2; /* Destination op addressing */
			unsigned int src :2; /* Source op addressing */
			unsigned int opcode :4; /* Command */

		} cmdBits;

		/* Registers (only 8 bits) */
		struct {
			unsigned int destBits :4;
			unsigned int srcBits :4;
		} regBits;

		/* Other operands */
		int value :10; /* (10 bits) */

	} valueBits; /* End of 10 bits union */

} memoryWord;

/* Methods Declaration */
int getCmdId(char *cmdName);
labelInfo *getLabel(char *labelName);
void trimLeftStr(char **ptStr);
void trimStr(char **ptStr);
char *getFirstToken(char *str, char **endOfToken);
bool isWhiteSpace(char *str);
bool isOneWord(char *str);
bool isLegalLabel(char *label, int lineNum, bool printErrors);
bool isExistingLabel(char *label);
bool isExistingEntryLabel(char *labelName);
bool checkRegisterGetValue(char *str, int *value);
char *getFirstOperand(char *line, char **endOfOp, bool *foundComma);
bool checkMatAndGetValue(operandInfo *operand, int lineNum);
bool isCommentOrEmpty(lineInfo *line);
bool isDirective(char *cmd);
bool isLegalStringParam(char **strParam, int lineNum);
bool isLegalNumber(char *numStr, int numOfBits, int lineNum, int *value);
bool isMatSyntax(operandInfo *operand);

/* firstRead.c methods */
int firstFileRead(FILE *fd, lineInfo *linesArr, int *linesFound, int *IC,
		int *DC);

/* secondRead.c methods */
int secondFileRead(int *memoryArr, lineInfo *linesArr, int lineNum, int IC,
		int DC);

/* main.c methods */
void printError(int lineNum, const char *format, ...);

#endif
