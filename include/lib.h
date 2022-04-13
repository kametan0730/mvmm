#ifndef MVMM_LIB_H
#define MVMM_LIB_H

#include "types.h"

void* memcpy(void* dst, const void* src, u64 n);

void* memmove(void* dst, const void* src, u64 n);

void* memset(void* dst, int c, u64 n);

int strcmp(const char* s1, const char* s2);

u64 strlen(const char* s);

char* strcpy(char* dst, const char* src);

#define offsetof(st, m)   ((u64)&(((st *)0)->m))

#endif
