#pragma once

#include <string.h>
#include "standart.h"

// Our hashmap entries (defined as a new struct with fields as key and value)
typedef struct HashMapEntry {
    char *key;
    int value;
    bool8 isdefined; /// for undefined variables
} HashMapEntry;

// Our Hashmap
typedef struct HashMap {
    HashMapEntry *array;
    int fullness;
    int size;
} HashMap;

HashMap initializeHashMap(); // allocates hashmap's array
void add_new_element(HashMap *hashMap, char * key, int value); // adds new element to the given hashmap
int getValue(HashMap *, char *); // gets the value of given string
void deconstractor(HashMap *); // deconstructs the Hashmap
int chadstrcmp(char *, char*); // compares strings
bool8 isAllocated(HashMap *,char *);
