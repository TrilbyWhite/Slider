/*****************************************************\
  COMMAND.C
  By: Jesse McClure (c) 2015
  See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

static void _cursor(const char *);
static void _fullscreen(const char *);
static void _link(const char *);
static void _next(const char *);
static void _pan(const char *);
static void _prev(const char *);
static void _quit(const char *);
static void _redraw(const char *);
static void _sorter(const char *);
static void _zoom(const char *);

static Atom NET_WM_STATE, NET_WM_STATE_FULLSCREEN;
static void (*_cmd[LASTCommand]) (const char *) = {
	[cmdCursor]              = _cursor,
	[cmdFullscreen]          = _fullscreen,
	[cmdLink]                = _link,
	[cmdNext]                = _next,
	[cmdPan]                 = _pan,
	[cmdPrev]                = _prev,
	[cmdQuit]                = _quit,
	[cmdRedraw]              = _redraw,
	[cmdSorter]              = _sorter,
	[cmdZoom]                = _zoom,
};
static const char *_cmd_str[LASTCommand] = {
	[cmdCursor]              = "cursor",
	[cmdFullscreen]          = "fullscreen",
	[cmdLink]                = "link",
	[cmdNext]                = "next",
	[cmdPan]                 = "pan",
	[cmdPrev]                = "prev",
	[cmdQuit]                = "quit",
	[cmdRedraw]              = "redraw",
	[cmdSorter]              = "sorter",
	[cmdZoom]                = "zoom",
};

int command_str_to_num(const char *str) {
	int i;
	for (i = 1; i < LASTCommand && _cmd_str[i]; ++i)
		if (strncasecmp(str, _cmd_str[i], strlen(_cmd_str[i])) == 0)
			return i;
	return cmdNone;
}

int command(int cmd, const char *arg) {
	if (!_cmd[cmd]) return 1;
	_cmd[cmd](arg);
	return 0;
}

int command_init() {
	NET_WM_STATE = XInternAtom(dpy, "_NET_WM_STATE", false);
	NET_WM_STATE_FULLSCREEN = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", false);
	return 0;
}

int command_free() {
	return 0;
}


void _cursor(const char *arg) {
#ifdef module_cursor
	cursor_visible(toggle);
#else
	fprintf(stderr,"[SLIDER] Module \"cursor\" not included in this build.\n");
#endif
}

void _fullscreen(const char *arg) {
	static bool fs = false;
	if (!arg || arg[0] == 't') fs = !fs;
	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = true;
	ev.xclient.display = dpy;
	ev.xclient.window = topWin;
	ev.xclient.message_type = NET_WM_STATE;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 2;
	ev.xclient.data.l[1] = NET_WM_STATE_FULLSCREEN;
	ev.xclient.data.l[2] = 0;
	XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
}

void _link(const char *arg) {
	link_follow(0);
}

void _next(const char *arg) {
	++cur;
	_redraw(NULL);
}

static const int winMin = 120;
static const int winMax = 12000;
void _pan_zoom(int mode) {
	int x, y, xx, yy, dx, dy, ig;
	unsigned int w, h, uig;
	Window wig;
	XEvent ev;
	XQueryPointer(dpy, root, &wig, &wig, &xx, &yy, &ig, &ig , &uig);
	XGetGeometry(dpy, presWin, &wig, &x, &y, &w, &h, &uig, &uig);
	XGrabPointer(dpy, root, true, PointerMotionMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	while (true) {
		XMaskEvent(dpy, PointerMotionMask | ButtonReleaseMask, &ev);
		if (ev.type == ButtonRelease) break;
		dx = ev.xbutton.x_root - xx; xx = ev.xbutton.x_root;
		dy = ev.xbutton.y_root - yy; yy = ev.xbutton.y_root;
		XClearWindow(dpy, topWin);
		if (mode == 1) {
			XMoveWindow(dpy, presWin, x+=dx, y+=dy);
		}
		else if (mode == 3) {
			if ((w+=dx) > winMax) w = winMax;
			if (w < winMin) w = winMin;
			if ((h+=dy) > winMax) h = winMax;
			if (h < winMin) h = winMin;
			XResizeWindow(dpy, presWin, w, h);
			render_page(cur, presWin, true);
		}
		while (XCheckMaskEvent(dpy, PointerMotionMask, &ev));
	}
	XUngrabPointer(dpy, CurrentTime);
}

void _pan(const char *arg) {
	if (!arg) _pan_zoom(1);
}

void _prev(const char *arg) {
	--cur;
	_redraw(NULL);
}

void _quit(const char *arg) {
	running = false;
}

void _redraw(const char *arg) {
	if (arg && arg[0] == 'n') {
		int ig;
		unsigned int w, h, uig;
		Window wig;
		XGetGeometry(dpy, topWin, &wig, &ig, &ig, &w, &h, &uig, &uig);
		XMoveResizeWindow(dpy, presWin, 0, 0, w, h);
	}
	render_page(cur, presWin, false);
#ifdef module_sorter
	sorter_draw(cur);
#endif
	if (!arg) return;
	// TODO notes ...
}

void _sorter(const char *arg) {
#ifdef module_sorter
	if (!arg || arg[0] == 't') sorter_visible(toggle);
	else if (arg[0] == 's') sorter_visible(true);
	else if (arg[0] == 'h') sorter_visible(false);
	_redraw(NULL);
#else
	fprintf(stderr,"[SLIDER] Module \"sorter\" not included in this build.\n");
#endif
}

void _zoom(const char *arg) {
	if (!arg) _pan_zoom(3);
}

