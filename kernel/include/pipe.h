#pragma once
#include <types.h>
#include <file.h>

int
pipealloc(struct file **, struct file **);
void
pipeclose(struct pipe *, int);
int
piperead(struct pipe *, char *, int);
int
pipewrite(struct pipe *, char *, int);
