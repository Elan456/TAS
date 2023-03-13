
#ifndef TAS_VARMGR_H
#define TAS_VARMGR_H

#include <stdbool.h>

typedef struct var_struct {
    char *name;
    int value;
} var;

struct varmgr{
    int size; // The size of the array
    int varCount; // The number of variables in the array
    var* vars; // The array of variables
};

// Returns the value of a variable in the variable manager with the given name
// Will return 0 if the variable does not exist
int getVar(char *name, struct varmgr *inVarMgr);

// Removes a variable from the variable manager with the given name
void removeVar(char *name, struct varmgr *inVarMgr);

// Increments or decrements the value of a variable by 1
void changeVar(char *name, bool direction, struct varmgr *inVarMgr);

void setVar(char *name, int value, struct varmgr *inVarMgr);

void freeVarMgr(struct varmgr *inVarMgr);

struct varmgr * createVarMgr();

void showVars(struct varmgr *inVarMgr);

char * joinName(char *name, struct varmgr *inVarMgr);

#endif //TAS_VARMGR_H
