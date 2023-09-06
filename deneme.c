#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "hashmap.h"
#include "standart.h"
#include "deneme.h"
#include "stack.h"

char mainStatement[257]; /// The given statement
int counter = 0; /// Iterator
char deflastChar[2] = {'\n', 0}; /// Our default last characters
char lastforparanthesis[3] = {')', '\n', 0}; /// Last characters for expressions inside parenthesis
char lastforfunc[3] = {',', '\n', 0}; /// Last characters for expressions inside functions
bool8 error = FALSE; /// Line error checker flag, checks whether there exists an error in the line being read
HashMap variables; /// Hashmap of variables
char *functions[6] = {"xor", "ls", "rs", "lr", "rr", "not"}; /// Array of function names
FILE *output; /// Pointer to the output file
int linecounter = 1; /// Line counter, counts which line we are reading
int variableCounter = 0; /// Variable counter, counts how many LLVM IR variables we've used so far
/// (counts the number of %1,%2,...,%x type of variables, not a,b,x type of variables)
bool8 areThereAnyErrors = FALSE; /// Overall error checker flag, checks whether there exists a line with error
/// (if there are no errors in the entire file, it remains FALSE, otherwise it turns into TRUE)


/// This is our main function. It reads lines until we reach the end of file
int main(int argc, char **argv) {
    FILE *input; // Pointer to the input file
    /// Opening the input file for reading
    input = fopen(argv[1], "r");

    /// Changing the file extension to .ll
    int i = 0;
    while (argv[1][i] != '\0') {
        if (argv[1][i] == '.') {
            argv[1][i + 1] = 'l';
            argv[1][i + 2] = 'l';
            argv[1][i + 3] = '\0';
            break;
        }
        i++;
        if (i > 1024) {
            areThereAnyErrors = TRUE;
            printf("Error: File name is too long");
            return 0;
        }
    }
    /// Writing to the output file
    output = fopen(argv[1], "w");
    /// Initialization of HashMap
    variables = initializeHashMap();

    /// Writing the beginning part of our output file
    fprintf(output, "; ModuleID = 'advcalc2ir'\n");
    fprintf(output, "declare i32 @printf(i8*, ...)\n");
    fprintf(output, "@print.str = constant [4 x i8] c\"%s%s\\0A\\00\"\n", "%", "d");
    fprintf(output, "\n");
    fprintf(output, "define i32 @main() {\n");


    /// Our main loop
    while (TRUE) {
        // Reseting the line error flag for each line being read
        error = FALSE;
        if (!fgets(mainStatement, 257, input)) {
            /// Writing the ending part of our output file
            fprintf(output, "ret i32 0\n");
            fprintf(output, "}\n");

            // In order to free allocated memories
            deconstractor(&variables);

            // Closing the input file
            fclose(input);

            // If no errors occur, just close the output file
            if (!areThereAnyErrors) {
                fclose(output);
            }

            // If any errors occur during the reading of the file, close and remove the output file
            else {
                fclose(output);
                remove(argv[1]);
            }
            return 0;
        }
        /// The part which reads lines and solves them
        counter = 0;
        solveStatement();
        linecounter++;
    }
}


/*
 * Our BNF rules for creating statements is (shortly):
 * <statement> := <assignment> | <expression>
 * <assignment> := <var> = <expression>
 * This function checks if the statement is an assignment or expression and proceeds accordingly
*/
void solveStatement() {
    char *assignedVar = calloc(257, sizeof(char));
    getVar(assignedVar);
    int solution;
    dismissblank();

    // Checks if this is an expression or an assignment
    if (*assignedVar == '\0' || (mainStatement[counter] != '=')) {
        if (*assignedVar == '\0' && (mainStatement[counter] == '\0')) { return; }
        counter = 0;
        solution = solveExpr(deflastChar);
        if (error) {
            // In case of an error, all the instructions on instructions stack will be discarded
            // areThereAnyErrors flag will be TRUE, and print "Error on line (linecounter)!"
            drain();
            areThereAnyErrors = TRUE;
            printf("Error on line %d!\n", linecounter);
        } else {
            // Otherwise, write all the instructions to the output file
            writeAllInstructions(output);

            // Writing the LLVM IR printing command to the output file
            char *printed = pop();
            fprintf(output,
                    "call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %s)\n",
                    printed);
            free(printed);
            getNewVar();
        }
    } else {
        // Checks if the variable's name is a function name
        for (int i = 0; i < 6; i++) {
            if (chadstrcmp(functions[i], assignedVar) == 0) {
                // In case of an error, all the instructions on instructions stack will be discarded
                // areThereAnyErrors flag will be TRUE, and print "Error on line (linecounter)!"
                drain();
                areThereAnyErrors = TRUE;
                printf("Error on line %d!\n", linecounter);
                return;
            }
        }
        counter++;
        dismissblank();
        solution = solveExpr(deflastChar);
        if (error) {
            // In case of an error, all the instructions on instructions stack will be discarded
            // areThereAnyErrors flag will be TRUE, and print "Error on line (linecounter)!"
            drain();
            areThereAnyErrors = TRUE;
            printf("Error on line %d!\n", linecounter); //asdf
        } else {
            // Once a value has been allocated, it cannot be allocated again. This part checks
            // whether a variable has already been allocated or not
            if (!isAllocated(&variables, assignedVar)) {
                fprintf(output, "%c%s = alloca i32\n", '%', assignedVar); //asdf
            }
            // Otherwise, write all the instructions to the output file
            writeAllInstructions(output);

            // Writing the LLVM IR value storing command to the output file
            char *printed = pop();
            fprintf(output, "store i32 %s, i32* %c%s\n", printed, '%', assignedVar); //asdf
            free(printed);
            add_new_element(&variables, assignedVar, solution);
        }
    }
    return;
}

