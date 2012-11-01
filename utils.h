/*
pxl - ppm image viewer
Written in 2012 by <Olga Miller> <olga.rgb@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

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

int ilog2(int v)
{
	int r = 0;
	while(v >>= 1)
		r++;

	return r;
}

#endif
