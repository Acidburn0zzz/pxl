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

struct image img;
char* filename;

int fb_dirty;

int x_grid_cell;
int y_grid_cell;

int offset_x;
int offset_y;

void exiterr(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, fmt, args);
	fprintf(stderr, "%s\n", SDL_GetError());
	exit(1);
}

void set_default_caption()
{
	char c[100];

	snprintf(c, 100, "%s zoom=%d", filename, scale);
	SDL_WM_SetCaption(c, icon);
}

void set_caption(int x, int y, int r, int g, int b)
{
	char c[100];

	snprintf(c, 100, "%s zoom=%d [%d x %d] (%d; %d; %d)", filename, scale, x, y, r, g, b);
	SDL_WM_SetCaption(c, icon);
}

void resize_video(int w, int h)
{
	if(!screen || (screen->w != w || screen->h != h))
	{
		if(screen)
			SDL_FreeSurface(screen);

		screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF);

		if(!screen)
			exiterr("Unable to set video.\n");
		if(screen->format->BitsPerPixel != 32)
			exiterr("Could not init 32 bit format.\n");
		if(screen->w != w || screen->h != h)
			exiterr("Could not get %dx%d window.\n", w, h);
	}

	set_default_caption();
}

void set_pixel(int x, int y, uint32_t* fb, int length, uint32_t rgb)
{
	int i = y * screen->w + x;

	if((0 <= i && i < length) && ((0 <= x && x < screen->w) && (0 <= y && y < screen->h)))
		fb[i] = rgb;
}

void get_color(int x, int y, uint32_t* color)
{
	if((0 <= x && x < img.w) && (0 <= y && y < img.h))
	{
		struct pixel p = img.pixels[y * img.w + x];
		*color = (p.red << 16) | (p.green << 8) | (p.blue);
	}
}

void draw()
{
	int step = scale + grid;
	uint32_t* fb = (uint32_t*) screen->pixels;

	int w = screen->w;
	int h = screen->h;

	for(int y = 0; y < h; y++)
	{
		int r_j = (y - offset_y - grid) % step;
		int j = (y - offset_y - grid) / step;

		for(int x = 0; x < w; x++)
		{
			int r_i = (x -  offset_x - grid) % step;
			int i = (x - offset_x - grid) / step;
			uint32_t rgb = 0x00000000;

			if(0 <= r_i && 0 <= r_j)
			{
				if(!grid || (r_i != scale && r_j != scale))
					get_color(i, j, &rgb);
			}

			fb[y * w + x] = rgb;
		}
	}

	fb_dirty = 1;
}

void set_filename(int direction)
{
	static int curr_arg = -1;

	if(direction != 1 && direction != -1)
		direction = 1;

	curr_arg = (curr_arg + args_num + direction) % args_num;
	filename = args[curr_arg + 1];
}

void read_image(int direction)
{
	int i = 0;

	set_filename(direction);

	while(!read_ppm_P6(filename, &img))
	{
		set_filename(direction);

		if(i == args_num)
			exiterr("No file is readable.\n");
		i++;
	}
}

void draw_grid_cell(uint32_t rgb)
{
	uint32_t* fb = (uint32_t*) screen->pixels;
	int length = screen->w * screen->h;

	int jump = scale + 1;
	int line = scale + 2;

	for(int i = 0; i < line; i++)
	{
		int x = x_grid_cell + i;
		int y = y_grid_cell;
		set_pixel(x, y, fb, length, rgb);
		set_pixel(x, y + jump, fb, length, rgb);

		x = x_grid_cell;
		y = y_grid_cell + i;
		set_pixel(x, y, fb, length, rgb);
		set_pixel(x + jump, y, fb, length, rgb);
	}

	fb_dirty = 1;
}

void change(int mouse_x, int mouse_y)
{
	int step = scale + grid;
	static int old_scale = 1;

	int x = (mouse_x - offset_x - grid) / step;
	int y = (mouse_y - offset_y - grid) / step;

	SDL_ShowCursor(grid ^ 1);

	if(grid && old_scale == scale)
		draw_grid_cell(0);
	old_scale = scale;

	if((0 <= x && x < img.w) && (0 <= y && y < img.h))
	{
		if(grid)
		{
			x_grid_cell = x * step + offset_x;
			y_grid_cell = y * step + offset_y;

			draw_grid_cell(0x00dddddd);
		}

		struct pixel p = img.pixels[y * img.w + x];
		set_caption(x, y, p.red, p.green, p.blue);
	}
	else 
	{
		SDL_ShowCursor(1);
		set_default_caption();
	}
}

