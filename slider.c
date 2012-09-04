/*************************************************************************\
| Slider                                                                  |
| ======                                                                  |
| by Jesse McClure <jesse@mccluresk9.com>, Copyright 2012                 |
|     Inspired by Impressive, coded from scratch                          |
| license: GPLv3                                                          |
\*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <poppler.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>


typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const char *);
	const char *arg;
} Key;

static void buttonpress(XEvent *);
static void die(const char *, ...);
static void draw(const char *);
static void fullscreen(const char *);
static void keypress(XEvent *);
static void move(const char *);
static void mute(const char *);
static void pen(const char *);
static void quit(const char *);
static void overview(const char *);
static void zoom(const char *);

static Display *dpy;
static int scr;
static Window root,win;
static GC gc,wgc,hgc,lgc;
static Bool running;
static int sw,sh,asp;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress]		= buttonpress,
	[KeyPress]			= keypress
};
static Bool netwm;
static Cursor invisible_cursor;
static Cursor highlight;

static PopplerDocument *pdf;
static Bool cancel_render = False;
static Bool white_muted = False;
static Bool overview_mode = False;
static Bool fullscreen_mode = True;
static Bool presenter_mode = False;
static char *uri;
struct {
	Pixmap *slide;
	int count, num, rendered;
	float scale;
} show;
struct {
	Pixmap view;
	int rendered, w, h, grid;
	float scale;
} sorter;

#include "config.h"

void buttonpress(XEvent *e) {
	XRaiseWindow(dpy,win);
	XSetInputFocus(dpy,win,RevertToPointerRoot,CurrentTime);
	int x,y,n;
	if (overview_mode) {
		if (e->xbutton.button == 1 || e->xbutton.button == 3) {
			x = (e->xbutton.x - (sw-sorter.grid*(sorter.w+10))/2)/(sorter.w + 10);
			y = (e->xbutton.y - 10) / (sorter.h + 10);
			n = y*sorter.grid+x;
			if (n <= show.rendered) {
				show.num = n;
				overview(NULL);
			}
		}
		if (e->xbutton.button == 2 || e->xbutton.button == 3)
			draw(NULL);
	}
	else {
		if (e->xbutton.button == 1) move("r");
		else if (e->xbutton.button == 2) overview(NULL);
		else if (e->xbutton.button == 3) move("l");
	}
}

void command_line(int argc, const char **argv) {
	if (argc == 1) die("%s requires a filename or uri for a pdf\n",argv[0]);
	int i;
	for (i = 1; i < argc - 1; i++) {
		if (argv[i][0] && argv[i][1] != '\0') {
			if (argv[i][1] == 'f') fullscreen_mode = False;
			else if (argv[i][1] == 'p') presenter_mode = True;
			else fprintf(stderr,"unrecognized parameter \"%s\" ignored.\n",argv[i]);
		}
		else fprintf(stderr,"unrecognized parameter \"%s\" ignored.\n",argv[i]);
	}	
	char *fullpath = realpath(argv[i],NULL);
	if (fullpath == NULL) die("Cannot find file \"%s\"\n",argv[i]);
	uri = (char *) calloc(strlen(fullpath) + 8,sizeof(char));
	strcpy(uri,"file://");
	strcat(uri,fullpath);
	free(fullpath);
}

void die(const char *msg, ...) {
	va_list arg;
	va_start(arg,msg);
	vfprintf(stderr,msg,arg);
	va_end(arg);
	exit(1);
}

void draw(const char *arg) {
	if (presenter_mode)  { fprintf(stdout,"SLIDE: %d\n",show.num); fflush(stdout); }
	XDefineCursor(dpy,win,invisible_cursor);
	if (white_muted || overview_mode) mute("black");
	XCopyArea(dpy,show.slide[show.num],win,gc,0,0,asp,sh,(sw-asp)/2,0);
	XFlush(dpy); /* or XSync(dpy,True|False); */
}

