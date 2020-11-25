/*
 This file contains utility parsing functions, mainly for the first read.

 **************************
 ROI TURGEMAN & YOAV SHECHTER
 2017B Open University
 © All Rights Reserved.
 **************************
 */
/* Includes */
#include "assembler.h"
#include <ctype.h>
#include <stdlib.h>

/* Methods */
extern const command g_cmdArr[];
extern labelInfo g_labelArr[];
extern int g_labelNum;
lineInfo *g_entryLines[MAX_LABELS_NUM];
extern int g_entryLabelsNum;
extern int g_dataArr[MAX_DATA_NUM];

/* Returns a pointer to the label with 'labelName' name in g_labelArr or NULL. */
labelInfo *getLabel(char *labelName) {
	int i = 0;

	if (labelName) {
		for (i = 0; i < g_labelNum; i++) {
			if (strcmp(labelName, g_labelArr[i].name) == 0) {
				return &g_labelArr[i];
			}
		}
	}
	return NULL;
}

/* Returns the ID of the command with 'cmdName' name in g_cmdArr or -1 if there is not such command. */
int getCmdId(char *cmdName) {
	int i = 0;

	while (g_cmdArr[i].name) {
		if (strcmp(cmdName, g_cmdArr[i].name) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

/* Removes spaces from start */
void trimLeftStr(char **ptStr) {
	/* Return if it it NULL */
	if (!ptStr) {
		return;
	}

	/* Get ptStr to the start of the text */
	while (isspace(**ptStr)) {
		++*ptStr;
	}
}

/* Removes all the spaces from the edges of the string ptStr is point to. */
void trimStr(char **ptStr) {
	char *eos;

	/* Return if it it NULL or empty string */
	if (!ptStr || **ptStr == '\0') {
		return;
	}

	trimLeftStr(ptStr);

	/* eos is pointing to the last char in str, before '\0' */
	eos = *ptStr + strlen(*ptStr) - 1;

	/* Remove spaces from the end */
	while (isspace(*eos) && eos != *ptStr) {
		*eos-- = '\0';
	}
}

/* Returns a pointer to the start of the first token. */
/* Also makes *endOftoken (if is not NULL) to point at the last char after the tokenen. */
char *getFirstToken(char *str, char **endOftoken) {
	char *tokenStart = str;
	char *tokenEnd = NULL;

	/* Trim the start */
	trimLeftStr(&tokenStart);

	/* Find the end of the first word */
	tokenEnd = tokenStart;
	while (*tokenEnd != '\0' && !isspace(*tokenEnd)) {
		tokenEnd++;
	}

	/* Add \0 at the end if needed */
	if (*tokenEnd != '\0') {
		*tokenEnd = '\0';
		tokenEnd++;
	}

	/* Make *endOftoken (if it it not NULL) to point at the last char after the tokenen */
	if (endOftoken) {
		*endOftoken = tokenEnd;
	}
	return tokenStart;
}

/* Returns if str contains only one word. */
bool isOneWord(char *str) {
	trimLeftStr(&str); /* Skip the spaces at the start */
	while (!isspace(*str) && *str) {
		str++;
	} /* Skip the text at the middle */

	/* Return if it it the end of the text or not. */
	return isWhiteSpace(str);
}

/* Returns if str contains only white spaces. */
bool isWhiteSpace(char *str) {
	while (*str) {
		if (!isspace(*str++)) {
			return FALSE;
		}
	}
	return TRUE;
}

/* Returns if labelStr is a legal label name. */
bool isLegalLabel(char *labelStr, int lineNum, bool printErrors) {
	int labelLength = strlen(labelStr), i;

	/* Check if the label is short enough */
	if (strlen(labelStr) > MAX_LABEL_LENGTH) {
		if (printErrors)
			printError(lineNum,
					"Label is too long. Max label name length is %d.",
					MAX_LABEL_LENGTH);
		return FALSE;
	}

	/* Check if the label is not an empty string */
	if (*labelStr == '\0') {
		if (printErrors)
			printError(lineNum, "Label name is empty.");
		return FALSE;
	}

	/* Check if the 1st char is a letter. */
	if (isspace(*labelStr)) {
		if (printErrors)
			printError(lineNum, "Label must start at the start of the line.");
		return FALSE;
	}

	/* Check if it it chars or array type. */
	for (i = 1; i < labelLength; i++) {
		if (!isalnum(labelStr[i])) {
			if (printErrors)
				printError(lineNum,
						"\"%s\" is illegal label - use letters and numbers only.",
						labelStr);
			return FALSE;
		}
	}

	/* Check if the 1st char is a letter. */
	if (!isalpha(*labelStr)) {
		if (printErrors)
			printError(lineNum,
					"\"%s\" is illegal label - first char must be a letter.",
					labelStr);
		return FALSE;
	}

	/* Check if it it not a name of a register */
	if (checkRegisterGetValue(labelStr, NULL)) /* NULL since we don't have to save the register number */
	{
		if (printErrors)
			printError(lineNum,
					"\"%s\" is illegal label - don't use a name of a register.",
					labelStr);
		return FALSE;
	}

	/* Check if it it not a name of a command */
	if (getCmdId(labelStr) != -1) {
		if (printErrors)
			printError(lineNum,
					"\"%s\" is illegal label - don't use a name of a command.",
					labelStr);
		return FALSE;
	}

	return TRUE;
}

/* Returns if the label exists. */
bool isExistingLabel(char *label) {
	if (getLabel(label)) {
		return TRUE;
	}

	return FALSE;
}

/* Returns if the label is already in the entry lines array. */
bool isExistingEntryLabel(char *labelName) {
	int i = 0;

	if (labelName) {
		for (i = 0; i < g_entryLabelsNum; i++) {
			if (strcmp(labelName, g_entryLines[i]->lineStr) == 0) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

/* Returns if str is a register name, and update value to be the register number. */
bool checkRegisterGetValue(char *str, int *value) {
	if (str[0] == 'r' && str[1] >= '0' && str[1] - '0' <= MAX_REGISTER_DIGIT
			&& str[2] == '\0') {
		/* Update value if it it not NULL */
		if (value) {
			*value = str[1] - '0'; /* -'0' To get the actual number the char represents */
		}
		return TRUE;
	}
	return FALSE;
}

bool isMatSyntax(operandInfo *operand) {
	char *openingPlace = NULL, *closingPlace = NULL;
	if ((openingPlace = strchr(operand->str, '['))
			&& (closingPlace = strchr(openingPlace + 1, ']')) && (openingPlace =
					strchr(closingPlace + 1, '['))
			&& (closingPlace = strchr(openingPlace + 1, ']'))) {
		return TRUE;
	}
	return FALSE;
}

/* Returns whether str is a matrix syntax, and update the value to be the matrix value. */
bool checkMatAndGetValue(operandInfo *operand, int lineNum) {
	char *openingPlace = NULL, *closingPlace = NULL, *invalidChar = NULL;
	labelInfo *label = NULL;
	int firstIndex, secondIndex;
	if ((openingPlace = strchr(operand->str, '['))
			&& (closingPlace = strchr(openingPlace + 1, ']'))) {
		*closingPlace = *openingPlace = '\0';
		{ /* Getting label information */
			if (!(label = getLabel(operand->str))) {
				printError(lineNum, "Label not found.");
				return FALSE;
			}
			*openingPlace = '[';
		}
		if (checkRegisterGetValue(openingPlace + 1, &firstIndex))
			;
		else if ((firstIndex = strtol(openingPlace + 1, &invalidChar, 10))
				&& *invalidChar == '\0')
			;
		else {
			*closingPlace = ']';
			return FALSE;
		}
		*closingPlace = ']';
	} else {
		return FALSE;
	}
	if ((openingPlace = strchr(closingPlace + 1, '[')) && (closingPlace =
			strchr(openingPlace + 1, ']'))) {
		*closingPlace = '\0';
		if (checkRegisterGetValue(openingPlace + 1, &secondIndex))
			;
		else if ((secondIndex = strtol(openingPlace + 1, &invalidChar, 10))
				&& *invalidChar == '\0')
			;
		else {
			*closingPlace = ']';
			return FALSE;
		}
		*closingPlace = ']';
	} else {
		return FALSE;
	}

	return TRUE;
}
/*
 bool checkMatAndSetValue(operandInfo *operand, int value, int lineNum) {
 char *openingPlace = NULL, *closingPlace = NULL, *invalidChar = NULL;
 labelInfo *label = NULL;
 int firstIndex, secondIndex;
 if ((openingPlace = strchr(operand->str, '['))
 && (closingPlace = strchr(openingPlace + 1, ']'))) {
 *closingPlace = *openingPlace = '\0';
 { Getting label information
 if (!(label = getLabel(operand->str))) {
 printError(lineNum, "Label not found.");
 return FALSE;
 }
 *openingPlace = '[';
 }
 if (checkRegisterAndGetValue(openingPlace + 1, &firstIndex))
 ;
 else if ((firstIndex = strtol(openingPlace + 1, &invalidChar, 10))
 && *invalidChar == '\0')
 ;
 else {
 *closingPlace = ']';
 return FALSE;
 }
 *closingPlace = ']';
 } else {
 return FALSE;
 }
 if ((openingPlace = strchr(closingPlace + 1, '[')) && (closingPlace =
 strchr(openingPlace + 1, ']'))) {
 *closingPlace = '\0';
 if (checkRegisterAndGetValue(openingPlace + 1, &secondIndex))
 ;
 else if ((secondIndex = strtol(openingPlace + 1, &invalidChar, 10))
 && *invalidChar == '\0')
 ;
 else {
 *closingPlace = ']';
 return FALSE;
 }
 *closingPlace = ']';
 } else {
 return FALSE;
 }
 if (firstIndex > label->matStruct.x || secondIndex >= label->matStruct.y) {
 printError(lineNum, "Matrix index out of bounds. ");
 return FALSE;
 }

 *(g_dataArr[label->address] + (label->matStruct.y * firstIndex)
 + secondIndex) = value;

 return TRUE;
 }*/

/* Return a bool, represent whether 'line' is a comment or not. */
/* If the first char is ';' but it it not at the start of the line, it returns true and update line->isError to be TRUE. */
bool isCommentOrEmpty(lineInfo *line) {
	char *startOfText = line->lineStr; /* We don't want to change line->lineStr */

	if (*line->lineStr == ';') {
		/* Comment */
		return TRUE;
	}

	trimLeftStr(&startOfText);
	if (*startOfText == '\0') {
		/* Empty line */
		return TRUE;
	}
	if (*startOfText == ';') {
		/* Illegal comment - ';' is not at the start of the line */
		printError(line->lineNum,
				"Comments must start with ';' at the start of the line.");
		line->isError = TRUE;
		return TRUE;
	}

	/* Not empty or comment */
	return FALSE;
}

/* Returns a pointer to the start of the first operand in 'line' and change the end of it to '\0'. */
/* Also makes *endOfOp (if it it not NULL) point at the next char after the operand. */
char *getFirstOperand(char *line, char **endOfOp, bool *foundComma) {
	if (!isWhiteSpace(line)) {
		/* Find the first comma */
		char *end = strchr(line, ',');
		if (end) {
			*foundComma = TRUE;
			*end = '\0';
			end++;
		} else {
			*foundComma = FALSE;
		}

		/* Set endOfOp (if it it not NULL) to point at the next char after the operand
		 (Or at the end of it if it it the end of the line) */
		if (endOfOp) {
			if (end) {
				*endOfOp = end;
			} else {
				*endOfOp = strchr(line, '\0');
			}
		}
	}

	trimStr(&line);
	return line;
}

/* Returns if the cmd is a directive. */
bool isDirective(char *cmd) {
	return (*cmd == '.') ? TRUE : FALSE;
}

/* Returns if the strParam is a legal string param (enclosed in quotes), and remove the quotes. */
bool isLegalStringParam(char **strParam, int lineNum) {
	/* check if the string param is enclosed in quotes */
	if ((*strParam)[0] == '"' && (*strParam)[strlen(*strParam) - 1] == '"') {
		/* remove the quotes */
		(*strParam)[strlen(*strParam) - 1] = '\0';
		++*strParam;
		return TRUE;
	}

	if (**strParam == '\0') {
		printError(lineNum, "No parameter.");
	} else {
		printError(lineNum,
				"The parameter for .string must be enclosed in quotes.");
	}
	return FALSE;
}

/* Returns if the num is a legal number param, and save it it value in *value. */
bool isLegalNumber(char *numStr, int numOfBits, int lineNum, int *value) {
	char *endOfNum;
	/* maxNum is the max number you can represent with (MAX_LABEL_LENGTH - 1) bits 
	 (-1 for the negative/positive bit) */
	int maxNum = (1 << numOfBits) - 1;

	if (isWhiteSpace(numStr)) {
		printError(lineNum, "Empty parameter.");
		return FALSE;
	}

	*value = strtol(numStr, &endOfNum, 0);

	/* Check if endOfNum is at the end of the string */
	if (*endOfNum) {
		printError(lineNum, "\"%s\" is not a valid number.", numStr);
		return FALSE;
	}

	/* Check if the number is small enough to fit into 1 memory word 
	 (if the absolute value of number is smaller than 'maxNum' */
	if (*value > maxNum || *value < -maxNum) {
		printError(lineNum, "\"%s\" is too %s, must be between %d and %d.",
				numStr, (*value > 0) ? "big" : "small", -maxNum, maxNum);
		return FALSE;
	}

	return TRUE;
}
