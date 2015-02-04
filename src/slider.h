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
#include <stdint.h>
#include <unistd.h>
#include <locale.h>
#include <math.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <poppler.h>

typedef unsigned short int bool;
enum { false=0, true=1, toggle=2, query=3 };
enum {
	noteX, noteY, noteFile,
	presX, presY, presW, presH, presFile,
	self,
	videoOut,
	LASTVar,
};
enum {
	cmdFullscreen,
	cmdNext,
	cmdPan,
	cmdPrev,
	cmdRedraw,
	cmdQuit,
	cmdZoom,
	LASTCommand,
};
typedef union {
	int d;
	float f;
	const char *s;
} config_union;

#define set(x,y)		config_set(x,(config_union)y)


Display *dpy;
int scr, cur;
Window root, topWin, presWin;
bool running;

/* config.c */
int config_init();
int config_free();
void config_set(int, config_union);
int get_d(int);
float get_f(int);
const char *get_s(int);

/* cursor.c */
int cursor_draw(int, int);
int cursor_init(Window);
int cursor_free();
int cursor_set_size(int, int);
int cursor_set_size_relative(int, int);
bool cursor_visible(bool);
bool cursor_suspend(bool);

/* randr.c */
#ifdef module_randr
int randr_init();
int randr_free();
#endif /* module randr */

/* render.c */
int render_init();
int render_free();
int render_page(int, Window, bool);
int render_set_fader(Window, int);
Window *render_create_sorter(Window);

/* sorter.c */
int sorter_draw(int);
int sorter_init(Window);
bool sorter_event(XEvent *);
int sorter_free();
int sorter_set_offset(int);
int sorter_set_offset_relative(int);
bool sorter_visible(bool);

/* xlib.c */
int xlib_init();
int xlib_free(int);
int xlib_mainloop();

#endif /* __SLIDER_H__ */
