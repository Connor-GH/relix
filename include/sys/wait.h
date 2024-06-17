#pragma once

int
wait(int *status);
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
