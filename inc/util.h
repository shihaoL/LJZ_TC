#ifndef UTIL_H
#define UTIL_H
typedef char* string;
string String(string s);
typedef char bool;
#define TRUE 1
#define FALSE 0

void *checked_malloc(int n);
#endif