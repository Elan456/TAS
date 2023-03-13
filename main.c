#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "varmgr.h"

// Control
//     > - Activate right
//     < - Activate left
//     } - Poke right
//     { - Poke left
//     ( - Deactivate left
//     ) - Deactivate right
//     _ - Blocker
//     . - Initializer
//     , - Remote activator
//     ? - Comparator
// Value
//     | - Unit
//     * - Reference
//     : - Joiner
//     = - Assignment
//     + - Successor
//     - - Predecessor
//     ~ - Destructor
// IO
//     & - Function call
//     " - User Input
//     ' - Parameter Input
//     ; - Output newline
//     @ - Output int
//     $ - Output char
//     ^ - Return value
// MISC
//     # - Comment

typedef struct PointStruct{
	char name [50];
    int index; // Only used for chuck activating i.e. (,)
} Point;

typedef struct TileStruct {
    unsigned int index; // Where the tile is in the tile array
	char type; // The type of the tile
	Point * point; // The variable, activation point, or filename that this tile works on
	struct TileStruct* nextActivate; // The next tile in the activation queue
    bool inActivationQueue; // Whether this tile is in the activation queue
} Tile;

typedef struct TileQueueStruct {
	Tile * first; // The first tile in the queue
    Tile * last; // The last tile in the queue
} tileQueue;

typedef struct ParameterStruct{
    var * variable;
    struct ParameterStruct * next; // The next parameter in the linked list

} Parameter;

typedef struct ParametersQueueStruct {
    Parameter * first; // The first parameter in the queue
    Parameter * using; // The parameter that is currently being used
} parameterQueue;

void parameterQueueAppend(parameterQueue * queue, Parameter * parameter){
    parameter->next = NULL;
    if (queue->first == NULL){
        queue->first = parameter;
    } else {
        Parameter * temp = queue->first;
        while (temp->next != NULL){ // Finding the last parameter
            temp = temp->next;
        }
        temp->next = parameter;
    }
}

typedef struct TASStruct {
	Tile ** tiles; // The array of tiles
	unsigned int length; // The length of the array
	tileQueue * Activation; // The activation queue
    struct varmgr * vm; // The variable manager

    // For function calls
    parameterQueue * parameters; // The parameters queue
    parameterQueue * returnHolders; // The return holders queue

} TAS;

// Prototypes
void runTAS(const char * fileName, bool isShowingStack, parameterQueue * arguments, parameterQueue * returnHolders);

void showActivationQueue(TAS * tas) {
    puts("Activation Queue:");
    Tile *tempTile = tas->Activation->first;
    unsigned int num = 1;
    while (tempTile != NULL) {
        printf("%d: %c\n", num, tempTile->type);
        num++;
        tempTile = tempTile->nextActivate;
    }
}

// Adds a tile to the end of the activation queue (FIFO)
void activate (tileQueue * activationQueue, Tile * tile){
    if (!tile->inActivationQueue){
        tile->inActivationQueue = true;
    } else {
        return;
    }

    if (activationQueue->last != NULL && activationQueue->first != NULL){
        activationQueue->last->nextActivate = tile; // Linking
        activationQueue->last = tile; // Saving the last name so a new tile can
        activationQueue->last->nextActivate = NULL;
        // be added to the end easily
    } else {
        // This queue has nothing in it
        activationQueue->last = tile;
        activationQueue->first = tile;
        activationQueue->first->nextActivate = NULL;
    }
}

void multiActivate(TAS * tas, unsigned int index, int direction){
    // Activates all the tiles with a greater or lower index until it hits a blocker or a poker or the end
    for (int i = index + direction; i < tas->length && i >= 0; i += direction){
        if (tas->tiles[i]->type == '_'){
            break;
        } else if (tas->tiles[i]->type == '}' || tas->tiles[i]->type == '{'){
            // Activates the poker and stops
            activate(tas->Activation, tas->tiles[i]);
            break;
        } else {
            // Activates the tile and continues
            activate(tas->Activation, tas->tiles[i]);
        }
    }
}

// Removes a tile from the activation queue
// This is used when a tile is deactivated
void deactivate(TAS * tas, Tile * tile){
    // Find the tile in the activation queue and removes it
    Tile * currentTile = tas->Activation->first;
    Tile * previousTile = NULL;
    while (currentTile != NULL){
        if (currentTile == tile){
            // Found the tile
            if (previousTile == NULL){
                // This is the first tile in the queue
                tas->Activation->first = currentTile->nextActivate; // Removing the first tile
            } else {
                // This is not the first tile in the queue
                previousTile->nextActivate = currentTile->nextActivate; // Removing the tile
            }
            break;
        }
        previousTile = currentTile;
        currentTile = currentTile->nextActivate;
    }
}