/*
 * Our EBNF rules for creating strings is (shortly):
 * <string> := '('<expression>')' | <var> | func'('<expression>, <expression>')'
 * | not'('<expression>')' | <num>
 * This function gets the nearest string and solves it
 */
int getString() {
    dismissblank();
    // Checks if it is a (<expression>)
    if (mainStatement[counter] == '(') {
        counter++;
        dismissblank();
        int solution = solveExpr(lastforparanthesis);
        if (error) { return 0; }
        if (mainStatement[counter] != ')') {
            error = TRUE;
            return 0;
        }
        counter++;
        return solution;
    }
    // Checks if it is a <var> | func(<expression>, <expression>) | not(<expression>)
    if ((mainStatement[counter] >= 'A' && mainStatement[counter] <= 'Z') ||
        (mainStatement[counter] >= 'a' && mainStatement[counter] <= 'z')) {
        char str[257];
        getVar(str);
        int right;
        int left;
        dismissblank();

        // Checks if it is a func(<expression>, <expression>) | not(<expression>)
        if (chadstrcmp(str, functions[0]) == 0) {
            initialize_sides(&left, &right);
            if (error) { return 0; }
            // If there are no errors, create the equivalent LLVM IR instruction, push it into irstack
            // and add the variable string (%variableCounter) to the (variable) stack
            char *newVar = getNewVar();
            char *rightSide = pop();
            char *leftSide = pop();
            char *ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = xor i32 %s,%s", newVar, leftSide, rightSide);
            irpush(ins);
            free(rightSide);
            free(leftSide);

            push(newVar);

            return left ^ right;
        }
        // Checks if it is a ls(<expression>, <expression>)
        if (chadstrcmp(str, functions[1]) == 0) {
            initialize_sides(&left, &right);
            if (error) { return 0; }
            // If there are no errors, create the equivalent LLVM IR instruction, push it into irstack
            // and add the variable string (%variableCounter) to the (variable) stack
            char *newVar = getNewVar();
            char *rightSide = pop();
            char *leftSide = pop();
            char *ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = shl i32 %s,%s", newVar, leftSide, rightSide);
            irpush(ins);
            free(rightSide);
            free(leftSide);

            push(newVar);

            return left << right;
        }
        // Checks if it is a rs(<expression>, <expression>)
        if (chadstrcmp(str, functions[2]) == 0) {
            initialize_sides(&left, &right);
            if (error) { return 0; }
            // If there are no errors, create the equivalent LLVM IR instruction, push it into irstack
            // and add the variable string (%variableCounter) to the (variable) stack
            char *newVar = getNewVar();
            char *rightSide = pop();
            char *leftSide = pop();
            char *ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = ashr i32 %s,%s", newVar, leftSide, rightSide);
            irpush(ins);
            free(rightSide);
            free(leftSide);

            push(newVar);

            return left >> right;
        }
        // Checks if it is a lr(<expression>, <expression>)
        if (chadstrcmp(str, functions[3]) == 0) {
            initialize_sides(&left, &right);
            if (error) { return 0; }
            // If there are no errors, create the equivalent LLVM IR instructions, push them into irstack
            // and add the variable strings (%variableCounter) to the (variable) stack
            char *rightSide = pop();
            char *leftSide = pop();
            char *newVar_1 = getNewVar();
            char *ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = urem i32 %s,%s", newVar_1, rightSide, "32");
            irpush(ins);
            ins = (char *) malloc(sizeof(char) * 257);
            char *newVar = getNewVar();
            sprintf(ins, "%s = shl i32 %s,%s", newVar, leftSide, newVar_1);
            irpush(ins);
            char *newVar2 = getNewVar();
            ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = sub i32 %s,%s", newVar2, "32", newVar_1);
            irpush(ins);
            char *newVar3 = getNewVar();
            ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = ashr i32 %s,%s", newVar3, leftSide, newVar2);
            irpush(ins);
            char *newVar4 = getNewVar();
            ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = or i32 %s,%s", newVar4, newVar, newVar3);
            irpush(ins);
            push(newVar4);

            free(newVar_1);
            free(newVar);
            free(newVar2);
            free(newVar3);
            free(rightSide);
            free(leftSide);

            right %= 32;
            return (left << right) | (left >> (32 - right));
        }
        // Checks if it is a rr(<expression>, <expression>)
        if (chadstrcmp(str, functions[4]) == 0) {
            initialize_sides(&left, &right);
            if (error) { return 0; }
            // If there are no errors, create the equivalent LLVM IR instructions, push them into irstack
            // and add the variable strings (%variableCounter) to the (variable) stack
            char *rightSide = pop();
            char *leftSide = pop();
            char *newVar_1 = getNewVar();
            char *ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = urem i32 %s,%s", newVar_1, rightSide, "32");
            irpush(ins);
            ins = (char *) malloc(sizeof(char) * 257);
            char *newVar = getNewVar();
            sprintf(ins, "%s = ashr i32 %s,%s", newVar, leftSide, newVar_1);
            irpush(ins);
            char *newVar2 = getNewVar();
            ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = sub i32 %s,%s", newVar2, "32", newVar_1);
            irpush(ins);
            char *newVar3 = getNewVar();
            ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = shl i32 %s,%s", newVar3, leftSide, newVar2);
            irpush(ins);
            char *newVar4 = getNewVar();
            ins = (char *) malloc(sizeof(char) * 257);
            sprintf(ins, "%s = or i32 %s,%s", newVar4, newVar, newVar3);
            irpush(ins);
            push(newVar4);

            free(newVar_1);
            free(newVar);
            free(newVar2);
            free(newVar3);
            free(rightSide);
            free(leftSide);

            right %= 32;
            return (left >> right) | (left << (32 - right));
        }
        // Checks if it is a not(<expression>)
        if (chadstrcmp(str, functions[5]) == 0) {
            if (mainStatement[counter] == '(') {
                counter++;
                dismissblank();
                left = solveExpr(lastforparanthesis);
                if (error) { return 0; }
                if (mainStatement[counter] != ')') {
                    error = TRUE;
                    return 0;
                }
                counter++;
                // If there are no errors, create the equivalent LLVM IR instruction, push it into irstack
                // and add the variable string (%variableCounter) to the (variable) stack
                char *newVar = getNewVar();
                char *moment = pop();
                char *ins = (char *) malloc(sizeof(char) * 257);
                sprintf(ins, "%s = xor i32 %s, -1", newVar, moment);
                irpush(ins);
                free(moment);

                push(newVar);

                return ~left;
            } else {
                error = TRUE;
                return 0;
            }
        }
        // If there are no errors, create the equivalent LLVM IR instruction, push it into irstack
        // and add the variable string (%variableCounter) to the (variable) stack
        char *newVar = getNewVar();
        char *ins = (char *) malloc(sizeof(char) * 257);
        sprintf(ins, "%s = load i32, i32* %c%s", newVar, '%', str);
        irpush(ins);

        push(newVar);

        return getValue(&variables, str);
    }
    // Checks if it is a <num>
    if ((mainStatement[counter] >= '0' && mainStatement[counter] <= '9')) {
        // If there are no errors, add the number to the (variable) stack
        char *number = calloc(257, sizeof(char));
        int solution = 0;
        int tempcounter = 0;
        while ((mainStatement[counter] >= '0' && mainStatement[counter] <= '9')) {
            solution = 10 * solution + (mainStatement[counter] - '0');
            number[tempcounter] = mainStatement[counter];
            counter++;
            tempcounter++;
        }
        number[tempcounter] = '\0';
        push(number);
        return solution;
    }
    error = TRUE;
    return 0;
}

