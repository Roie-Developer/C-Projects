/*
 This file parses a specific assembly language.
 It saves the data from an assembly file in data structures, and finds the errors.

 *****************************
 ROI TURGEMAN & YOAV SHECHTER
 2017B Open University
 © All Rights Reserved.
 *****************************
 */

#include "assembler.h"

/* Includes  */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

/* Directives List */
void parserDataDirc(lineInfo *line, int *IC, int *DC);
void parserMatDirc(lineInfo *line, int *IC, int *DC);
void parserStringDirc(lineInfo *line, int *IC, int *DC);
void parserExternDirc(lineInfo *line);
void parserEntryDirc(lineInfo *line);
void parseLine(lineInfo *line, char *lineStr, int lineNum, int *IC, int *DC);

const directive g_dircArr[] = { /* Name | Parseing Function */
{ "data", parserDataDirc }, { "string", parserStringDirc }, { "mat",
		parserMatDirc }, { "extern", parserExternDirc }, { "entry",
				parserEntryDirc }, { NULL } /* represent the end of the array */
};

/*  Commands List  */
const command g_cmdArr[] = { /* Name | Opcode | NumOfParams */
{ "mov", 0, 2 }, { "cmp", 1, 2 }, { "add", 2, 2 }, { "sub", 3, 2 }, { "not", 4,
		1 }, { "clr", 5, 1 }, { "lea", 6, 2 }, { "inc", 7, 1 }, { "dec", 8, 1 },
		{ "jmp", 9, 1 }, { "bne", 10, 1 }, { "red", 11, 1 }, { "prn", 12, 1 }, {
				"jsr", 13, 1 }, { "rts", 14, 0 }, { "stop", 15, 0 }, { NULL } /* represent the end of the array */
};

/* Externs */
extern labelInfo g_labelArr[MAX_LABELS_NUM];
extern int g_labelNum;
lineInfo *g_entryLines[MAX_LABELS_NUM];
extern int g_entryLabelsNum;
extern int g_dataArr[MAX_DATA_NUM];

/*  Methods  */

/* Add the label to the labelArr and increase labelNum. Returns pointer to the label in the array. */
labelInfo *addLabelToArray(labelInfo label, lineInfo *line) {
	/* Check if label is legal */
	if (!isLegalLabel(line->lineStr, line->lineNum, TRUE)) {
		/* Illegal label name */
		line->isError = TRUE;
		return NULL;
	}

	/* Check if label is legal */
	if (isExistingLabel(line->lineStr)) {
		printError(line->lineNum, "Label already exists.");
		line->isError = TRUE;
		return NULL;
	}

	/* Adds name to the label */
	strcpy(label.name, line->lineStr);

	/* Add the label to g_labelArr and then to the lineInfo */
	if (g_labelNum < MAX_LABELS_NUM) {
		g_labelArr[g_labelNum] = label;
		return &g_labelArr[g_labelNum++];
	}
	/* Too many labels */
	printError(line->lineNum, "Too many labels - max is %d.", MAX_LABELS_NUM,
	TRUE);
	line->isError = TRUE;
	return NULL;
}

/* Adds the number to the g_dataArr and increase DC. Return if it succeeded. */
bool addNumToData(int num, int *IC, int *DC, int lineNum) {
	/* Check if there is enough space in g_dataArr for the data */
	if (*DC + *IC < MAX_DATA_NUM) {
		g_dataArr[(*DC)++] = num;
	} else {
		return FALSE;
	}

	return TRUE;
}

/* Allocates the memory needed for the matrix and raises the DC accordingly. Returns TRUE if it succeeded. */
bool addMatToData(char *lineStr, int xIndex, int yIndex, int *IC, int *DC) {
	int i;
	char *invalidChar = lineStr;
	/* Check if there is enough space in g_dataArr for the data */
	if (*DC + *IC + xIndex * yIndex < MAX_DATA_NUM) {
		for (i = 0; i < xIndex * yIndex; i++) {
			g_dataArr[(*DC)++] = (int) strtol(invalidChar, &invalidChar, 0);
			if (i != xIndex * yIndex - 1 && *invalidChar == '\0') {
				return FALSE;
			}
			invalidChar++;
		}
		if (*(invalidChar - 1) != '\0') {
			return FALSE;
		}
	} else {
		return FALSE;
	}
	return TRUE;
}

