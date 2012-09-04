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

	/* set up Xlib graphics contexts */
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
	while ( (slider_atom=XInternAtom(dpy,"SLIDER_PRESENTATION",True))==None ) usleep(200000);
	Window slider = XGetSelectionOwner(dpy,slider_atom);
	if (slider) XCopyArea(dpy,slider,win,gc,0,0,sw,sh,0,0);
	else printf("no initial win\n");
	XFlush(dpy);
	XSetInputFocus(dpy,slider,RevertToPointerRoot,CurrentTime);
	while ( fgets(line,254,stdin) ) {
		/* TODO: scale down the following copy and display notes*/
		/* TODO: lots of other stuff here */
		if (strncmp(line,"END",3) == 0) break;
		if (slider) XCopyArea(dpy,slider,win,gc,0,0,sw,sh,0,0);
		else printf("no win\n");
		XFlush(dpy);
	}
	XCloseDisplay(dpy);
	return 0;
}

