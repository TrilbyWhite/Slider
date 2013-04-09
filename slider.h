

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <poppler.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>

#define RENDERED	0x0001

enum { Black, White, ScreenBG, SlideBG, Empty, Highlight, Warn };

typedef struct Show Show;
struct Show {		// CREATE/FREE
	Pixmap *slide;	// render.c
	int *flag;		// render.c
	Show *sorter;	// slider.c
	Show *notes;	// slider.c
	int x, y, w, h, count, cur;
	float scale;
	char *uri;		// slider.c
};

void die(const char *,...);
void render(Show *);
void free_renderings(Show *);
GC cgc(int);

Display *dpy;
int scr, sw, sh, swnote, shnote;
int prerender;
Window root;
GC gc;