void multiDeactivate(TAS * tas, unsigned int index, int direction){
    // Deactivates all the tiles with a greater or lower index until it hits a blocker
    for (int i = index + direction; i < tas->length && i >= 0; i += direction){
        if (tas->tiles[i]->type == '_'){
            break;
        } else {
            // Deactivates the tile and continues
            deactivate(tas, tas->tiles[i]);
        }
    }
}



void cycle(TAS * tas){
    // Activating the first tile in the activation queue
    Tile * currentTile = tas->Activation->first;
    currentTile->inActivationQueue = false; // Removing the tile from the activation queue
    // Removing the first tile from the activation queue
    tas->Activation->first = tas->Activation->first->nextActivate;
    // Activating the tile

    int leftValue;
    int rightValue;

    int input;
    int val;

    Tile * tempTile;

    switch (currentTile->type) {
        // Activates all the tiles with a greater index until it hits a blocker or a poker
        case '>':
            multiActivate(tas, currentTile->index, 1);
            break;
        case '<':
            multiActivate(tas, currentTile->index, -1);
            break;
        case '}':
            // Activates the tile immediately to the right if not on the right edge
            if (currentTile->index != tas->length - 1){
                activate(tas->Activation, tas->tiles[currentTile->index + 1]);
            }
            break;
        case '{':
            // Activates the tile immediately to the left if not on the left edge
            if (currentTile->index != 0){
                activate(tas->Activation, tas->tiles[currentTile->index - 1]);
            }
            break;
        case '(':
            multiDeactivate(tas, currentTile->index, -1);
            break;
        case ')':
            multiDeactivate(tas, currentTile->index, 1);
            break;
        case ',':
            // Activates the tile based on the index of its point from previous linking
            activate(tas->Activation, tas->tiles[currentTile->point->index]);
            break;
        case '?':
            // Comparing the variable on the right to the variable on the left
            // If there are units (|), those are counted instead

            // Checking if the tile to left is a unit (|) or a reference (*)
            leftValue = 0; // 0 is the default value if there is no tile to the left
            if (currentTile->index != 0) {
                tempTile = tas->tiles[currentTile->index - 1];

                // Its a reference (*)
                if (tempTile->type == '*') {
                    leftValue = getVar(tempTile->point->name, tas->vm);
                } else if (tempTile->type == '|') {
                    // Counting the number of consecutive units to the left
                    leftValue = 0;
                    while (tempTile->type == '|') {
                        leftValue++;
                        tempTile = tas->tiles[tempTile->index - leftValue - 1];
                    }
                }
            }
            rightValue = 0; // 0 is the default value if there is no tile to the right
            if (currentTile->index != tas->length - 1) {

                // Getting the value on the right side
                tempTile = tas->tiles[currentTile->index + 1];
                if (tempTile->type == '*') {
                    rightValue = getVar(tempTile->point->name, tas->vm);
                } else if (tempTile->type == '|') {
                    // Counting the number of consecutive units to the right
                    rightValue = 0;
                    while (tempTile->type == '|') {
                        rightValue++;
                        tempTile = tas->tiles[tempTile->index + rightValue + 1];
                    }
                }
            }

            // Comparing right to left and then activating in that direction
            // If they are equal it activates to the left i.e. left is default
            if (rightValue > leftValue){
                multiActivate(tas, currentTile->index, 1);
            } else { // When they are equal it activates to the left
                multiActivate(tas, currentTile->index, -1);
            }
            break;
        case '=':
            // Taking the values of the variable on the left and on the right and combining them,
            // then setting the variable of this tile to that value

            // 0 is the default value if there is no tile to the left or right
            leftValue = 0;
            rightValue = 0;

            // Getting the left value
            if ( currentTile->index != 0) {
                tempTile = tas->tiles[currentTile->index - 1];
                if (tempTile->type == '*') {
                    leftValue = getVar(tempTile->point->name, tas->vm);
                }
            }

            // Getting the right value
            if (currentTile->index != tas->length - 1) {
                tempTile = tas->tiles[currentTile->index + 1];
                if (tempTile->type == '*') {
                    rightValue = getVar(tempTile->point->name, tas->vm);
                }
            }

            // Combining the values and setting the variable
            setVar(currentTile->point->name, leftValue + rightValue, tas->vm);
            break;
        case '+':
            changeVar(currentTile->point->name, true, tas->vm);
            break;
        case '-':
            changeVar(currentTile->point->name, false, tas->vm);
            break;
        case '\"':
            // Collect an integer input from the user and set the value of the variable to that

            scanf("%d", &input);
            int difference = input - getVar(currentTile->point->name, tas->vm);

            for (int i = 0; i < abs(difference); i++){
                changeVar(currentTile->point->name, difference > 0, tas->vm);
            }
            break;
        case '\'':
            // Using the next parameter in parameters as the value of the variable
            // If there are no more parameters, it will use 0
            if (tas->parameters != NULL && tas->parameters->first != NULL){
                setVar(currentTile->point->name, tas->parameters->first->variable->value, tas->vm);
                tas->parameters->first = tas->parameters->first->next;

            } else {
                puts("Variable is being set to 0 because there are no more parameters");
                setVar(currentTile->point->name, 0, tas->vm);
            }


            break;

        case '~':
            // Removes this variable from the varmngr
            removeVar(currentTile->point->name, tas->vm);
            break;
        case '&': {
            // Grabbing variables on the left to be used as arguments and variables on the right to be used as return holders

            // Setting up the linked list
            parameterQueue *parameters = malloc(sizeof(parameterQueue));
            parameters->first = NULL;

            parameterQueue *returnHolders = malloc(sizeof(parameterQueue));
            returnHolders->first = NULL;
            returnHolders->using = NULL;

            // Getting the parameters from the left
            if (currentTile->index != 0) {
                tempTile = tas->tiles[currentTile->index - 1];
                while (tempTile->type == '*' && tempTile->index >= 0) {
                    // Adding the variable to the parameters
                    // Creating a parameter
                    Parameter *param = malloc(sizeof(Parameter));
                    // Setting the value
                    param->variable = malloc(sizeof(var));
                    param->variable->value = getVar(tempTile->point->name, tas->vm);


                    parameterQueueAppend(parameters, param);

                    // Moving to the next tile
                    tempTile = tas->tiles[tempTile->index - 1];
                }
            }

            // Getting the return holders from the right
            if (currentTile->index != tas->length - 1) {
                tempTile = tas->tiles[currentTile->index + 1];
                while (tempTile->type == '*' && tempTile->index < tas->length) {
                    // Adding the variable to the parameters
                    // Creating a parameter
                    Parameter *param = malloc(sizeof(Parameter));
                    // Setting the value
                    param->variable = malloc(sizeof(var));
                    param->variable->value = 0; // Setting the value to 0 because it will be set by the function
                    param->variable->name = tempTile->point->name;

                    parameterQueueAppend(returnHolders, param);

                    // Moving to the next tile
                    tempTile = tas->tiles[tempTile->index + 1];
                }
            }

            returnHolders->using = returnHolders->first;

            // Runs a TAS using the point as the filename
            // Creating the filename by add .ptas to the end of the name
            char *filename = malloc(strlen(currentTile->point->name) + 6);
            strcpy(filename, currentTile->point->name);
            strcat(filename, ".ptas");

            // Checking if the file exists by trying to open it
            FILE *file = fopen(filename, "r");
            if (file == NULL){
                // Checking inside of the stdlib folder
                char *newFilename = malloc(strlen(filename) + 8);
                strcpy(newFilename, "stdlib/");
                strcat(newFilename, filename);
                file = fopen(newFilename, "r");
                if (file == NULL){
                    // If the file doesn't exist, it will print an error message and exit
                    printf("File %s does not exist\n", filename);
                    exit(1);
                } else {
                    // If the file does exist, it will use the new filename
                    filename = newFilename;
                }
            }


            runTAS(filename, false, parameters, returnHolders);

            // Using the return holders to set the variables
            // Going through the return holders
            while (returnHolders->first != NULL){
                // Setting the variable to the value of the return holder
                setVar(returnHolders->first->variable->name, returnHolders->first->variable->value, tas->vm);
                // Moving on to the next return holder
                returnHolders->first = returnHolders->first->next;
            }
        }
            break;
        case '@':
            printf("%d", getVar(currentTile->point->name, tas->vm));
            break;
        case '^':
            // Setting the value of the next returnHolder to the value of this variable
            // Checking if there are parameters left in returnHolders
            if (tas->returnHolders != NULL && tas->returnHolders->using != NULL){
                // Setting the value of that variable
                tas->returnHolders->using->variable->value = getVar(currentTile->point->name, tas->vm);
                // Moving on to the next variable from the returnHolders
                tas->returnHolders->using = tas->returnHolders->using->next;
            }
            break;
        case '$':
            printf("%c", getVar(currentTile->point->name, tas->vm));
            break;
        case ';':
            puts("");
            break;
    }
}