/* Adds the string to the g_dataArr and increase DC. Returns if it succeeded. */
bool addStringToData(char *str, int *IC, int *DC, int lineNum) {
	do {
		if (!addNumToData((int) *str, IC, DC, lineNum)) {
			return FALSE;
		}
	} while (*(str++));

	return TRUE;
}

/* Find the label in line->lineStr and add it to the label list. */
/* Returns pointer to the next char after the label, or NULL if it is not a legal label. */
char *findLabel(lineInfo *line, int IC) {
	char *labelEnd = strchr(line->lineStr, ':');
	labelInfo label = { 0 };
	label.address = FIRST_ADDRESS + IC;

	/* Find the label (or return NULL) */
	if (!labelEnd) {
		return NULL;
	}
	*labelEnd = '\0';

	/* Check if the ':' came after the first word */
	if (!isOneWord(line->lineStr)) {
		*labelEnd = ':'; /* Fix the change in line->lineStr */
		return NULL;
	}

	/* Check if the label is legal and add it to the labelList */
	line->label = addLabelToArray(label, line);

	return labelEnd + 1; /* +1 to make it point at the next char after the \0 */
}

/* Remove the last label in labelArr by updating g_labelNum. */
/* Used to remove the label from a entry / extern line. */
void removeLastLabel(int lineNum) {
	g_labelNum--;
	printf(
			"[Warning] At line %d: The assembler ignored the label before the directive.\n",
			lineNum);
}

/* Parses a .data directive. */
void parserDataDirc(lineInfo *line, int *IC, int *DC) {
	char *operandToken = line->lineStr, *endOfOp = line->lineStr;
	int operandValue;
	bool foundComma;

	/* Make the label a data label (if there is) */
	if (line->label) {
		line->label->isData = TRUE;
		line->label->address = FIRST_ADDRESS + *DC;
	}

	/* Check if there are parameters */
	if (isWhiteSpace(line->lineStr)) {
		/* No parameters */
		printError(line->lineNum, "No parameter.");
		line->isError = TRUE;
		return;
	}
	/* Find all the parameters and add them to g_dataArr */FOREVER {
		/* Get next parameter or break if there is not */
		if (isWhiteSpace(line->lineStr)) {
			break;
		}

		operandToken = getFirstOperand(line->lineStr, &endOfOp, &foundComma);
		/* Add the param to g_dataArr */
		if (isLegalNumber(operandToken, MEMORY_WORD_LENGTH, line->lineNum,
				&operandValue)) {
			if (!addNumToData(operandValue, IC, DC, line->lineNum)) {
				/* Not enough memory */
				line->isError = TRUE;
				return;
			}
		} else {
			/* Illegal number */
			line->isError = TRUE;
			return;
		}
		/* Change the line to start after the parameter */
		line->lineStr = endOfOp;
	}

	if (foundComma) {
		/* Comma after the last parameter */
		printError(line->lineNum,
				"Do not write a comma after the last parameter.");
		line->isError = TRUE;
		return;
	}
}

/* Parses a .mat directive. */
void parserMatDirc(lineInfo *line, int *IC, int *DC) {
	int firstVal = 0, secondVal = 0;
	char *startMat = NULL, *endMat = NULL, *temp = NULL;
	int ret = 0;

	/* Make the label a mat label (if there is'nt one) */
	if (line->label) {
		line->label->isMat = TRUE;
		line->label->address = FIRST_ADDRESS + *DC;
	}

	/* Check if there are params */
	if (isWhiteSpace(line->lineStr)) {
		/* No parameters */
		printError(line->lineNum, "No parameters.");
		line->isError = TRUE;
		return;
	}
	/* Getting the matrix dimensions */
	if ((startMat = strchr(line->lineStr, '[')) == NULL) {
		printError(line->lineNum, "Must be array type [ ][ ].");
		line->isError = TRUE;
		return;
	}
	if ((endMat = strchr(startMat, ']')) != NULL) {
		ret = strtol((++startMat), &temp, 0);
		if (ret == 0 || ret > MAX_DATA_NUM) {
			printError(line->lineNum, "Mat input error");
			line->isError = TRUE;
		}
		firstVal = ret;
	}
	if ((startMat = strchr(endMat, '[')) == NULL) {
		printError(line->lineNum, "Must be array type [ ][ ].");
		line->isError = TRUE;
		return;
	}
	if ((endMat = strchr(startMat, ']')) != NULL) {
		ret = strtol((++startMat), &temp, 0);
		if (ret == 0 || ret > MAX_DATA_NUM) {
			fprintf(stderr, "Mat input error");
			line->isError = TRUE;
		}
		secondVal = ret;
	}
	endMat += 2;

	if (!addMatToData(endMat, firstVal, secondVal, IC, DC)) {
		fprintf(stderr, "Matrix data is bad");
		line->isError = TRUE;
	}

	line->label->matStruct.x = firstVal;
	line->label->matStruct.y = secondVal;
}

