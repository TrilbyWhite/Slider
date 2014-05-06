/*****************************************************\
* SLIDER.H
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#ifndef __SLIDER_H__
#define __SLIDER_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <math.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-ft.h>
#include <poppler.h>

#define NBUTTON	12

typedef struct Target Target;
typedef struct Show Show;
struct Target {
	Show *show;
	cairo_t *ctx;
	Window win;
	int w, h, offset;
};
struct Show {
	cairo_surface_t **slide;
	int nslides, w, h, cur, ntargets;
	char *uri;
	Show *notes;
	Target *target;
};

typedef struct Key {
	unsigned short int mod;
	KeySym keysym;
	const char *arg;
} Key;

typedef struct View {
	int x, y, w, h, show, offset;
} View;

typedef union Theme {
	struct { double x, y, w, h, r; };
	struct { double R, G, B, A, e; };
} Theme;

typedef struct Config {
	cairo_font_face_t *font;
	View *view;
	Key *key;
	int nviews, nkeys, font_size, fade, mon;
	const char *button[NBUTTON];
	const char *url_handler, *mov_handler, *aud_handler;
	Bool loop, interleave, lock_aspect, launch;
} Config;

Show *show;
FT_Library ftlib;
Config conf;

extern int config_init(const char *, const char *);
extern void die(const char *, ...);
extern int xlib_init(Show *);
extern int xlib_free();
extern int xlib_main_loop();
extern Show *render_init(const char *, const char *);
extern int render_free(Show *);

#endif /* __SLIDER_H__ */
