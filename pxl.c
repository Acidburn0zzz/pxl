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
#include "utils.h"
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

int last_cell_x;
int last_cell_y;

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

void set_pixel(int x, int y, uint32_t* fb, uint32_t rgb)
{
	int i = y * screen->w + x;

	if((0 <= x && x < screen->w) && (0 <= y && y < screen->h))
		fb[i] = rgb;
}

void draw_tile(int size, int x0, int y0, uint32_t color)
{	
	uint32_t* fb0 = (uint32_t*)screen->pixels + x0 + screen->w * y0;

	for(int y = 0; y < size; y++, fb0 += screen->w)
		for(int x = 0; x < size; x++)
			fb0[x] = color;
}

void draw()
{
	int step = scale + grid;

	// calc interval (a, b) for whole tiles in image coordinates
	int xa = (max(-offset_x, 0) + (step - 1)) / step;
	int ya = (max(-offset_y, 0) + (step - 1)) / step;
	int xb = (max(-offset_x, 0) + min(img.w * step, screen->w)) / step;
	int yb = (max(-offset_y, 0) + min(img.h * step, screen->h)) / step;

	SDL_FillRect(screen, 0, 0);

	uint32_t* pa = img.pixels + ya * img.w;

	for(int y = ya, y0 = ya * step + offset_y + grid; y < yb; y++, y0 += step, pa += img.w)
		for(int x = xa, x0 = xa * step + offset_x + grid; x < xb; x++, x0 += step)
			draw_tile(scale, x0, y0, pa[x]);

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

	int jump = scale + 1;
	int line = scale + 2;

	for(int i = 0; i < line; i++)
	{
		int x = last_cell_x + i;
		int y = last_cell_y;
		set_pixel(x, y, fb, rgb);
		set_pixel(x, y + jump, fb, rgb);

		x = last_cell_x;
		y = last_cell_y + i;
		set_pixel(x, y, fb, rgb);
		set_pixel(x + jump, y, fb, rgb);
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
			last_cell_x = x * step + offset_x;
			last_cell_y = y * step + offset_y;

			draw_grid_cell(0x00dddddd);
		}

		uint32_t p = img.pixels[y * img.w + x];
		set_caption(x, y, (p >> 16) & 255, (p >> 8) & 255, p & 255);
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
		offset_x = max_x / 2;
	else
		offset_x = min(max(new_x, max_x), 0);

	if(max_y >= 0)
		offset_y = max_y / 2;
	else
		offset_y = min(max(new_y, max_y), 0);

	last_cell_x += offset_x - old_x;
	last_cell_y += offset_y - old_y;
}

void redraw()
{
	set_offset(offset_x, offset_y);
	set_default_caption();

	draw();
}

void jump(int xrel, int yrel)
{
	int xstep = xrel * screen->w / 2;
	int ystep = yrel * screen->h / 2;

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

	last_cell_x = 0;
	last_cell_y = 0;

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