/* Parses a .string directive. */
void parserStringDirc(lineInfo *line, int *IC, int *DC) {
	/* Make the label a data label (is there is one) */
	if (line->label) {
		line->label->isData = TRUE;
		line->label->address = FIRST_ADDRESS + *DC;
	}

	trimStr(&line->lineStr);

	if (isLegalStringParam(&line->lineStr, line->lineNum)) {
		if (!addStringToData(line->lineStr, IC, DC, line->lineNum)) {
			/* Not enough memory */
			line->isError = TRUE;
			return;
		}
	} else {
		/* Illegal string */
		line->isError = TRUE;
		return;
	}
}

/* Parses a .extern directive. */
void parserExternDirc(lineInfo *line) {
	labelInfo label = { 0 }, *labelPointer;

	/* If there is a label in the line, remove it from labelArr */
	if (line->label) {
		removeLastLabel(line->lineNum);
	}

	trimStr(&line->lineStr);
	labelPointer = addLabelToArray(label, line);

	/* Make the label an extern label */
	if (!line->isError) {
		labelPointer->address = 0;
		labelPointer->isExtern = TRUE;
	}
}

/* Parses a .entry directive. */
void parserEntryDirc(lineInfo *line) {
	/* If there is a label in the line, remove the it from labelArr */
	if (line->label) {
		removeLastLabel(line->lineNum);
	}

	/* Add the label to the entry label list */
	trimStr(&line->lineStr);

	if (isLegalLabel(line->lineStr, line->lineNum, TRUE)) {
		if (isExistingEntryLabel(line->lineStr)) {
			printError(line->lineNum,
					"Label already defined as an entry label.");
			line->isError = TRUE;
		} else if (g_entryLabelsNum < MAX_LABELS_NUM) {
			g_entryLines[g_entryLabelsNum++] = line;
		}
	}
}

/* Parses the directive and in a directive line. */
void parseDirective(lineInfo *line, int *IC, int *DC) {
	int i = 0;
	while (g_dircArr[i].name) {
		if (!strcmp(line->commandStr, g_dircArr[i].name)) {
			/* Call the parse function for this type of directive */
			g_dircArr[i].parseFunc(line, IC, DC);
			return;
		}
		i++;
	}

	/* line->commandStr is not a real directive */
	printError(line->lineNum, "No such directive as \"%s\".", line->commandStr);
	line->isError = TRUE;
}

/* Returns if the operands types are legal (depending on the command). */
bool areLegalOpTypes(const command *cmd, operandInfo op1, operandInfo op2,
		int lineNum) {
	/* --- Check First Operand --- */
	/* "lea" command (opcode is 6) can only get a label as the 1st op */
	if (cmd->opcode == 6 && op1.type != LABEL) {
		printError(lineNum,
				"Source operand for \"%s\" command must be a label.",
				cmd->name);
		return FALSE;
	}

	/* 2nd operand can be a number only if the command is "cmp" (opcode is 1) or "prn" (opcode is 12).*/
	if (op2.type == NUMBER && cmd->opcode != 1 && cmd->opcode != 12) {
		printError(lineNum,
				"Destination operand for \"%s\" command can't be a number.",
				cmd->name);
		return FALSE;
	}

	return TRUE;
}

