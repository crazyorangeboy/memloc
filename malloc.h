#ifndef MALLOC_H
#define MALLOC_H

#include <unistd.h>

void *malloc(unsigned int size);
void free(void *p);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

#endif