// Looks through the stack to find the . characters.
// Creates the activation queue
tileQueue * MakeInitialActivationQueue(TAS * stack){
	tileQueue * Activation = (tileQueue *)malloc(sizeof(tileQueue));
	Activation->first = NULL;
	Activation->last = NULL;

	return Activation;
}

void getCharTileCount(char * fileName, unsigned int * data){
	FILE * f = fopen(fileName, "r");
    // Checking if the file exists
    if (f == NULL){
        printf("Error: Could not open file \"%s\"\n", fileName);
        exit(1);
    }
	char tempChar;
	int tileCount = 0;
	int charCount = 0;
	while (!feof(f)){
		tempChar = fgetc(f);
		if (tempChar != EOF){
			charCount++;
			if (!isalnum(tempChar) && tempChar != ':' && tempChar != '.'){
				tileCount++;
			}
		}
	}
	fclose(f);
	data[0] = tileCount;
	data[1] = charCount;
}

void linkRemoteActivators(TAS * tas){
    // Finding the remote activators
    for (int i = 0; i < tas->length; i++){
        if (tas->tiles[i]->type == ','){ // Remote activator that needs to be linked
            // Finding the nearest tile with the same point name that isn't a remote activator
            bool done = false;
            int leftLook = i - 1;
            int rightLook = i + 1;
            while (!done && (leftLook >= 0 || rightLook < tas->length)){
                if (leftLook >= 0){
                    // Checking if left look is a tile with the same point name
                    if (tas->tiles[leftLook]->type != ',' && strcmp(tas->tiles[leftLook]->point->name, tas->tiles[i]->point->name) == 0){
                        tas->tiles[i]->point->index = leftLook;
                        done = true;
                    } else {
                        leftLook--;
                    }
                }

                if (rightLook < tas->length){
                    // Checking if right look is a tile with the same point name
                    if (tas->tiles[rightLook]->type != ',' && strcmp(tas->tiles[rightLook]->point->name, tas->tiles[i]->point->name) == 0){
                        tas->tiles[i]->point->index = rightLook;
                        done = true;
                    } else {
                        rightLook++;
                    }
                }
            }

            if (!done){
                printf("Failed to think remote activator #%d with point %s\n", i, tas->tiles[i]->point->name);
                exit(1);
            }
        }
    }
}
// Iterates through the file and creates a tile for each character and links
// them into a linked list
// Creates an activate queue as well
TAS * MakeTAS(char * fileName, parameterQueue * parameters, parameterQueue * returnHolders) {
	unsigned int counts [2];
	getCharTileCount(fileName, counts);
	unsigned int tileCount = counts[0];
	unsigned int charCount = counts[1];

	FILE * stackFile = fopen(fileName, "r");
	Tile * tempTile;
	char charList[charCount + 1];
	
	if (stackFile){
		fgets(charList, charCount+1, stackFile);
	} else {
        printf("Error: Could not open file %s\n", fileName);
		exit(1);
	}

	// Allocating room for the structure
	TAS * tlist = (TAS *)malloc(sizeof(TAS));

    // Setting function stuff up
    tlist->parameters = parameters;
    tlist->returnHolders = returnHolders;

	tlist->length = tileCount;

	// Allocating room for all the pointers in the tiles list
	tlist->tiles = (Tile **)malloc(sizeof(Tile *) * tileCount);
	unsigned int foundTiles = 0; // How many real tiles have been found

    bool activateNextTile = false; // Used for . initializers
    tlist->Activation = MakeInitialActivationQueue(tlist); // Creating the activation queue

	for (int i = 0; i < strlen(charList); i++){
		// Each time a non-alphanumeric character appears, continue
		// until another non-alphanumeric characters appears to get the whole
		// points
		
		if (!isalnum(charList[i]) && charList[i] != ':' && charList[i] != '.'){
            // Creating a new tile
			tempTile = (Tile *)malloc(sizeof(Tile));

			tempTile->point = (Point *)malloc(sizeof(Point));
            // Setting the point to an empty string
            tempTile->point->name[0] = '\0';

			tempTile->type = charList[i];
			tempTile->nextActivate = NULL;
            tempTile->index = foundTiles;
			tlist->tiles[foundTiles] = tempTile;

            // If the previous tile was a ., then this tile should be activated
            if (activateNextTile){
                activateNextTile = false;
                activate(tlist->Activation, tempTile);
            }
			foundTiles++;

			// Iterating to find the point
			int j = 1;
			bool foundAnything = false;
            // This loop will continue until it finds a non-alphanumeric character or colon or the end of the string
			while ( i + j < strlen(charList) && (isalnum(charList[i + j]) || charList[i + j] == ':')){
				foundAnything = true;
				// Adding characters to the end of the point
				char cur [2];
				cur[0] = charList[i + j];
				
				strcat(tempTile->point->name, cur);
				j++;
			}

            // If nothing was found, set the point to an empty string
			if (!foundAnything){
				tempTile->point->name[0] = '0';
				tempTile->point->name[1] = '\0';
			}
			i = i + j - 1; // Move up to that name

		} else if (charList[i] == '.'){
            activateNextTile = true;
        }

	}
	fclose(stackFile);
    // Linking remote activators
    linkRemoteActivators(tlist);
    tlist->vm = createVarMgr(); // Creating the variable manager
	return tlist;
}

