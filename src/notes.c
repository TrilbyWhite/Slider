/*****************************************************\
  NOTES.C
  By: Jesse McClure (c) 2015
  See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

typedef struct NoteWin {
	Window win;
	cairo_t *ctx;
	int x, y, w, h, show, offset;
} NoteWin;

static NoteWin *_win = NULL;
static int _n = 0;

int notes_init(const char *fmt) {
	if (!fmt) return 1;
	/* allocate new NoteWin and read parameters */
	NoteWin *nw;
	_win = realloc(_win, (++_n) * sizeof(NoteWin));
	nw = &_win[_n - 1];
	sscanf(fmt, "%d:%d:%dx%d+%d+%d", &nw->show, &nw->offset, &nw->w, &nw->h, &nw->x, &nw->y)
	/* checks: */
	if (nw->show > 1) nw->show = 1;
	// TODO ...
	/* create window */
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.background_pixmap = ParentRelative;
	nw->win = XCreateWindow(dpy, root, nw->x, nw->y, nw->w, nw->h, 0, DefaultDepth(dpy,scr),
			InputOutput, DefaultVisual(dpy,scr), CWBackingStore | CWBackPixmap, &wa);
	cairo_surface_t *s = cairo_xlib_surface_create(dpy, nw->win,
			DefaultVisual(dpy,scr), nw->w, nw->h);
	nw->ctx = cairo_create(s);
	cairo_surface_destroy(s);
	return 0;
}

int notes_free() {
	int i;
	for (i = 0; i < _n; ++i) {
		cairo_destroy(_win[i].ctx);
		XDestroyWindow(dpy, _win[i].win);
	}
	free(_win);
	return 0;
}

int notes_draw() {
	return 0;
}

