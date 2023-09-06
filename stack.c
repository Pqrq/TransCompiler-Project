#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

/// Stack for variables and numbers
char *stack[1000];
int stackCounter = 0;

/// Function for pushing a variable or a number into the stack
void push(char *str) {
    stack[stackCounter] = str;
    stackCounter++;
}

/// Function for popping a variable or a number into the stack
char *pop() {
    stackCounter--;
    return stack[stackCounter];
}

//////////////////////////// IR STACK ////////////////////////////

/// Stack of instructions, but reversed
char *irstack[1000];
int irstackCounter = 0;

/// Function for pushing an instruction into the irstack
void irpush(char *str) {
    irstack[irstackCounter] = str;
    irstackCounter++;
}

/// Function for popping an instruction into the irstack
char *irpop() {
    irstackCounter--;
    return irstack[irstackCounter];
}

/// Emptying the reversed instructions stack
void drain() {
    while (irstackCounter > 0) {
        char *temp = irpop();
        free(temp);
    }
}

/// Writing all instructions in proper order
void writeAllInstructions(FILE *file) {
    for (int i = 0; i < irstackCounter; i++) {
        fprintf(file, "%s\n", irstack[i]);
        if (irstack[i] != NULL) {
            free(irstack[i]);
            irstack[i] = NULL;
        }
    }
    irstackCounter = 0;
}
