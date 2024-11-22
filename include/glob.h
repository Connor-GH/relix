#ifndef GLOB_H
#define GLOB_H
#include <stdint.h>



typedef struct {
	size_t gl_pathc;
	char **gl_pathv;
	size_t gl_offs;
} glob_t;

#endif  // GLOB_H