/// Gets operation (stays in the current operator)
/// Returns to precedence of the given operator
int getOper(char op) {
    dismissblank();
    if (op == '|') { return or; }
    if (op == '&') { return and; }
    if (op == '+') { return sum; }
    if (op == '-') { return min; }
    if (op == '*') { return mul; }
    if (op == '/') { return sdiv; }
    if (op == '%') { return urem; }
    error = TRUE;
    return notoperator;
}

/// Gets the nearest variable from the counter
void getVar(char *assignedvar) {
    char *debug = mainStatement;
    int *debug2 = &counter;
    dismissblank();
    for (int i = 0; i < 257; i++) {
        if ((mainStatement[counter] >= 'A' && mainStatement[counter] <= 'Z') ||
            (mainStatement[counter] >= 'a' && mainStatement[counter] <= 'z')) {
            assignedvar[i] = mainStatement[counter];
            counter++;
        } else {
            assignedvar[i] = 0;
            dismissblank();
            return;
        }
    }
}

/// Dismisses all blanks until it hits something else
void dismissblank() {
    while (isspace(mainStatement[counter])) {
        counter++;
    }
}

/// Solves the expression starting from the counter until it reaches a last character
int solveExpr(char *last) {
    if (error) { return 0; }
    int *counterchecker = &counter;
    char *mainStatementchecker = mainStatement;
    return solveOp(or, last);
}

