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
#include <X11/extensions/Xrandr.h>

/*temporary*/
#define fact	3.0

static void draw();
static void buttonpress(XEvent *);
static void expose(XEvent *);

static Display *dpy;
static int scr;
static Window root,win,slider_win;
static FILE *slider;
static Pixmap mirror;
static GC gc;
static Bool running;
static int sw,sh;
static cairo_surface_t *target, *slider_c;
static cairo_t *cairo;
static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress]	= buttonpress,
	[Expose]		= expose,
};

void buttonpress(XEvent *ev) {
	draw();
	XSetInputFocus(dpy,slider_win,RevertToPointerRoot,CurrentTime);
}

void expose(XEvent *ev) { draw(); }

void draw() {
	XCopyArea(dpy,slider_win,mirror,gc,0,0,sw,sh,0,0);
	cairo_set_source_surface(cairo,slider_c,0,0);
	cairo_paint(cairo);
	XFlush(dpy);
}

int main(int argc, const char **argv) {
	/* TODO: process command line arguments & get notes file */

	/* open X connection & create window */
	if (!(dpy= XOpenDisplay(NULL))) exit(1);
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	sw = DisplayWidth(dpy,scr); // gets wrong width with external display
	sh = DisplayHeight(dpy,scr);
	win = XCreateSimpleWindow(dpy,root,0,0,sw,sh,1,0,0);
	/* set attributes and map */
	XStoreName(dpy,win,"Slipper");
	XSetWindowAttributes wa;
	//wa.override_redirect = True;
	//XChangeWindowAttributes(dpy,win,CWOverrideRedirect,&wa);
	XMapWindow(dpy, win);
	mirror = XCreatePixmap(dpy,root,sw,sh,DefaultDepth(dpy,scr));
	int num_sizes;
	XRRScreenSize *xrrs = XRRSizes(dpy,0,&num_sizes);
	int slider_w = xrrs[0].width;
	int slider_h = xrrs[0].height;

	/* set up Xlib graphics context(s) */
	XGCValues val;
	XColor color;
	Colormap cmap = DefaultColormap(dpy,scr);
		/* black, background, default */
	val.foreground = BlackPixel(dpy,scr);
	gc = XCreateGC(dpy,root,GCForeground,&val);

	/* connect to slider */
	char *cmd = (char *) calloc(32+strlen(argv[1]),sizeof(char));
	sprintf(cmd,"slider -p -g %dx%d ",slider_w,slider_h);
	strcat(cmd,argv[1]);
	slider = popen(cmd,"r"); //TODO: send to correct screen
	char line[255];
	fgets(line,254,slider);
	while( strncmp(line,"START",5) != 0) fgets(line,254,slider);
	Atom slider_atom;
	while ( (slider_atom=XInternAtom(dpy,"SLIDER_PRESENTATION",True))==None )
		usleep(200000);
	slider_win = XGetSelectionOwner(dpy,slider_atom);
	if (!slider_win) {
		fprintf(stderr,"Could not connect to slider\n");
		exit(1);
	}

	/* mirror slider output scaled down */
	target = cairo_xlib_surface_create(dpy,win,DefaultVisual(dpy,scr),sw,sh);
	cairo = cairo_create(target);
	cairo_scale(cairo,sw/(slider_w*fact),sh/(slider_h*fact));
	slider_c = cairo_xlib_surface_create(dpy,mirror,DefaultVisual(dpy,scr),sw,sh);

	XEvent ev;
	int xfd,sfd,r;
	struct timeval tv;
	fd_set rfds;
	xfd = ConnectionNumber(dpy);
	sfd = fileno(slider);
	char *ret = line;
	running = True;
	while (running) {
		memset(&tv,0,sizeof(tv));
		tv.tv_usec=500000; /* 0.5 sec */
		FD_ZERO(&rfds);
		FD_SET(xfd,&rfds);
		FD_SET(sfd,&rfds);
		r = select(sfd+1,&rfds,0,0,&tv);
		if (FD_ISSET(xfd,&rfds)) while (XPending(dpy)) { /* x input */
			XNextEvent(dpy,&ev);
			if (handler[ev.type]) handler[ev.type](&ev);
		}
		else if (FD_ISSET(sfd,&rfds)) { /* slider input */
			fgets(line,254,slider);
			if (strncmp(line,"END",3)==0) {
				running = False;
			}
			/* process commands */
			draw();
		}
	}
	cairo_surface_destroy(slider_c);
	pclose(slider);

	XCloseDisplay(dpy);
	return 0;
}

