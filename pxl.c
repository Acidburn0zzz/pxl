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

struct pixel
{
	unsigned char red, green, blue;
};

struct image 
{
	struct pixel* pixels;
	int w, h;
};

SDL_Surface* screen;
int scale;
int grid;

int args_num;
char** args;
int curr_arg;

struct image img;
const char* file_name;

void showerr(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, fmt, args);
	fprintf(stderr, "%s\n", SDL_GetError());
}

void exiterr(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	showerr(fmt, args);
	exit(1);
}

void init_video(int w, int h)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		exiterr("SDL can not be initialized: ");
	
	screen = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);
	
	if(!screen)
		exiterr("Unable to set video: ");
	if(screen->format->BitsPerPixel != 32)
		exiterr("Could not init 32 bit format: ");
	if(screen->w != w || screen->h != h)
		exiterr("Couldnt get %dx%d Window.\n", w, h);
}

void init_keyboard()
{
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

void init_ui(int w, int h)
{
	init_video(w, h);
	init_keyboard();
}

void draw()
{
	uint32_t rgb_grid_w = 0x00000000;
	uint32_t* fb = (uint32_t*) screen->pixels;
	
	for (int i = 0; i < img.h; i++)
	{
		for (int j = 0; j < img.w; j++)
		{
			struct pixel p = img.pixels[i * img.w + j];
			uint32_t rgb = (p.red << 16) | (p.green << 8) | (p.blue);
		
			int w_pos = j * (scale + grid);
			int h_pos = i * (scale + grid);

			for(int k = 0; k < scale; k++)
			{
				int w_pos_k = w_pos + k;

				for(int l = 0; l < scale; l++)
				{
					fb[(h_pos + l) * screen->w + w_pos_k] = rgb;
				}
				
				if(grid)
				{
					rgb_grid_w = 0xffffffff - rgb_grid_w;
					fb[(h_pos + scale) * screen->w + w_pos_k] = rgb_grid_w;
				}
			}
			
			if(grid)
			{
				uint32_t rgb_grid_h = 0x00000000;
				
				for(int l = 0; l < scale; l++)
				{
					rgb_grid_h = 0xffffffff - rgb_grid_h;
					fb[(h_pos + l) * screen->w + (w_pos + scale)] = rgb_grid_h;
				}
			}
		} 
	}
}

//current = (current + argc + direction) % argc

void toScreen()
{
	SDL_Flip(screen);
}

void set_next_arg()
{
	curr_arg++;
	
	if(curr_arg == args_num)
		curr_arg = 1;
}

void set_prev_arg()
{
	curr_arg--;

	if(curr_arg == 0)
		curr_arg = args_num -1;

}

void set_curr_arg(int next)
{
	if(next)
		set_next_arg();
	else
		set_prev_arg();
}

int read_ppm()
{
	int c, rgb;

	file_name = args[curr_arg];	
	FILE* f = fopen(file_name, "r");
	
	if(!f)
	{
		showerr("File can not be read. ");
		return 0;
	}

	if(('P' != getc(f)) || ('6' != getc(f)))
	{
		fclose(f);
		showerr("not ppm p6 image.");
		return 0;
	}

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
	fprintf(stderr, "%d %d %d\n", integer[0], integer[1], integer[2]);
	if(!integer[0] && !integer[1] && !integer[2])
	{
		fclose(f);
		showerr("couldnt read image.");
		return 0;
	}
	img.w = integer[0];
	img.h = integer[1];
	rgb = integer[2];

	if(rgb != 255)
	{
		fclose(f);
		showerr("sorry, only 24bit images supported.");
		return 0;
	}
	
	size_t bytes = sizeof(struct pixel) * img.w * img.h;

	free(img.pixels);
	img.pixels = (struct pixel*)malloc(bytes);
	if(!img.pixels)
	{
		fclose(f);
		exiterr("Unable to allocate memory.");
		return 0;
	}

	if(fread(img.pixels, bytes, 1, f) != 1)
	{
		free(img.pixels);
		img.pixels = 0;
		fclose(f);
		showerr("Error loading image.");
		return 0;
	}

	fclose(f);
	return 1;
}

void update()
{
	int width = img.w * (scale + grid);
	int height = img.h * (scale + grid);

	if(!screen || (screen->w != width || screen->h != height))
		init_ui(width, height);
	
	SDL_WM_SetCaption(file_name, "pixelka");
	SDL_FillRect(screen, 0, 0);

	draw();
	toScreen();
}

void show_image(int next)
{	
	int i = 0;

	set_curr_arg(next);
	while(!read_ppm())
	{
		set_curr_arg(next);
		
		if(i == args_num)
			exiterr("No file is readable.");
		i++;
	}

	update();
}

void change_caption(int x, int y)
{
	char caption[100]; 
	
	int step = scale + grid;

	x /= step;
	y /= step;

	int i = y * img.w + x;
	struct pixel p = img.pixels[i];
	
	snprintf(caption, 100, "%s [%d x %d] (%d; %d; %d)", file_name, x, y, p.red, p.green, p.blue);
	SDL_WM_SetCaption(caption, "pixelka");
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
				change_caption(event.motion.x, event.motion.y);
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
						show_image(0);
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
		exiterr("No args.");

	img.pixels = 0;
	
	scale = 4;
	grid = 1;

	args_num = argc;
	args = argv;

	curr_arg = 0;
	show_image(1);

	for(;;)
	{
		handle_event();	
	}

	return 0;
}