/* Updates the type and value of operand. */
void parseOpInfo(operandInfo *operand, int lineNum) {
	int value = 0;

	if (isWhiteSpace(operand->str)) {
		printError(lineNum, "Empty parameter.");
		operand->type = INVALID;
		return;
	}

	/* Check if the type is NUMBER */
	if (*operand->str == '#') {
		operand->str++; /* Remove the '#' */

		/* Check if the number is legal */
		if (isspace(*operand->str)) {
			printError(lineNum, "There is a white space after the '#'.");
			operand->type = INVALID;
		} else {
			operand->type =
					isLegalNumber(operand->str, MEMORY_WORD_LENGTH - 2, lineNum,
							&value) ? NUMBER : INVALID;
		}
	} else if (isMatSyntax(operand)) {
		operand->type = MAT;
	}
	/* Check if the type is REGISTER */
	else if (checkRegisterGetValue(operand->str, &value)) {
		operand->type = REGISTER;
	}
	/* Check if the type is LABEL */
	else if (isLegalLabel(operand->str, lineNum, FALSE)) {
		operand->type = LABEL;
	}
	/* The type is INVALID */
	else {
		printError(lineNum, "\"%s\" is an invalid parameter.", operand->str);
		operand->type = INVALID;
		value = -1;
	}

	operand->value = value;
}

/* Parses the operands in a command line. */
void parseCmdOperands(lineInfo *line, int *IC, int *DC) {
	char *startOfNextPart = line->lineStr;
	bool foundComma = FALSE;
	int numOfOpsFound = 0;

	/* Reset the op types */
	line->op1.type = INVALID;
	line->op2.type = INVALID;

	/* Get the parameters */FOREVER {
		/* If both of the operands are registers, they will only take 1 memory word (instead of 2) */
		if (!(line->op1.type == REGISTER && line->op2.type == REGISTER)) {
			/* Check if there is enough memory */
			if (*IC + *DC < MAX_DATA_NUM) {
				++*IC; /* Count the last command word or operand. */
			} else {
				line->isError = TRUE;
				return;
			}
		}

		/* Check if there are still more operands to read */
		if (isWhiteSpace(line->lineStr) || numOfOpsFound > 2) {
			/* If there are more than 2 operands it it already illegal */
			break;
		}

		/* If there are 2 ops, make the destination become the source op */
		if (numOfOpsFound == 1) {
			line->op1 = line->op2;
			/* Reset op2 */
			line->op2.type = INVALID;
		}

		/* Parse the opernad*/
		line->op2.str = getFirstOperand(line->lineStr, &startOfNextPart,
				&foundComma);
		parseOpInfo(&line->op2, line->lineNum);

		if (line->op2.type == INVALID) {
			line->isError = TRUE;
			return;
		}

		numOfOpsFound++;
		line->lineStr = startOfNextPart;
	} /* End of while */

	/* Check if there are enough operands */
	if (numOfOpsFound != line->cmd->numOfParams) {
		/* There are more/less operands than needed */
		if (numOfOpsFound < line->cmd->numOfParams) {
			printError(line->lineNum, "Not enough operands.", line->commandStr);
		} else {
			printError(line->lineNum, "Too many operands.", line->commandStr);
		}

		line->isError = TRUE;
		return;
	}

	/* Check if there is a comma after the last param */
	if (foundComma) {
		printError(line->lineNum,
				"Don't write a comma after the last parameter.");
		line->isError = TRUE;
		return;
	}
	/* Check if the operands' types are legal */
	if (!areLegalOpTypes(line->cmd, line->op1, line->op2, line->lineNum)) {
		line->isError = TRUE;
		return;
	}
}

/* Parses the command in a command line. */
void parseCommand(lineInfo *line, int *IC, int *DC) {
	int cmdId = getCmdId(line->commandStr);

	if (cmdId == -1) {
		line->cmd = NULL;
		if (*line->commandStr == '\0') {
			/* The command is empty, but the line is not empty so it it only a label. */
			printError(line->lineNum, "Can't write a label to an empty line.",
					line->commandStr);
		} else {
			/* Illegal command. */
			printError(line->lineNum, "No such command as \"%s\".",
					line->commandStr);
		}
		line->isError = TRUE;
		return;
	}

	line->cmd = &g_cmdArr[cmdId];
	parseCmdOperands(line, IC, DC);
}

