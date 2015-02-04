/*****************************************************\
* CONFIG.C
* By: Jesse McClure (c) 2015
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

static void _fullscreen(const char *);
static void _next(const char *);
static void _pan(const char *);
static void _prev(const char *);
static void _quit(const char *);
static void _redraw(const char *);
static void _zoom(const char *);

static Atom NET_WM_STATE, NET_WM_STATE_FULLSCREEN;
static void (*_cmd[LASTCommand]) (const char *) = {
	[cmdFullscreen]          = _fullscreen,
	[cmdNext]                = _next,
	[cmdPan]                 = _pan,
	[cmdPrev]                = _prev,
	[cmdQuit]                = _quit,
	[cmdRedraw]              = _redraw,
	[cmdZoom]                = _zoom,
};


int command(int cmd, const char *arg) {
	if (!_cmd[cmd]) return 1;
	_cmd[cmd](arg);
	return 0;
}

int command_init() {
	NET_WM_STATE = XInternAtom(dpy, "_NET_WM_STATE", false);
	NET_WM_STATE_FULLSCREEN = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", false);
}

int command_free() {
}




void _fullscreen(const char *arg) {
	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.display = dpy;
	ev.xclient.window = topWin;
	ev.xclient.message_type = NET_WM_STATE;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 2;
	ev.xclient.data.l[1] = NET_WM_STATE_FULLSCREEN;
	ev.xclient.data.l[2] = 0;
	XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
/*
	ev.xclient.message_type = NET_ACTIVE_WINDOW;
	ev.xclient.data.l[0] = 1;
	ev.xclient.data.l[1] = CurrentTime;
	ev.xclient.data.l[2] = win;
	XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
*/
}

void _next(const char *arg) {
	++cur;
	_redraw(NULL);
}

void _pan_zoom(int mode) {
	int x, y, xx, yy, dx, dy, ig;
	unsigned int w, h;
	Window wig;
	XEvent ev;
	XQueryPointer(dpy, root, &wig, &wig, &xx, &yy, &ig, &ig , &ig);
	XGetGeometry(dpy, presWin, &wig, &x, &y, &w, &h, &ig, &ig);
	XGrabPointer(dpy, root, true, PointerMotionMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	while (True) {
//		expose(ev);
		XMaskEvent(dpy, PointerMotionMask | ButtonReleaseMask, &ev);
		if (ev.type == ButtonRelease) break;
		dx = ev.xbutton.x_root - xx; xx = ev.xbutton.x_root;
		dy = ev.xbutton.y_root - yy; yy = ev.xbutton.y_root;
		if (mode == 1) XMoveWindow(dpy, presWin, x+=dx, y+=dy);
		else if (mode == 3) XResizeWindow(dpy, presWin, w+=dx, y+=dy);
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
	render_page(cur, presWin, false);
	sorter_draw(cur);
	if (!arg) return;
	// TODO notes ...
}

void _zoom(const char *arg) {
	if (!arg) _pan_zoom(3);
}

