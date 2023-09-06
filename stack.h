#pragma once
#include <stdio.h>

void push(char *);
char *pop();
void irpush(char *);
char *irpop();
void drain();
void writeAllInstructions(FILE *);