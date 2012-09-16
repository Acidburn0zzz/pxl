
/*
pxl - ppm image viewer
Written in 2012 by <Olga Miller> <olga.rgb@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#ifndef IMAGE_H
#define IMAGE_H

struct pixel
{
	unsigned char red, green, blue;
};

struct image 
{
	struct pixel* pixels;
	int w, h;
};

#endif
