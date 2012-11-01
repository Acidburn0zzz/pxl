#ifndef UTILS_H
#define UTILS_H

int max(int a, int b)
{
	return (a > b) ? a : b;
}

int min(int a, int b)
{
	return (a < b) ? a : b;
}

int clamp(int v, int min, int max)
{
	int tmp = (v < min) ? min : v;
	return (tmp > max) ? max : tmp;
}

#endif
