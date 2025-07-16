#include <__intrinsics.h>
#include <math.h>

int
abs(int j)
{
	return j < 0 ? -j : j;
}

double
fabs(double x)
{
	return __fabs(x);
}

double
pow(double x, double y)
{
	return __builtin_pow(x, y);
}
