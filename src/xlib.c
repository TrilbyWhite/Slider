/*****************************************************\
* XLIB.C
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

static void _buttonpress(XEvent *);
static void _keypress(XEvent *);
static void _motionnotify(XEvent *);
static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress]     = _buttonpress,
	[KeyPress]        = _keypress,
	[MotionNotify]    = _motionnotify,
};

int xlib_init() {
	dpy = XOpenDisplay(0x0);
	scr = DefaultScreen(dpy);
	root = DefaultRootWindow(dpy);
	XSetWindowAttributes wa;
	wa.background_pixel = 0x000000;
	wa.backing_store = Always;
#ifdef module_randr
	randr_init();
#else
	set(presX, 0); set(presY, 0);
	set(presW, DisplayWidth(dpy,scr));
	set(presH, DisplayHeight(dpy,scr));
#endif /* moudle_randr */
	topWin = XCreateWindow(dpy, root, get_d(presX), get_d(presY), get_d(presW),
			get_d(presH), 0, DefaultDepth(dpy,scr), InputOutput,
			DefaultVisual(dpy,scr), CWBackingPixel, &wa);
	wa.event_mask = ButtonPressMask | KeyPressMask | PointerMotionMask;
	presWin = XCreateWindow(dpy, topWin, 0, 0, get_d(presW), get_d(presH), 0,
			DefaultDepth(dpy,scr), InputOutput, DefaultVisual(dpy,scr),
			CWBackingStore | CWEventMask, &wa);
	XMapWindow(dpy, topWin); // TODO where is it?
	XMapWindow(dpy, presWin);

	/* other init functions */
	if (render_init(get_s(presFile))) return xlib_free(1);
	if (cursor_init(presWin)) return xlib_free(2);
	if (sorter_init(topWin)) return xlib_free(3);
	render_set_fader(presWin, 15);
	return 0;
}

int xlib_free(int ret) {
	sorter_free();
	cursor_free();
	render_free();
	XDestroyWindow(dpy, topWin);
	XCloseDisplay(dpy);
	return ret;
}

int xlib_mainloop() {
	XEvent ev;
cur = 0;
	running = true;
	render_page(cur, presWin, false);
	while (running && !XNextEvent(dpy,&ev)) {
		if (sorter_event(&ev)) continue;
		//if (notes_event(&ev)) continue;
		if (ev.type < LASTEvent && handler[ev.type])
			handler[ev.type](&ev);
		XSync(dpy,true);
	}
}

static const int minWindow = 40;
static const int maxWindow = 10000;
void _buttonpress(XEvent *ev) {
	XButtonEvent *e = &ev->xbutton;
	if (e->state & ControlMask) { /* TODO cursor stuff */
		if (e->button == 1) cursor_visible(toggle);
		else if (e->button == 3) { } // TODO follow link
		else if (e->button == 2) running = false; // command("quit");
		else return;
	}
	else if (e->state & ShiftMask) { /* Move / Resize-Zoom */
		int x, y, ig, mode = 0; unsigned int w, h;
		XGetGeometry(dpy, presWin, (Window *)&ig, &x, &y, &w, &h, &ig, &ig);
		//if (e->button == 4) { x-=40, y-=40, w+=80, h+=80; }
		//else if (e->button == 5) { x+=40, y+=40, w-=80, h-=80; }
		if (e->button == 1) command(cmdPan, NULL);
		else if (e->button == 2) { /* TODO reset win size */ }
		else if (e->button == 3) command(cmdZoom, NULL);
		else return;
		if (mode) { // TODO drag move/resize
		}
		if (w < minWindow) w = minWindow;
		else if (w > maxWindow) w = maxWindow;
		if (h < minWindow) h = minWindow;
		else if (h > maxWindow) h = maxWindow;
		if (x + w < minWindow) x = minWindow - w;
		else if (x > 800) x = 800; // TODO
		if (y + h < minWindow) y = minWindow - h;
		else if (y > 800) y = 800; // TODO
		XMoveResizeWindow(dpy, presWin, x, y, w, h);
	}
	else { /* Basic navigation */
		if (e->button == 1) command(cmdNext, NULL);
		else if (e->button == 2) sorter_visible(toggle);
		else if (e->button == 3) command(cmdPrev, NULL);
		else if (e->button == 4) { } //TODO history back
		else if (e->button == 5) { } //TODO history forward
		else return;
	}
}

void _keypress(XEvent *ev) {
	XKeyEvent *e = &ev->xkey;
	//running = false;
	//render_page(++cur, presWin, false);
	//sorter_draw(cur);
}

void _motionnotify(XEvent *ev) {
	XMotionEvent *e = &ev->xmotion;
	cursor_draw(e->x, e->y);
}

