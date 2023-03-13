#include "varmgr.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>



int varHash(const char *name, int size){
    int hash = 0;
    int i;
    for (i = 0; i < strlen(name); i++){
        hash = (hash * 31 + name[i]) % size;
    }
    return hash;
}

// Returns the index of a variable in the variable manager with the given name
// If it does not exist, it will return the index of an empty slot
// If the array is full, it will return -1
int findVar(char *name, struct varmgr *inVarMgr){
    int hash = varHash(name, inVarMgr->size);
    int i;
    for (i = 0; i < inVarMgr->size; i++) {
        // If the name is null, or the name is the same as the one we are looking for
        if ((inVarMgr->vars[hash].name == NULL) || (strcmp(inVarMgr->vars[hash].name, name) == 0)) {
            return hash;
        }
        hash = (hash + 1) % inVarMgr->size; // Linear probing
    }
    return -1; // Array is full
}

// Doubles the size of the array and rehashes the variables
// This is called when the array is full
void expandArray(struct varmgr *inVarMgr){
    int oldSize = inVarMgr->size;
    inVarMgr->size *= 2;
    var *oldVars = inVarMgr->vars;
    inVarMgr->vars = malloc(sizeof(var) * inVarMgr->size);
    int i;
    for (i = 0; i < inVarMgr->size; i++){
        inVarMgr->vars[i].name = NULL;
        inVarMgr->vars[i].value = 0;
    }
    for (i = 0; i < oldSize; i++){
        // Rehashing the nonnull variables in the old array
        if (oldVars[i].name != NULL){
            int hash = varHash(oldVars[i].name, inVarMgr->size);
            while (inVarMgr->vars[hash].name != NULL){
                hash = (hash + 1) % inVarMgr->size; // Linear probing
            }
            inVarMgr->vars[hash] = oldVars[i];
        }

    }
    free(oldVars); // Free the old array
}

void insertVariable(char *name, struct varmgr *inVarMgr, int index){
    // Checking if a name needs to be found.
    if (index == -1){ // Index of -1 means that a name has not already been provided
        // Finding an open name
        index = findVar(name, inVarMgr); // Find the variable in the array or an empty slot
        if (index == -1){ // If the array is full, then expand it and try again
            expandArray(inVarMgr);
            index = findVar(name, inVarMgr);
        }
    }
    int nameLength = strlen(name);
    // Double-checking that the index points to a free name, then add a new variable with this name and a value of 0
    if (inVarMgr->vars[index].name == NULL){ // If the name is null, then the variable does not exist
        inVarMgr->vars[index].name = malloc(sizeof(char) * (nameLength + 1)); // Allocating space for the name
        strncpy(inVarMgr->vars[index].name, name, nameLength); // Copying the name into the variable
        inVarMgr->vars[index].name[nameLength] = '\0'; // Adding the null terminator
        inVarMgr->vars[index].value = 0; // Setting the value to 0
        inVarMgr->varCount++; // Incrementing the number of variables
    }
}

// Returns the value of a variable in the variable manager with the given name
// Will return 0 if the variable does not exist
// Will not ever create a new variable, use changeVar for that
int getVar(char *inName, struct varmgr *inVarMgr){
    char *name = joinName(inName, inVarMgr);

    int index = findVar(name, inVarMgr); // Find the variable in the array or an empty slot

    if (index == -1){ // If the array is full, then expand it and try again
        return 0;
    }

    if (inVarMgr->vars[index].name == NULL){ // If the name is null, then the variable does not exist
        return 0;
    }

    // Getting the value at that index
    return inVarMgr->vars[index].value;
}

// Changes the value of a variable in the variable manager with the given name
// Will create a new variable if it does not exist
void changeVar(char *inName, bool direction, struct varmgr *inVarMgr){
    char *name = joinName(inName, inVarMgr);
    int index = findVar(name, inVarMgr); // Find the variable in the array or an empty slot

    if (index == -1){ // If the array is full, then expand it and try again
        expandArray(inVarMgr);
        index = findVar(name, inVarMgr);
    }

    // If the index points to a free name, then add a new variable with this name and a value of 0
    if (inVarMgr->vars[index].name == NULL){ // If the name is null, then the variable does not exist
        insertVariable(name, inVarMgr, index);
    }

    // Incrementing or decrementing the value at that index
    if (direction){
        inVarMgr->vars[index].value++;
    } else {
        inVarMgr->vars[index].value--;
    }
}