void fullscreen(const char *arg) {
	static x,y,w,h;
	XWindowAttributes xwa;
	XSetWindowAttributes attr;
	XEvent xev;
	Atom state = XInternAtom(dpy, "_NET_WM_STATE", False);
	Atom full = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	if (netwm) {	/* NET_WM   !! Experimental !! */
		xev.xclient.type=ClientMessage;
		xev.xclient.serial = 0;
		xev.xclient.send_event=True;
		xev.xclient.window=win;
		xev.xclient.message_type=state;
		xev.xclient.format=32;
		xev.xclient.data.l[0] = 2;
		xev.xclient.data.l[1] = full;
		xev.xclient.data.l[2] = 0;
		XSendEvent(dpy,root, False, SubstructureRedirectMask |
				SubstructureNotifyMask, &xev);
		XSync(dpy,False);
		usleep(10000);
	}
	else { /* not NET_WM */
		if ( (fullscreen_mode=!fullscreen_mode) ) {
			XGetWindowAttributes(dpy,win, &xwa);
			x = xwa.x; y = xwa.y; w = xwa.width; h = xwa.height;
			attr.override_redirect = True;
			XChangeWindowAttributes(dpy,win,CWOverrideRedirect,&attr);
			XMoveResizeWindow(dpy,win,0,0,sw,sh);
			XRaiseWindow(dpy,win);
		}
		else {
			attr.override_redirect = False;
			XMoveResizeWindow(dpy,win,x,y,w,h);
			XChangeWindowAttributes(dpy,win,CWOverrideRedirect,&attr);
		}
	}
	draw(NULL);
}
	

void keypress(XEvent *e) {
	unsigned int i;
	XKeyEvent *ev = &e->xkey;
	KeySym keysym = XkbKeycodeToKeysym(dpy,(KeyCode)ev->keycode,0,0);
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
		if ( (keysym == keys[i].keysym) && keys[i].func && keys[i].mod  == ((ev->state&~Mod2Mask)&~LockMask) )
				keys[i].func(keys[i].arg);
}

void move(const char *arg) {
	if (overview_mode) {
		if (arg[0] == 'r') show.num++;
		else if (arg[0] == 'l') show.num--;
		else if (arg[0] == 'd') show.num+=sorter.grid;
		else if (arg[0] == 'u') show.num-=sorter.grid;	
		if (show.num < 0) show.num = 0;
		if (show.num > show.rendered) show.num = show.rendered;
		overview(NULL);
	}
	else {
		if (arg[0] == 'd' || arg[0] == 'r') show.num++;
		else if (arg[0] == 'u' || arg[0] == 'l') show.num--;
		if (show.num < 0) show.num = 0;
		if (show.num > show.rendered) show.num = show.rendered;
		draw(NULL);
	}
}

void mute(const char *color) {
	if (presenter_mode) fprintf(stdout,"MUTE: %s\n",color);
	if (color[0] == 'w') { /* white */
		XFillRectangle(dpy,win,wgc,0,0,sw,sh);
		white_muted = True;
	}
	else { /* black */
		XFillRectangle(dpy,win,gc,0,0,sw,sh);
		white_muted = False;
		overview_mode = False;
	}
	XFlush(dpy);
}

void overview(const char *arg) {
	if (presenter_mode) fprintf(stdout,"OVERVIEW: %d\n",show.num);
	XDefineCursor(dpy,win,None);
	XLockDisplay(dpy); /* incase render thread is still working */
	XCopyArea(dpy,sorter.view,win,gc,0,0,sw,sh,0,0);
	XUnlockDisplay(dpy);
	/* calculate coordinates for highlighter box */
	/* TOOD: there has to be a better way of calculating these coordinates! */
	int i, n = 0, x = (sw-sorter.grid*(sorter.w+10))/2, y = 10;
	for (i = 0; i < show.num; i++) {
		x += sorter.w + 10;
		if (++n == sorter.grid) {
			n = 0;
			x = (sw-sorter.grid*(sorter.w+10))/2;
			y += sorter.h + 10;
		}
	}
	/* draw the highlighter box */
	XDrawRectangle(dpy,win,hgc,x-1,y-1,sorter.w+2,sorter.h+2);
	XFlush(dpy);
	overview_mode = True;
}

