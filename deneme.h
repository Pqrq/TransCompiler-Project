#pragma once

#include <string.h>
#include "standart.h"

void solveStatement();
int getString();
int solveExpr(char *);
int getOper(char);
void getVar(char *);
void dismissblank();
void initialize_sides( int*,  int*);
void push(char *);
char *pop();
void toString(char*,int);
char* getNewVar();
void irpush(char *);
char *irpop();
void drain();
void writeAllInstructions(FILE *);
int solveOp(int, char *last);



enum operator {
    notoperator,
    or,
    and,
    sum,
    mul,
};

#define min sum
#define sdiv mul
#define urem mul
