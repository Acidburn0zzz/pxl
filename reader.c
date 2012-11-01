/*
pxl - ppm image viewer
Written in 2012 by <Olga Miller> <olga.rgb@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include "reader.h"

struct pixel
{
	unsigned char red, green, blue;
};

int read_ppm_P6(const char* filename, struct image *img)
{
	int c, rgb;

	FILE* f = fopen(filename, "r");
	
	if(!f)
	{
		fprintf(stderr, "File \"%s\" can not be opened.\n", filename);
		return 0;
	}

	if(('P' != getc(f)) || ('6' != getc(f)))
	{
		fclose(f);
		fprintf(stderr, "File \"%s\" is not ppm P6 image.\n", filename);
		return 0;
	}

	c = getc(f);

	int integer[3] = { 0, 0, 0};
	for (int i = 0; i < 3; i++) {
		while('#' == (c = getc(f)))
			while(getc(f) != '\n');

		while ((c < '0') || ('9' < c))
			c = getc(f);

		char buff[16];
		for (int pos = 0; pos < 16; pos++) {
			if (('0' <= c) && (c <= '9'))
			{
				buff[pos] = c;
				c = getc(f);
			} else {
				buff[pos] = 0;
				break;
			}
		}
		integer[i] = atoi(buff);
	}
	
	if(!integer[0] && !integer[1] && !integer[2])
	{
		fclose(f);
		fprintf(stderr, "Could not read image \"%s\".\n", filename);
		return 0;
	}
	img->w = integer[0];
	img->h = integer[1];
	rgb = integer[2];

	if(rgb != 255)
	{
		fclose(f);
		fprintf(stderr, "Sorry, can not read image \"%s\". Only 24bit images supported.\n", filename);
		return 0;
	}

	free(img->pixels);
	
	size_t bytes = sizeof(struct pixel) * img->w * img->h;
	struct pixel* pixels = (struct pixel*)malloc(bytes);

	if(!pixels)
	{
		fclose(f);
		fprintf(stderr, "Unable to allocate memory for %dx%d image \"%s\".\n", img->w, img->h, filename);
		return 0;
	}

	if(fread(pixels, bytes, 1, f) != 1)
	{
		free(pixels);
		img->pixels = 0;
		fclose(f);
		fprintf(stderr, "Error loading image \"%s\".\n", filename);
		return 0;
	}

	fclose(f);

	img->pixels = (uint32_t*)malloc(sizeof(uint32_t) * img->w * img->h);
	for (int i = 0; i < img->w * img->h; i++)
		img->pixels[i] = (pixels[i].red << 16) | (pixels[i].green << 8) | (pixels[i].blue);

	free(pixels);

	return 1;
}