/// Solves the operations in a recursive manner, considering precedences as well
int solveOp(int precedence, char *last) {
    // As we don't have any operation having bigger precedence than *,/,% 
    // if the precedence is higher than their precedences, just apply getString
    // to obtain the number
    if (precedence == mul + 1) { return getString(); }

    // In case of error, return 0
    if (error) { return 0; }
    int right;
    char op;
    // Solving the left part (recursively)
    int left = solveOp(precedence + 1, last);
    if (error) { return 0; }
    dismissblank();
    // The main solving loop
    while (TRUE) {
        // Checks if the operator is a last character or not
        op = mainStatement[counter];
        int i = 0;
        if (op == 0) {
            return left;
        }
        while (last[i] != 0) {
            if (op == last[i]) {
                return left;
            }
            i++;
        }
        // In case of seeing an operation with different precedence, return the left part
        if (getOper(op) != precedence) {
            return left;
        }
        counter++;
        // Solving the right part
        right = solveOp(precedence + 1, last);
        if (error) { return 0; }
        dismissblank();

        // Creating a new variable, which holds the result of the operation for LLVM IR
        char *newVar = getNewVar();
        char *rightSide = pop();
        char *leftSide = pop();
        char *ins = (char *) malloc(sizeof(char) * 257);
        // Writing the proper LLVM IR instruction and pushing it into the irstack
        if (op == '*') {
            left = left * right;
            sprintf(ins, "%s = mul i32 %s,%s", newVar, leftSide, rightSide);
        } else if (op == '+') {
            left = left + right;
            sprintf(ins, "%s = add i32 %s,%s", newVar, leftSide, rightSide);
        } else if (op == '-') {
            left = left - right;
            sprintf(ins, "%s = sub i32 %s,%s", newVar, leftSide, rightSide);
        } else if (op == '/') {
            left = left / right;
            sprintf(ins, "%s = sdiv i32 %s,%s", newVar, leftSide, rightSide);
        } else if (op == '&') {
            left = left & right;
            sprintf(ins, "%s = and i32 %s,%s", newVar, leftSide, rightSide);
        } else if (op == '|') {
            left = left | right;
            sprintf(ins, "%s = or i32 %s,%s", newVar, leftSide, rightSide);
        } else if (op == '%') {
            left = left % right;
            sprintf(ins, "%s = urem i32 %s,%s", newVar, leftSide, rightSide);
        } else {
            error = TRUE;
            return 0;
        }

        // Frees the popped variables
        irpush(ins);
        free(rightSide);
        free(leftSide);
        push(newVar);
    }
}

/// Gets the first and second parameters of a function
void initialize_sides(int *left, int *right) {
    if (mainStatement[counter] == '(') {
        counter++;
        dismissblank();
        *left = solveExpr(lastforfunc);
        if (error) { return; }
        if (mainStatement[counter] != ',') {
            error = TRUE;
            return;
        }
        counter++;
        *right = solveExpr(lastforparanthesis);
        if (error) { return; }
        if (mainStatement[counter] != ')') {
            error = TRUE;
            return;
        }
        counter++;
    } else {
        error = TRUE;
        return;
    }
}

/// This returns us %(variableCounter) as a string
char *getNewVar() {
    char *tempvar = calloc(12, sizeof(char));
    variableCounter++;
    tempvar[0] = '%';
    toString(tempvar + 1, variableCounter);
    return tempvar;
}

/// Turns a number into its string form
void toString(char *str, int num) {
    int i = 0;
    char ablacim[257];
    while (num > 0) {
        ablacim[i] = (num % 10) + '0';
        num /= 10;
        i++;
    }
    str[i] = '\0';
    int last = i;
    while (i > 0) {
        str[last - i] = ablacim[i - 1];
        i--;
    }
}