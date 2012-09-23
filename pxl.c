/*
pxl - ppm image viewer
Written in 2012 by <Olga Miller> <olga.rgb@googlemail.com>
To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.
You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include "image.h"
#include "reader.h"

char* icon = "pixelka"; //TODO: create icon

SDL_Surface* screen;
int scale;
int grid;

int args_num;
char** args;
int curr_arg;

struct image img;
char* file_name;

int x_pos;
int y_pos;

void exiterr(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, fmt, args);
	fprintf(stderr, "%s\n", SDL_GetError());
	exit(1);
}

void resize_video(int w, int h)
{
	screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);

	if(!screen)
		exiterr("Unable to set video.\n");
	if(screen->format->BitsPerPixel != 32)
		exiterr("Could not init 32 bit format.\n");
	if(screen->w != w || screen->h != h)
		exiterr("Could not get %dx%d window.\n", w, h);
}

void draw()
{
	uint32_t rgb_grid = 0;
	uint32_t* fb = (uint32_t*) screen->pixels;

	for (int i = 0; i < img.h; i++)
	{
		for (int j = 0; j < img.w; j++)
		{
			struct pixel p = img.pixels[i * img.w + j];
			uint32_t rgb = (p.red << 16) | (p.green << 8) | (p.blue);

			int w_pos = j * (scale + grid) + grid;
			int h_pos = i * (scale + grid) + grid;

			for(int k = 0; k < scale; k++)
			{
				int w_pos_k = w_pos + k;

				for(int l = 0; l < scale; l++)
				{
					fb[(h_pos + l) * screen->w + w_pos_k] = rgb;
				}

				if(grid)
					fb[(h_pos + scale) * screen->w + w_pos_k] = rgb_grid;
			}

			if(grid)
			{
				for(int l = 0; l < scale; l++)
				{
					fb[(h_pos + l) * screen->w + (w_pos + scale)] = rgb_grid;
				}
			}
		}
	}
}

void set_curr_arg(int direction)
{
	int n = args_num - 1;
	curr_arg = (curr_arg + n + direction) % n;
	file_name = args[curr_arg + 1];
}

void update()
{
	int width = img.w * (scale + grid) + grid;
	int height = img.h * (scale + grid) + grid;

	if(!screen || (screen->w != width || screen->h != height))
		resize_video(width, height);

	SDL_WM_SetCaption(file_name, icon);
	SDL_FillRect(screen, 0, 0);

	draw();

	x_pos = y_pos = 0;

	SDL_Flip(screen);
}

void show_image(int direction)
{
	int i = 0;

	set_curr_arg(direction);
	while(!read_ppm_P6(file_name, &img))
	{
		set_curr_arg(direction);

		if(i == args_num)
			exiterr("No file is readable.\n");
		i++;
	}

	update();
}

void draw_cell_grid(uint32_t rgb)
{
	uint32_t* fb = (uint32_t*) screen->pixels;
	int step = scale + grid;

	for(int i = 0; i <= step; i++)
	{
		fb[y_pos * screen->w + x_pos + i] = rgb;
		fb[(y_pos + step) * screen->w + x_pos + i] = rgb;

		fb[(y_pos + i) * screen->w + x_pos] = rgb;
		fb[(y_pos + i) * screen->w + x_pos + step] = rgb;
	}

}

void change(int x_mouse, int y_mouse)
{
	char caption[100];

	int step = scale + grid;

	int x = x_mouse / step;
	int y = y_mouse / step;

	if((0 <= x && x < img.w) && (0 <= y && y < img.h))
	{
		if(grid)
		{
			draw_cell_grid(0);

			x_pos = x * step;//TODO +grid
			y_pos = y * step;

			draw_cell_grid(0xffffffff);
			SDL_Flip(screen);
		}

		int i = y * img.w + x;
		struct pixel p = img.pixels[i];

		snprintf(caption, 100, "%s [%d x %d] (%d; %d; %d)", file_name, x, y, p.red, p.green, p.blue);
		SDL_WM_SetCaption(caption, icon);
	}
}

void handle_event()
{
	SDL_Event event;
	SDLKey sym;

	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_MOUSEMOTION:
				change(event.motion.x, event.motion.y);
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym)
				{
					case SDLK_g:
						grid ^= 1;
						update();
						break;
					case SDLK_SPACE:
						show_image(1);
						break;
					case SDLK_BACKSPACE:
						show_image(-1);
						break;
					case SDLK_q:
					case SDLK_ESCAPE:
						exit(0);
						break;
					default:
						sym = event.key.keysym.sym;
						if (SDLK_0 < sym && sym <= SDLK_9) {
							scale = sym - SDLK_0;
							update();
						}
						break;
				}
				break;
			case SDL_QUIT:
				exit(0);
				break;
			default:
				break;
		}
	}
}

int main(int argc, char** argv)
{
	if(argc <= 1)
		exiterr("No args.\n");

	img.pixels = 0;

	scale = 4;
	grid = 0;

	args_num = argc;
	args = argv;

	curr_arg = 0;

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		exiterr("SDL can not be initialized.\n");
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	show_image(1);

	for(;;)
	{
		handle_event();
		SDL_Delay(10);
	}

	return 0;
}
