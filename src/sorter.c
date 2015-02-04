/*****************************************************\
* RENDER.C
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"


static Window _win;
static int _cur, *_x;
static bool _visible = false;
static Window *_wins = NULL;
static Window parent = None;

bool sorter_event(XEvent *ev) {
	/* is it a sorter window event: */
	int i;
	if (!_wins) return false;
	for (i = 0; _wins[i] != None; ++i)
		if (ev->xany.window == _wins[i]) break;
	if (_wins[i] == None) return false;

	//if (ev->type == Expose)
	//	sorter_draw();
	if (ev->type == MotionNotify) {
	}
	if (ev->type == ButtonPress) {
		if (ev->xbutton.button == 1) {
			render_page(cur=i, presWin, false);
			sorter_draw(cur);
		}
		else if (ev->xbutton.button == 3) sorter_visible(false);
		else if (ev->xbutton.button == 4) sorter_draw(--_cur);
		else if (ev->xbutton.button == 5) sorter_draw(++_cur);
		//else if (ev->xbutton.button == 6)
		//else if (ev->xbutton.button == 7)
	}
	return true;
}

int sorter_draw(int set) {
	int i;
	int w = 180;
	int x = 640 - w/2; // sw/2 - w/2
	static int last;
	if (!_wins) {
		_wins = render_create_sorter(parent);
		for (i = 0; _wins[i] != None; ++i);
		last = i;;
		_x = malloc(i * sizeof(int));
		_x[0] = 0;
		for (i = 1; i < last; ++i)
			_x[i] = _x[i-1] + w/i;
		--last;
	}
_cur = set;
if (_cur < 0) _cur = 0;
if (_cur > last) _cur = last;
	if (_visible) {
		for (i = 0; _wins[i] != None; ++i) {
			XMoveWindow(dpy, _wins[i], x + (i < _cur ? -1 : 1) * _x[abs(_cur-i)],
					600 + pow(abs(_cur-i),1.6));
			XMapRaised(dpy, _wins[i]);
		}
		for (--i; i >= _cur; --i) XRaiseWindow(dpy, _wins[i]);
	}
	else {
		for (i = 0; _wins[i] != None; ++i)
			XUnmapWindow(dpy, _wins[i]);
	}
	return 0;
}

int sorter_init(Window win) {
	parent = win;
	return 0;
}

int sorter_free() {
	if (!_wins) return 0;
	if (_x) free(_x);
	int i;
	for (i = 0; _wins[i] != None; ++i)
		XDestroyWindow(dpy, _wins[i]);
	free(_wins);
	return 0;
}

bool sorter_visible(bool set) {
	if (set == query) return _visible;
	if (set == toggle) _visible = !_visible;
	else _visible = set;
	if (_visible) {
		cursor_visible(false);
		XUndefineCursor(dpy, topWin);
	}
	else {
		cursor_visible(true);
		cursor_visible(false);
	}
	//sorter_draw(cur);
	return _visible;
}