// Used to initialize a variable manager
// Returns a pointer to the variable manager
struct varmgr *createVarMgr(){
    struct varmgr *newVarMgr = malloc(sizeof(struct varmgr));
    newVarMgr->size = 1;
    newVarMgr->varCount = 0;
    newVarMgr->vars = malloc(sizeof(var) * newVarMgr->size);
    int i;
    for (i = 0; i < newVarMgr->size; i++){
        newVarMgr->vars[i].name = NULL;
        newVarMgr->vars[i].value = 0;
    }
    return newVarMgr;
}

// Shows the variables in the variable manager
// Used for debugging
void showVars(struct varmgr *inVarMgr){
    // Printing a header
    printf("%3s %10s %5s\n", "loc", "Name", "Value");
    int i;
    for (i = 0; i < inVarMgr->size; i++){
        if (inVarMgr->vars[i].name != NULL){
            printf("%3d %10s %5d\n", i, inVarMgr->vars[i].name, inVarMgr->vars[i].value);
        } else {
            printf("%3d %10s\n", i, "[]");
        }
    }
}

// Frees the memory used by the variable manager
void freeVarMgr(struct varmgr *inVarMgr){
    int i;
    for (i = 0; i < inVarMgr->size; i++){
        if (inVarMgr->vars[i].name != NULL){
            free(inVarMgr->vars[i].name); // Freeing the name
        }
    }
    free(inVarMgr->vars); // Freeing the array
    free(inVarMgr); // Freeing the variable manager
}

char * joinName(char *name, struct varmgr *inVarMgr){
    // Searching for a colon
    // The value of the name between the colon and the next colon or the end is added to the name
    // Then the value of the name that comes after the colon is added to the name

    char * newName = malloc(sizeof(char) * (strlen(name) + 1));
    newName[0] = '\0'; // Adding the null terminator

    int i;
    for (i = 0; i < strlen(name); i++){
        if (name[i] == ':'){
            // Adding the colon to the processed name
            char tempC[2];
            tempC[0] = ':';
            tempC[1] = '\0';
            strcat(newName, tempC);

            // Using a for loop to find where this name ends
            int j;
            for(j = i + 1; j < strlen(name) && name[j] != ':'; j++){}

            // Getting the value of the name between the colons
            char tempName[j - i];
            strncpy(tempName, name + i + 1, j - i - 1);

            // Adding the null terminator
            tempName[j - i - 1] = '\0';

            // Using tempName to get the value of the variable in the variable manager
            int value = getVar(tempName, inVarMgr);

            // Converting the integer to a string using sprintf
            char temp[10];
            sprintf(temp, "%d", value);

            // Adding the value to the processed name
            strcat(newName, temp);

            // Setting i to the end of the name
            i = j - 1;


        } else {
            // Adding the character to the processed name, it's not special
            char temp[2];
            temp[0] = name[i];
            temp[1] = '\0';
            strcat(newName, temp);
        }
    }
    return newName;

}

void removeVar(char *inName, struct varmgr *inVarMgr){
    char *name = joinName(inName, inVarMgr);
    int index = findVar(name, inVarMgr); // Find the variable in the array or an empty slot

    if (index == -1){ // Array is full and the variable does not exist
        return;
    }

    if (inVarMgr->vars[index].name == NULL){ // If the name is null, then the variable does not exist
        return;
    }

    free(inVarMgr->vars[index].name);
    inVarMgr->vars[index].name = NULL;
    inVarMgr->vars[index].value = 0;
    inVarMgr->varCount--;
}

// Will create a new variable if it does not exist and set it to the value passed in
// If the variable already exists, then it will set the value to the value passed in
void setVar(char *inName, int value, struct varmgr *inVarMgr){
    char *name = joinName(inName, inVarMgr);
    int index = findVar(name, inVarMgr); // Find the variable in the array or an empty slot

    if (index == -1){ // If the array is full, then expand it and try again
        expandArray(inVarMgr);
        index = findVar(name, inVarMgr);
    }

    // If the index points to a free name, then add a new variable with this name and a value of 0
    if (inVarMgr->vars[index].name == NULL){ // If the name is null, then the variable does not exist
        insertVariable(name, inVarMgr, index); // Inserting the variable
    }

    inVarMgr->vars[index].value = value; // Setting the value
}