void pen(const char *arg) {
	if (presenter_mode) fprintf(stdout,"PEN\n");
	XDefineCursor(dpy,win,None);
	XEvent ev;
	int x,y,nx,ny;
	char pw[3] = "  "; pw[0] = arg[0]; pw[1] = arg[1];
	Colormap cmap = DefaultColormap(dpy,scr);
	XColor color;
	XAllocNamedColor(dpy,cmap,arg+3,&color,&color);
	XGCValues val;
	val.foreground = color.pixel;
	val.line_width = atoi(pw);
	GC pgc = XCreateGC(dpy,win,GCForeground|GCLineWidth,&val);
	for (;;) {
		while ( !XNextEvent(dpy,&ev) && ev.type!=ButtonPress && ev.type!=KeyPress );
		if (ev.type == KeyPress) {
			XPutBackEvent(dpy,&ev);
			break;
		}
		XGrabPointer(dpy,ev.xbutton.window,True,
			PointerMotionMask | ButtonReleaseMask, GrabModeAsync,
			GrabModeAsync,None,None,CurrentTime);
		x = ev.xbutton.x; y = ev.xbutton.y;
		while ( !XNextEvent(dpy,&ev) && ev.type != ButtonRelease) {
			nx = ev.xmotion.x; ny = ev.xmotion.y;
			XDrawLine(dpy,win,pgc,x,y,nx,ny);
			x = nx; y = ny;
			XFlush(dpy);
		}
		XUngrabPointer(dpy,CurrentTime);
	}
	XDefineCursor(dpy,win,invisible_cursor);
	/* force a redraw of the background in case pens went outside of the slide */
	white_muted = True;
}

void quit(const char *arg) { running=False; }

void warn() {
	if (presenter_mode) fprintf(stdout,"WARN\n");
	XDrawRectangle(dpy,win,hgc,(sw-asp)/2+2,2,asp-4,sh-4);
	XFlush(dpy);
	usleep(150000);
	XCopyArea(dpy,show.slide[show.num],win,gc,0,0,asp,sh,(sw-asp)/2,0);
}

void zoom(const char *arg) {
	if (show.rendered < show.count - 1) { warn(); return; } /* ensure rendering is done */
	XDefineCursor(dpy,win,None); // Make crosshair cursor 
	XEvent ev;
	int x1,y1,x2=-1,y2;
	while ( !XNextEvent(dpy,&ev) && ev.type!=ButtonPress && ev.type!=KeyPress );
	if (ev.type == KeyPress) {
		XPutBackEvent(dpy,&ev);
		XDefineCursor(dpy,win,invisible_cursor);
		return;
	}
	XGrabPointer(dpy,ev.xbutton.window,True,
		PointerMotionMask | ButtonReleaseMask, GrabModeAsync,
		GrabModeAsync,None,None,CurrentTime);
	x1 = ev.xbutton.x; y1 = ev.xbutton.y;
	while ( !XNextEvent(dpy,&ev) && ev.type != ButtonRelease && ev.type!=KeyPress ) {
		XCopyArea(dpy,show.slide[show.num],win,gc,0,0,asp,sh,(sw-asp)/2,0);
		XDrawRectangle(dpy,win,hgc,x1,y1,ev.xbutton.x-x1,ev.xbutton.y-y1);
		XSync(dpy,True);
	}
	if (ev.type == KeyPress) {
		XPutBackEvent(dpy,&ev);
		XDefineCursor(dpy,win,invisible_cursor);
		return;
	}
	x2 = ev.xbutton.x; y2 = ev.xbutton.y;
	mute("black");
	white_muted = True;
	Pixmap region = XCreatePixmap(dpy,root,sw,sh,DefaultDepth(dpy,scr));
	XFillRectangle(dpy,region,wgc,0,0,sw,sh);
	PopplerPage *page = poppler_document_get_page(pdf,show.num);
	cairo_surface_t *target = cairo_xlib_surface_create(
			dpy, region, DefaultVisual(dpy,scr), sw, sh);
	cairo_t *cairo = cairo_create(target);
	double xscale = show.scale * sw / (x2 - x1);
	double yscale = show.scale * sh / (y2-y1);
	double scale = (xscale > yscale ? yscale : xscale);
	cairo_scale(cairo,scale,scale);
	cairo_translate(cairo, ((sw-asp)/2-x1)/show.scale, -y1/show.scale );
	poppler_page_render(page,cairo);
	cairo_surface_destroy(target);
	cairo_destroy(cairo);
	XCopyArea(dpy,region,win,gc,0,0,sw,sh,
		(sw-(x2-x1)*scale/show.scale)/2,
		(sh-(y2-y1)*scale/show.scale)/2	);
	XFreePixmap(dpy,region);
	XDefineCursor(dpy,win,invisible_cursor);
	XFlush(dpy);
}

