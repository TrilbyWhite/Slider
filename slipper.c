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
#include <X11/extensions/Xrandr.h>

/*temporary*/
#define fact	2.0

static void draw();
static void buttonpress(XEvent *);
static void expose(XEvent *);

static Display *dpy;
static int scr;
static Window root,win,slider_win;
static FILE *slider;
static Pixmap current,preview,pix_current,pix_preview;
static GC gc;
static Bool running;
static int sw,sh,aw,ah;
static cairo_surface_t *target_current, *target_preview, *current_c, *preview_c;
static cairo_t *cairo_current, *cairo_preview;
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
	XCopyArea(dpy,current,pix_current,gc,0,0,sw,sh,0,0);
	cairo_set_source_surface(cairo_current,current_c,0,0);
	cairo_paint(cairo_current);
	if (preview != 0) {
		XCopyArea(dpy,preview,pix_preview,gc,0,0,sw,sh,0,0);
		cairo_set_source_surface(cairo_preview,preview_c,0,0);
		cairo_paint(cairo_preview);
	}
	XFlush(dpy);
}

int main(int argc, const char **argv) {
	if (argc == 1) exit(1);

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
	sprintf(cmd,"./slider -p -g %dx%d ",slider_w,slider_h);
	strcat(cmd,argv[1]);
	slider = popen(cmd,"r"); //TODO: send to correct screen
	char line[255];
	fgets(line,254,slider);
	while( strncmp(line,"SLIDER START",11) != 0) fgets(line,254,slider);
	sscanf(line,"SLIDER START (%dx%d)",&aw,&ah);
	fgets(line,254,slider);
	sscanf(line,"SLIDER: current=%lu, next=%lu",&current,&preview);
	pix_current = XCreatePixmap(dpy,root,aw,ah,DefaultDepth(dpy,scr));
	pix_preview = XCreatePixmap(dpy,root,aw,ah,DefaultDepth(dpy,scr));

	/* mirror slider output scaled down */
	target_current = cairo_xlib_surface_create(dpy,win,DefaultVisual(dpy,scr),sw,sh);
	target_preview = cairo_xlib_surface_create(dpy,win,DefaultVisual(dpy,scr),sw,sh);
	cairo_current = cairo_create(target_current);
	cairo_preview = cairo_create(target_preview);
	cairo_scale(cairo_current,sw/(aw*fact),sh/(ah*fact));
	cairo_scale(cairo_preview,sw/(aw*fact*2),sh/(ah*fact*2));
	cairo_translate(cairo_preview,sw-sw/(aw*fact*2)-4,4);
	current_c = cairo_xlib_surface_create(dpy,pix_current,DefaultVisual(dpy,scr),aw,ah);
	preview_c = cairo_xlib_surface_create(dpy,pix_preview,DefaultVisual(dpy,scr),aw,ah);

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
			if (strncmp(line,"SLIDER END",9)==0) {
				running = False;
			}
			sscanf(line,"SLIDER: current=%lu, next=%lu",&current,&preview);
			/* process commands */
			draw();
		}
	}
	cairo_surface_destroy(current_c);
	pclose(slider);

	XCloseDisplay(dpy);
	return 0;
}

