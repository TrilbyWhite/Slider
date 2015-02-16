/*****************************************************\
  CURSOR.C
  By: Jesse McClure (c) 2015
  See slider.c or COPYING for license information
\*****************************************************/

/*
TODO
cursor_img_from_file(const char *fname);
cursor_img_dot(rgba col);
*/

#include "slider.h"

enum { _min_w = 5, _min_h = 5, _max_w = 127, _max_h = 127 };

static int _check_size();
static int _img_dummy();

static Window _win;
static cairo_t *_ctx;
static uint8_t _w = 41, _h = 41, _cx = 21, _cy = 21;
static cairo_surface_t *_img;
static uint8_t _img_w, _img_h, _img_cx, _img_cy;
static bool _visible = false;

static Cursor no_cursor;

int cursor_draw(int x, int y) {
	static int _x, _y;
	if (x != -1) { _x = x; _y = y; }
	if (!_visible) return 0;
	XMoveWindow(dpy, _win, _x - _cx, _y - _cy);
	XClearWindow(dpy, _win);
	cairo_set_source_surface(_ctx, _img, 0, 0);
	cairo_paint(_ctx);
	return 0;
}

int cursor_free() {
	_visible = false;
	cairo_destroy(_ctx);
	cairo_surface_destroy(_img);
	XFreeCursor(dpy, no_cursor);
	XDestroyWindow(dpy, _win);
	return 0;
}

int cursor_init(Window parent) {
	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.background_pixmap = ParentRelative;
	_win = XCreateWindow(dpy, parent, 0, 0, _max_w, _max_h, 0, DefaultDepth(dpy,scr),
			InputOutput, DefaultVisual(dpy,scr), CWBackingStore | CWBackPixmap, &wa);
	/* create empty mouse cursor */
	XColor color;
	char cdat = 0;
	Pixmap cmap = XCreateBitmapFromData(dpy, parent, &cdat, 1, 1);
	no_cursor = XCreatePixmapCursor(dpy, cmap, cmap, &color, &color, 0, 0);
	XFreePixmap(dpy, cmap);
	XDefineCursor(dpy, topWin, no_cursor);

	cairo_surface_t *s = cairo_xlib_surface_create(dpy, _win,
			DefaultVisual(dpy,scr), _max_w, _max_h);
	_ctx = cairo_create(s);
	cairo_surface_destroy(s);
	_img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, _max_w, _max_h);
	_img_w = _max_w; _img_h = _max_h;
	_img_cx = (_max_w + 1) / 2; _img_cy = (_max_h + 1) / 2;
_check_size();
_img_dummy();
	return 0;
}

int cursor_set_size(int w, int h) {
	_w = w;
	_h = h;
	return _check_size();
}

int cursor_set_size_relative(int dw, int dh) {
	_w += dw;
	_h += dh;
	return _check_size();
}

bool cursor_visible(bool set) {
	if (set == query) return _visible;
	if (set == toggle) _visible = !_visible;
	else _visible = set;
	if (_visible) {
		XDefineCursor(dpy, topWin, no_cursor);
		XMapWindow(dpy, _win);
	}
	else {
		XUnmapWindow(dpy, _win);
	}
	return _visible;
}



int _check_size() {
	int w = _w, h = _h;
	if (_w < _min_w) _w = _min_w;
	if (_h < _min_h) _h = _min_h;
	if (_w > _max_w) _w = _max_w;
	if (_h > _max_h) _h = _max_h;
	XResizeWindow(dpy, _win, _w, _h);
	double sx = _w / (double) _img_w, sy = _h / (double) _img_w;
	_cx = _img_cx * sx; _cy = _img_cy * sy;
	cairo_identity_matrix(_ctx);
	cairo_scale(_ctx, sx, sy);
	/* return success only if all checks passed */
	if (_w == w && _h == h) return 0;
	return 1;
}

int _img_dummy() {
	_img_cx = (_max_w + 1) / 2;
	_img_cy = (_max_h + 1) / 2;
	cairo_t *c = cairo_create(_img);
	cairo_arc(c, _img_cx, _img_cy, _img_cx - 2 , 0, 4 * M_PI);
	cairo_set_source_rgba(c, 0, 0, 1, 0.20);
	cairo_fill_preserve(c);
	cairo_set_source_rgba(c, 0, 0, 1, 0.95);
	cairo_stroke(c);
	cairo_set_source_rgba(c, 0, 1, 0, 0.95);
//cairo_set_font_size(c, 64);
//cairo_move_to(c, 0, _img_cy + 32);
//cairo_show_text(c, "Hello");
	cairo_destroy(c);
	return 0;
}