void set_offset(int new_x, int new_y)
{
	int old_x = offset_x;
	int old_y = offset_y;

	int max_x = screen->w - (img.w * (scale + grid) + grid);	
	int max_y = screen->h - (img.h * (scale + grid) + grid);

	if(max_x >= 0)
		offset_x = (int)(max_x * 0.5f);
	else
		offset_x = fminf(fmaxf(new_x, max_x), 0);

	if(max_y >= 0)
		offset_y = (int)(max_y * 0.5);
	else
		offset_y = fminf(fmaxf(new_y, max_y), 0);

	x_grid_cell += offset_x - old_x;
	y_grid_cell += offset_y - old_y;
}

void redraw()
{
	set_offset(offset_x, offset_y);
	set_default_caption();

	draw();
}

void jump(int xrel, int yrel)
{
	int xstep = xrel * screen->w;
	int ystep = yrel * screen->h;

	set_offset(offset_x + xstep, offset_y + ystep);
	draw();
}

void handle_keydown(SDL_KeyboardEvent* event)
{
	SDLKey key = event->keysym.sym;

	switch(key)
	{
		case SDLK_LEFT:
			jump(1, 0);
			break;
		case SDLK_UP:
			jump(0, 1);
			break;
		case SDLK_RIGHT:
			jump(-1, 0);
			break;
		case SDLK_DOWN:
			jump(0, -1);
			break;
		case SDLK_g:
			grid ^= 1;
			redraw();
			break;
		case SDLK_SPACE:
			read_image(1);
			redraw();
			break;
		case SDLK_BACKSPACE:
			read_image(-1);
			redraw();
			break;
		case SDLK_q:
		case SDLK_ESCAPE:
			exit(0);
			break;

		default:
			if(SDLK_0 <= key && key <= SDLK_9)
			{
				scale = 1 << (key - SDLK_0);
				redraw();
			}
			else if(SDLK_KP0 <= key && key <= SDLK_KP9)
			{
				scale = 1 << (key - SDLK_KP0);
				redraw();
			}
			break;
	}
}

void handle_event()
{
	SDL_Event event;

	static int mousebuttonleft_down = 0;

	int mouse_x = -1;
	int mouse_y = -1;

	int mouse_xrel = 0;
	int mouse_yrel = 0;

	int w = 0;
	int h = 0;

	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_MOUSEMOTION:
				mouse_x = event.motion.x;
				mouse_y = event.motion.y;
				mouse_xrel += event.motion.xrel;
				mouse_yrel += event.motion.yrel;
				break;
			case SDL_KEYDOWN:
				handle_keydown(&event.key);
				break;
			case SDL_MOUSEBUTTONDOWN:
				if(event.button.button == SDL_BUTTON_LEFT)
					mousebuttonleft_down = 1;
				break;
			case SDL_MOUSEBUTTONUP:
				if(event.button.button == SDL_BUTTON_LEFT)
					mousebuttonleft_down = 0;
				break;
			case SDL_VIDEORESIZE:
				w = event.resize.w;
				h = event.resize.h;
				break;
			case SDL_QUIT:
				exit(0);
				break;
			default:
				break;
		}
	}

	if(w && h)
	{
		resize_video(w, h);
		redraw();
	}
	else
	{
		if(mousebuttonleft_down && (mouse_xrel || mouse_yrel))
		{
			set_offset(offset_x + mouse_xrel, offset_y + mouse_yrel);
			draw();
		}

		if(mouse_x != -1 && mouse_y != -1)
			change(mouse_x, mouse_y);
	}
}

int main(int argc, char** argv)
{
	if(argc <= 1)
		exiterr("No args.\n");

	img.pixels = 0;

	scale = 1;
	grid = 0;

	x_grid_cell = 0;
	y_grid_cell = 0;

	offset_x = 0;
	offset_y = 0;

	fb_dirty = 0;

	args_num = argc - 1;
	args = argv;

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		exiterr("SDL can not be initialized.\n");
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

	read_image(1);
	resize_video(640, 480);
	set_offset(0, 0);
	draw();

	for(;;)
	{
		handle_event();

		if(fb_dirty)
		{
			SDL_Flip(screen);
			fb_dirty = 0;
		}

		SDL_Delay(10);
	}

	return 0;
}
