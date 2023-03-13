#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
char BASE62 [62] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

// When smallVarNames is true, every variable name will be mapped to a hexadecimal number as a string
// This is because small variable names can be processed faster by the interpreter

struct varNode{
    char * name;
    int num;
    struct varNode * next;
};

// Converts a number to a base 62 string, to minimize the number of characters
char * numToBase62(int num){
    if (num == 0) return "0";
    char * base62 = malloc(10);
    int i = 0;
    while (num > 0){
        base62[i] = BASE62[num % 62];
        num /= 62;
        i++;
    }
    base62[i] = '\0';
    return base62;
}

// Returns the node with the given name, or NULL if it is not found
struct varNode * findValue(struct varNode * varList, char * name){
    struct varNode * tempNode = varList;
    while (tempNode != NULL){
        if (strcmp(tempNode->name, name) == 0){
            return tempNode;
        }
        tempNode = tempNode->next;
    }
    return NULL;
}

// Returns the last node in the linked list
struct varNode * getLastNode(struct varNode * varList){
    struct varNode * tempNode = varList;
    while (tempNode->next != NULL){
        tempNode = tempNode->next;
    }
    return tempNode;
}

// Returns the short name of the variable using the list of variables, also updates the list if the variable is not in the list
char * getShortName(struct varNode * varList, char * name){
    // Creating a new node
    struct varNode * newNode = malloc(sizeof(struct varNode));
    newNode->name = malloc(strlen(name) + 1);
    strcpy(newNode->name, name);

    // Checking if this variable name is already in the list
    struct varNode * tempNode = findValue(varList, name);
    if (tempNode != NULL){ // If the variable name is already in the list
        return numToBase62(tempNode->num); // Returning the short name of the variable
    } else {
        newNode->next = NULL;
        // Setting the number of the node
        newNode->num = getLastNode(varList)->num + 1;
        // Adding the node to end of the linked list
        getLastNode(varList)->next = newNode;

        // Returning the short name of the variable
        return numToBase62(newNode->num);
    }
}

bool makeRawStackFile(char * fileName, bool smallVarNames){
	FILE * stackFile = fopen(fileName, "r");
	if (stackFile == NULL){
		printf("Could not find file \" %s \"", fileName);
		return false;
	}

    struct varNode * varList = malloc(sizeof(struct varNode));
    varList->next = NULL;
    varList->num = 0;
    varList->name = malloc(1);
    varList->name[0] = '\0';

	char rawFileName [strlen(fileName) + 5];
	strcpy(rawFileName, fileName);
	int i;	
	for (i = 0; i < strlen(rawFileName) && rawFileName[i] != '.'; i++);
	
	rawFileName[i] = '\0';
	strcat(rawFileName, ".ptas");

	FILE * rawStackFile = fopen(rawFileName, "w");
	char tempChar;
	int inComment = 0;

    bool inVar = false;
    int varCount = 0;

    char tempVarName [50];
    int tempVarLength = 0;

    char lastCharAdded = '\0';
    char lastTileTypeAdded = '\0';

	while (!feof(stackFile)){
		tempChar = fgetc(stackFile);
		if (tempChar != EOF){
			if (tempChar == '#'){
				inComment = 1;
			} else if (tempChar == '\n'){
				inComment = 0;
				tempChar = '_'; // Replacing the newline with a blocker
			}
			if (tempChar != ' ' && !inComment){
                if (isalnum(tempChar) && smallVarNames && lastTileTypeAdded != '&'){
                    // This character is part of a variable name, either starting a name or adding to it
                    if (!inVar){ // This is the start of a variable name
                        inVar = true;
                        tempVarLength = 0;
                        tempVarName[0] = '\0'; // Adding the null terminator to reset the string
                        varCount++;
                    }
                    // Adding this character to the tempVarName
                    tempVarName[tempVarLength] = tempChar;
                    tempVarName[tempVarLength + 1] = '\0';
                    tempVarLength++;
                } else { // This character is not part of a variable name because it is not alphanumeric
                    if (inVar){
                        inVar = false;
                        tempVarName[tempVarLength + 1] = '\0'; // Adding the null terminator
                        char * shortName = getShortName(varList, tempVarName);
                        fputs(shortName, rawStackFile);
                        if (tempChar != '_' || lastCharAdded != '_') {
                            fputc(tempChar, rawStackFile); // Putting the non-alphanumeric character in the file
                            lastTileTypeAdded = tempChar;
                            lastCharAdded = tempChar;
                        }
                    } else {
                        // This character is not part of a variable name
                        // We will just print it to the file
                        if (tempChar != '_' || lastCharAdded != '_') {
                            fputc(tempChar, rawStackFile);
                            lastCharAdded = tempChar;
                            if (!isalnum(tempChar)){
                                lastTileTypeAdded = tempChar;
                            }
                        }
                    }
                }
			}
		} else { // End of file

            // End of file but still must add variable name to the list or check for duplicates
            if (inVar) {
                inVar = false;
                tempVarName[tempVarLength + 1] = '\0'; // Adding the null terminator
                char *shortName = getShortName(varList, tempVarName);
                fputs(shortName, rawStackFile);
            }
        }
	}

	fclose(stackFile);
	fclose(rawStackFile);
	return true;
}

int main(int argc, char* argv[]){
	char * fileNames [argc];
    int filesCount = 0;

	if (argc == 1){
		puts ("Need a file to run");
		return(1);
	}
    puts("Processing files...");
    bool smallVarNames = false;
    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-s") == 0){
            smallVarNames = true;
        } else {
            // Checking for .tas extension
            char * fileName = argv[i];
            if (strcmp(fileName + (strlen(fileName) - 4), ".tas") != 0){
                puts("Warning - File does not have a .tas extension");
            }
            fileNames[filesCount] = argv[i];
            filesCount++;
            printf("\"%s\" ", argv[i]);
        }
    }

    // Making the raw stack files
    for (int i = 0; i < filesCount; i++){
        makeRawStackFile(fileNames[i], smallVarNames);
    }
	return 0;
}
