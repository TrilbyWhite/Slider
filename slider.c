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

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const char *);
	const char *arg;
} Key;

static void buttonpress(XEvent *);
static void die(const char *, ...);
static void draw(const char *);
static void keypress(XEvent *);
static void move(const char *);
static void mute(const char *);
static void quit(const char *);
static void overview(const char *);

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

static PopplerDocument *pdf;
static Bool cancel_render = False;
static Bool white_muted = False;
static Bool overview_mode = False;
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
		if (e->xbutton.button == 1) {
			x = (e->xbutton.x - (sw-sorter.grid*(sorter.w+10))/2)/(sorter.w + 10);
			y = (e->xbutton.y - 10) / (sorter.h + 10);
			n = y*sorter.grid+x;
			if (n <= show.rendered) {
				show.num = n;
				overview(NULL);
			}
		}
		else if (e->xbutton.button == 2) draw(NULL);
		else if (e->xbutton.button == 3) { /* do nothing yet */ }
	}
	else {
		if (e->xbutton.button == 1) move("r");
		else if (e->xbutton.button == 2) overview(NULL);
		else if (e->xbutton.button == 3) move("l");
	}
}

void command_line(int argc, const char **argv) {
	if (argc == 1) die("%s requires a filename or uri for a pdf\n",argv[0]);
	char *fullpath = realpath(argv[1],NULL);
	if (fullpath == NULL) die("Cannot find file \"%s\"\n",argv[1]);
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
	if (white_muted || overview_mode) mute("black");
	XCopyArea(dpy,show.slide[show.num],win,gc,0,0,asp,sh,(sw-asp)/2,0);
	XFlush(dpy); /* or XSync(dpy,True|False); */
}

void keypress(XEvent *e) {
	unsigned int i;
	XKeyEvent *ev = &e->xkey;
	KeySym keysym = XkbKeycodeToKeysym(dpy,(KeyCode)ev->keycode,0,0);
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
		if ( (keysym == keys[i].keysym) && keys[i].func &&
				(keys[i].mod == ev->state & ~LockMask)  )
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
	
void quit(const char *arg) { running=False; }

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
	XSetWindowAttributes wa;
	wa.event_mask =  ExposureMask|KeyPressMask|ButtonPressMask|StructureNotifyMask;
	wa.override_redirect = True;
	// TODO: wa.cursor CWCursor
	XChangeWindowAttributes(dpy,win,CWEventMask|CWOverrideRedirect,&wa);
	XMapWindow(dpy, win);
	XSetInputFocus(dpy,win,RevertToPointerRoot,CurrentTime);
	
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

	/* start rendering thread */
	pthread_t render_thread;
	pthread_create(&render_thread,NULL,render_all,NULL);
	/* wait for first frame to render */
	while (show.rendered < 1) usleep(50000);

	/* main loop */
	draw(NULL);
	XEvent ev;
	running = True;
	while ( running && ! XNextEvent(dpy, &ev) )
		if (handler[ev.type]) handler[ev.type](&ev);	

	/* clean up */
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