void *render_all(void *arg) {
/* render_all runs as an independent thread and quits when it is done */
	/* initalize data elements */
	pdf = poppler_document_new_from_file((char *)uri,NULL,NULL);
	if (!pdf) die("%s is not a recognizable pdf file\n",uri);
	show.count = poppler_document_get_n_pages(pdf);
	show.slide = (Pixmap *) calloc(show.count,sizeof(Pixmap));
	PopplerPage *page;
	cairo_surface_t *target;
	cairo_t *cairo;
	/* calculations for overview/sorter mode */
	page = poppler_document_get_page(pdf,0);
	double pdfw,pdfh;
	poppler_page_get_size(page,&pdfw,&pdfh);
	show.scale = sh / pdfh;
	asp = sh * pdfw/pdfh;
	sorter.grid = (int) sqrt(show.count) + 1;
	sorter.h = (sh-10)/sorter.grid - 10;
	sorter.w = sorter.h * pdfw/pdfh;
	sorter.scale = show.scale * sorter.h / sh;
	Pixmap thumbnail = XCreatePixmap(dpy,root,
		sorter.w,sorter.h,DefaultDepth(dpy,scr));

	/* create empty overview frame in sorter.view (dotted outlines for slides) */
	XFillRectangle(dpy,sorter.view,gc,0,0,sw,sh);
	int i,j, n=0, x=(sw-sorter.grid*(sorter.w+10))/2, y=10;
	for (i = 0; i < sorter.grid; i++, y += sorter.h + 10) {
		for (j = 0; j < sorter.grid; j++, x += sorter.w + 10) {
			if (++n > show.count) break;
			XDrawRectangle(dpy,sorter.view,lgc,x+2,y+2,sorter.w-5,sorter.h-5);
		}
		x = (sw-sorter.grid*(sorter.w+10))/2;
	}
	/* create show.slide[] renderings and update sorter.view with each page */
	n = 0; x = (sw-sorter.grid*(sorter.w+10))/2; y = 10;
	for (i = 0; i < show.count; i++) {
		/* show.slide[i] creation and rendering */
		show.slide[i] = XCreatePixmap(dpy,root,asp,sh,DefaultDepth(dpy,scr));
		XFillRectangle(dpy,show.slide[i],wgc,0,0,asp,sh);
		page = poppler_document_get_page(pdf,i);
		target = cairo_xlib_surface_create(
				dpy, show.slide[i], DefaultVisual(dpy,scr), asp, sh);
		cairo = cairo_create(target);
		cairo_scale(cairo,show.scale,show.scale);
		poppler_page_render(page,cairo);
		cairo_surface_destroy(target);
		cairo_destroy(cairo);
		/* sorter.view updating */
		XFillRectangle(dpy,thumbnail,wgc,0,0,sorter.w,sorter.h);
		target = cairo_xlib_surface_create(
			dpy,thumbnail, DefaultVisual(dpy,scr),sorter.w, sorter.h);
		cairo = cairo_create(target);
		cairo_scale(cairo,sorter.scale,sorter.scale);
		poppler_page_render(page,cairo);
		cairo_surface_destroy(target);
		cairo_destroy(cairo);
		XCopyArea(dpy,thumbnail,sorter.view,gc,0,0,sorter.w,sorter.h,x,y);
		/* increment show.rendered and calculate coordinates for next loop */
		show.rendered = i;
		if (cancel_render) break;
		x += sorter.w + 10;
		if (++n == sorter.grid) {
			n = 0;
			x = (sw-sorter.grid*(sorter.w+10))/2;
			y += sorter.h + 10;
		}
	}
	XFreePixmap(dpy,thumbnail);
	if (presenter_mode) fprintf(stdout,"RENDERING COMPLETE\n");
}

