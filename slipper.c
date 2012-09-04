/*************************************************************************\
| Slipper                                                                 |
| =======                                                                 |
| by Jesse McClure <jesse@mccluresk9.com>, Copyright 2012                 |
| license: GPLv3                                                          |
\*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static Display *dpy;
static int scr;
static Window root,win;
static GC gc;
static int sw,sh;

int main(int argc, const char **argv) {
	/* TODO: process command line arguments & get notes file */

	/* open X connection & create window */
	if (!(dpy= XOpenDisplay(NULL))) exit(1);
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	sw = DisplayWidth(dpy,scr);
	sh = DisplayHeight(dpy,scr);
	win = XCreateSimpleWindow(dpy,root,0,0,sw,sh,1,0,0);
	/* set attributes and map */
	XStoreName(dpy,win,"Slipper");
	XSetWindowAttributes wa;
	//wa.override_redirect = True;
	//XChangeWindowAttributes(dpy,win,CWOverrideRedirect,&wa);
	XMapWindow(dpy, win);
	Pixmap mirror = XCreatePixmap(dpy,root,sw,sh,DefaultDepth(dpy,scr));

	/* set up Xlib graphics context(s) */
	XGCValues val;
	XColor color;
	Colormap cmap = DefaultColormap(dpy,scr);
		/* black, background, default */
	val.foreground = BlackPixel(dpy,scr);
	gc = XCreateGC(dpy,root,GCForeground,&val);

	/* connect to slider */
	char line[255];
	fgets(line,254,stdin);
	while( strncmp(line,"START",5) != 0) fgets(line,254,stdin);
	Atom slider_atom;
	while ( (slider_atom=XInternAtom(dpy,"SLIDER_PRESENTATION",True))==None )
		usleep(200000);
	Window slider = XGetSelectionOwner(dpy,slider_atom);
	if (!slider) exit(1);
	// TODO get slider w and h
	int slider_w=sw,slider_h=sh;
	float fact = 3;

	XSetInputFocus(dpy,slider,RevertToPointerRoot,CurrentTime);

	/* mirror slider output scaled down */
	char *ret = line;
	cairo_surface_t *target, *slider_c;
	cairo_t *cairo;
	target = cairo_xlib_surface_create(dpy,win,DefaultVisual(dpy,scr),sw,sh);
	cairo = cairo_create(target);
	//cairo_scale(cairo,sw/(slider_w*fact),sh/(slider_h*fact));
	cairo_scale(cairo,sw/(slider_w*fact),sh/(slider_h*fact));
	slider_c = cairo_xlib_surface_create(dpy,mirror,DefaultVisual(dpy,scr),sw,sh);

	while ( ret != NULL && strncmp(line,"END",3 != 0) ) {
		XCopyArea(dpy,slider,mirror,gc,0,0,sw,sh,0,0);
		cairo_set_source_surface(cairo,slider_c,0,0);
		cairo_paint(cairo);
		XSync(dpy,True);
		ret = fgets(line,254,stdin);
	}
	cairo_surface_destroy(slider_c);

	XCloseDisplay(dpy);
	return 0;
}

