/*****************************************************\
  XLIB.C
  By: Jesse McClure (c) 2015
  See slider.c or COPYING for license information
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
	scr = DefaultScreen(dpy);
	root = DefaultRootWindow(dpy);
#ifdef module_randr
	randr_init();
#else
	set(presX, 0); set(presY, 0);
	set(presW, DisplayWidth(dpy,scr));
	set(presH, DisplayHeight(dpy,scr));
#endif /* moudle_randr */
	XSetWindowAttributes wa;
	wa.background_pixel = 0x000000;
	wa.backing_store = Always;
	wa.event_mask = ButtonPressMask | KeyPressMask | PointerMotionMask;
	topWin = XCreateWindow(dpy, root, get_d(presX), get_d(presY), get_d(presW),
			get_d(presH), 0, DefaultDepth(dpy,scr), InputOutput,
			DefaultVisual(dpy,scr), CWBackPixel | CWBackingStore | CWEventMask, &wa);
	presWin = XCreateWindow(dpy, topWin, 0, 0, get_d(presW), get_d(presH), 0,
			DefaultDepth(dpy,scr), InputOutput, DefaultVisual(dpy,scr),
			CWBackingStore | CWEventMask, &wa);
	XStoreName(dpy, topWin, "Slider");
	XMapWindow(dpy, topWin); // TODO where is it?
	XMapWindow(dpy, presWin);

	/* other init functions */
	if (command_init()) return xlib_free(1);
	if (render_init(get_s(presFile))) return xlib_free(2);
	if (cursor_init(presWin)) return xlib_free(3);
	if (sorter_init(topWin)) return xlib_free(4);
	render_set_fader(presWin, 20);
	command(cmdFullscreen, NULL);
	return 0;
}

int xlib_free(int ret) {
	sorter_free();
	cursor_free();
	render_free();
	command_free();
	XDestroyWindow(dpy, topWin);
	return ret;
}

int xlib_mainloop() {
	XEvent ev;
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

void _buttonpress(XEvent *ev) {
	XButtonEvent *e = &ev->xbutton;
	config_bind_exec(e->state, e->button, None);
}

void _keypress(XEvent *ev) {
	XKeyEvent *e = &ev->xkey;
	config_bind_exec(e->state, 0, e->keycode);
}

void _motionnotify(XEvent *ev) {
	XMotionEvent *e = &ev->xmotion;
	cursor_draw(e->x, e->y);
}