void freeActivationQueue(tileQueue * aq){
    // Freeing the tiles
    Tile * tempTile = aq->first;
    while (tempTile != NULL){
        Tile * nextTile = tempTile->nextActivate;
        free(tempTile);
        tempTile = nextTile;
    }
    free(aq);
}

void freeTAS(TAS * tas){
    // Freeing the tiles
    for (int i = 0; i < tas->length; i++){
        free(tas->tiles[i]->point);
        free(tas->tiles[i]);
    }
    free(tas->tiles);
    freeActivationQueue(tas->Activation);
    freeVarMgr(tas->vm);
    free(tas);
}


void showStack(TAS * tas, struct varmgr * vm){
	
	struct displayTile{
		Tile * tile;
		unsigned int activationNum;
	};
	
	struct displayTile dtiles [tas->length];
	 
	for (int i = 0; i < tas->length; i++){
		dtiles[i].tile = tas->tiles[i];
		dtiles[i].activationNum = 0;
	}
	

	// Going through the queue to get activation numbers
	Tile * tempTile = tas->Activation->first;
	unsigned int num = 1;
	while (tempTile != NULL){
		// Finding which dtile has this tile
		int i = 0;
		while (dtiles[i].tile != tempTile){i++;} // Linear search
		dtiles[i].activationNum = num;
		num++;
		tempTile = tempTile->nextActivate;
	}

	// Displaying the dtiles
    printf("%4s | %2c | %5s | %10s | %5s | %s\n", "Loc", 'T', "Act", "Point", "PVal", "Address");
    puts("--------------------------------------------------");
	for (int i = 0; i < tas->length; i++){
		printf("%4d | %2c | %5d | %10s | %5d | %p\n",
				i,
				dtiles[i].tile->type,
				dtiles[i].activationNum,
                dtiles[i].tile->point->name,
                getVar(dtiles[i].tile->point->name, vm),
				dtiles[i].tile);
	}
}


void runTAS(const char *fileName, bool isShowingStack, parameterQueue *arguments, parameterQueue *returnHolders) {
    // Creating the initial TAS
    TAS * tas = MakeTAS(fileName, arguments, returnHolders);

    // Running the TAS until the activation queue is empty
    while (tas->Activation->first != NULL){
        // Running the TAS for a cycle
        cycle(tas);
        if (isShowingStack){
            showStack(tas, tas->vm);
            puts("");
        }
    }


}

int main(int argc, char* argv[]){
    puts("Started");
	bool isShowingStack = false;
	char * fileName;
	if (argc == 1){
		puts ("Need a file to run - No arguments given");
		return(1);
	}
	for (int i = 1; i < argc; i++){
		if (strlen(argv[i]) == 2){
			// Must be a flag
			if (argv[i][1] == 's'){
				isShowingStack = true;
			}
		} else {
			// Must be the file name
			fileName = argv[i];
		}
	}
	
	// Using the given filename to run a TAS
    runTAS(fileName, isShowingStack, NULL, NULL);

    printf("\n\nDone \n");

	return 0;
}