int main(int argc, const char **argv) {
	/* process command line arguments */
	command_line(argc,argv);

	/* some ugly but needed initializations */
	XInitThreads();
	g_type_init();

	/* open X connection & create window */
	if (!(dpy= XOpenDisplay(NULL))) die("Failed opening X display\n");
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	sw = DisplayWidth(dpy,scr);
	sh = DisplayHeight(dpy,scr);
	win = XCreateSimpleWindow(dpy,root,0,0,sw,sh,1,0,0);
	/* set attributes and map */
	XStoreName(dpy,win,"Slider");
	XSetWindowAttributes wa;
	wa.event_mask =  ExposureMask|KeyPressMask|ButtonPressMask|StructureNotifyMask;
	XChangeWindowAttributes(dpy,win,CWEventMask,&wa);

/* EXPERIMENTAL!  for presentor mode */
Atom slider_atom;
if (presenter_mode) {
	slider_atom = XInternAtom(dpy,"SLIDER_PRESENTATION",False);
	XSetSelectionOwner(dpy,slider_atom,win,CurrentTime);
	XFlush(dpy);
	fprintf(stdout,"START: set SLIDER_PRESENTATION atom");
	fflush(stdout);
}
/* end experimental */

	XMapWindow(dpy, win);

	/* check for EWMH compliant WM */
	Atom type, NET_CHECK = XInternAtom(dpy,"_NET_SUPPORTING_WM_CHECK",False);
	Window *wins;
	int fmt;
	unsigned long after,nwins;
	XGetWindowProperty(dpy,root,NET_CHECK,0,UINT_MAX,False,XA_WINDOW,
		&type,&fmt,&nwins,&after,(unsigned char**)&wins);
	if ( type == XA_WINDOW && nwins > 0 && wins[0] != None) netwm = True;
	else netwm = False;
	XFree(wins);

	/* set up Xlib graphics contexts */
	XGCValues val;
	XColor color;
	Colormap cmap = DefaultColormap(dpy,scr);
		/* black, background, default */
	val.foreground = BlackPixel(dpy,scr);
	gc = XCreateGC(dpy,root,GCForeground,&val);
		/* white, mute_white */
	val.foreground = WhitePixel(dpy,scr);
	wgc = XCreateGC(dpy,root,GCForeground,&val);
		/* dotted lines for loading slides in overview mode */
	XAllocNamedColor(dpy,cmap,colors[0],&color,&color);
	val.foreground = color.pixel;
	val.line_style = LineDoubleDash;
	lgc = XCreateGC(dpy,root,GCForeground|GCLineStyle,&val); /* lines */
		/* highlight box for slide overview */
	XAllocNamedColor(dpy,cmap,colors[1],&color,&color);
	val.foreground = color.pixel;
	val.line_width = 3;
	hgc = XCreateGC(dpy,root,GCForeground|GCLineWidth,&val);
		/* pixmap for overview */
	sorter.view = XCreatePixmap(dpy,root,sw,sh,DefaultDepth(dpy,scr));
		/* create cursor(s) */
	char curs_data = 0;
	Pixmap curs_map = XCreateBitmapFromData(dpy,win,&curs_data,1,1);
	invisible_cursor = XCreatePixmapCursor(dpy,curs_map,curs_map,&color,&color,0,0);
	XFreePixmap(dpy,curs_map);

	/* start rendering thread */
	pthread_t render_thread;
	pthread_create(&render_thread,NULL,render_all,NULL);
	/* wait for first frame to render */
	while (show.rendered < 1) usleep(50000);
	if (fullscreen_mode) { fullscreen_mode = ! fullscreen_mode; fullscreen(NULL); }
	XDefineCursor(dpy,win,invisible_cursor);

	/* main loop */
	draw(NULL);
	XEvent ev;
	running = True;
	while ( running && ! XNextEvent(dpy, &ev) )
		if (handler[ev.type]) handler[ev.type](&ev);	

	/* clean up */
	if (presenter_mode) {
		printf("END\n");
		fflush(stdout);
		XSetSelectionOwner(dpy,slider_atom,None,CurrentTime);
		XFlush(dpy);
	}
	if (show.rendered < show.count) { /* quiting before render thread ended */
		cancel_render = True;	/* give thread time to quit */
		usleep(250000);
	}
	int i;
	for (i = 0; i < show.rendered; i++)
		XFreePixmap(dpy,show.slide[i]);
	XFreePixmap(dpy,sorter.view);
	free(show.slide);
	free(uri);
	XCloseDisplay(dpy);
	return 0;
}