/* Returns the same string in a different part of the memory by using malloc. */
char *allocString(const char *str) {
	char *newString = (char *) malloc(strlen(str) + 1);
	if (newString) {
		strcpy(newString, str);
	}

	return newString;
}

/* Parses a line, and print errors. */
void parseLine(lineInfo *line, char *lineStr, int lineNum, int *IC, int *DC) {
	char *startOfNextPart = lineStr;

	line->lineNum = lineNum;
	line->address = FIRST_ADDRESS + *IC;
	line->originalString = allocString(lineStr);
	line->lineStr = line->originalString;
	line->isError = FALSE;
	line->label = NULL;
	line->commandStr = NULL;
	line->cmd = NULL;

	if (!line->originalString) {
		printf("[Error] Not enough memory - malloc falied.");
		return;
	}

	/* Check if the line is a comment */
	if (isCommentOrEmpty(line)) {
		return;
	}

	/* Find label and add it to the label list */
	startOfNextPart = findLabel(line, *IC);
	if (line->isError) {
		return;
	}
	/* Update the line if startOfNextPart is not NULL */
	if (startOfNextPart) {
		line->lineStr = startOfNextPart;
	}

	/* Find the command token */
	line->commandStr = getFirstToken(line->lineStr, &startOfNextPart);
	line->lineStr = startOfNextPart;
	/* Parse the command / directive */
	if (isDirective(line->commandStr)) {
		line->commandStr++; /* Remove the '.' from the command */
		parseDirective(line, IC, DC);
	} else {
		parseCommand(line, IC, DC);
	}

	if (line->isError) {
		return;
	}
}

/* Puts a line from 'file' in 'buf'. Returns if the line is shorter than maxLength. */
bool readLine(FILE *file, char *buf, size_t maxLength) {
	char *endOfLine;

	if (!fgets(buf, maxLength, file)) {
		return FALSE;
	}

	/* Check if the line os too long (no '\n' was present). */
	endOfLine = strchr(buf, '\n');
	if (endOfLine) {
		*endOfLine = '\0';
	} else {
		char c;
		bool ret = (feof(file)) ? TRUE : FALSE; /* Return FALSE, unless it it the end of the file */

		/* Keep reading chars until you reach the end of the line ('\n') or EOF */
		do {
			c = fgetc(file);
		} while (c != '\n' && c != EOF);

		return ret;
	}

	return TRUE;
}

/* Reading the file for the first time, line by line, and parsing it. */
/* Returns how many errors were found. */
int firstFileRead(FILE *file, lineInfo *linesArr, int *linesFound, int *IC,
		int *DC) {
	char lineStr[MAX_LINE_LENGTH + 2]; /* +2 for the \n and \0 at the end */
	int errorsFound = 0;

	*linesFound = 0;

	/* Read lines and parse them */
	while (!feof(file)) {
		if (readLine(file, lineStr, MAX_LINE_LENGTH + 2)) {
			/* Check if the file is too lone */
			if (*linesFound >= MAX_LINES_NUM) {
				printf(
						"[Error] File is too long. Max lines number in file is %d.\n",
						MAX_LINES_NUM);
				return ++errorsFound;
			}

			/* Parse a line */
			parseLine(&linesArr[*linesFound], lineStr, *linesFound + 1, IC, DC);

			/* Update errorsFound */
			if (linesArr[*linesFound].isError) {
				errorsFound++;
			}

			/* Check if the number of memory words need is small enough */
			if (*IC + *DC >= MAX_DATA_NUM) {
				/* dataArr is full. Stop reading the file. */
				printError(*linesFound + 1,
						"Too much data and code. Max memory words is %d.",
						MAX_DATA_NUM);
				printf("[Info] Memory is full. Stoping to read the file.\n");
				return ++errorsFound;
			}
			++*linesFound;
		} else if (!feof(file)) {
			/* Line is too long */
			printError(*linesFound + 1,
					"Line is too long. Max line length is %d.",
					MAX_LINE_LENGTH);
			errorsFound++;
			++*linesFound;
		}
	}

	return errorsFound;
}
