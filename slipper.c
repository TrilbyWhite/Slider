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


static void buttonpress(XEvent *);
static void command_line(int,const char**);
static void draw();
static void expose(XEvent *);
static void usage(int);

static FILE *dat;
static Bool useXrandr;
static time_t start_time;
static float fact=0.7,pfact=0.4;
static char fontstring[120] =
	"-xos4-terminus-bold-r-normal--22-220-72-72-c-110-iso8859-2";
static char msg[32];
static int cur_slide, last_slide;
static char *pdf = NULL,*video1=NULL,*video2=NULL;
static int duration=600; /* 10 min = 600 seconds */
static Display *dpy;
static int scr;
static Window root,win,slider_win;
static FILE *slider;
static Pixmap current,preview,pix_current,pix_preview,buffer;
static GC gc,pgc,ngc;
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

void command_line(int argc, const char **argv) {
	int i;
	char datfile[255] = "";
	char xrand[10] = "on";
	char pdfname[255] = "";
	char tv1[255] = "LVDS1";
	char tv2[255] = "VGA1";
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			/* switches */
		}
		else
			strncpy(datfile,argv[i],254);
	}
	if (strlen(datfile) < 3) {
		usage(1);
	}
	char *ext = strchr(datfile,'.');
	if (ext && strncmp(ext,".pdf",4)==0) strcpy(pdfname,datfile);
	dat = fopen(datfile,"r");
	int bad_commands=0;
	if (strlen(pdfname) < 2) while (fgets(datfile,254,dat)) {
		if ( (datfile[0] == '#') || (datfile[0] == '\n') )
			continue;
		if ( strncmp(datfile,"slide",5) == 0 )
			break;
		if (	! (sscanf(datfile,"set duration %d\n",&duration)) 		&&
				! (sscanf(datfile,"set xrandr %s\n",xrand))				&&
				! (sscanf(datfile,"set window %dx%d\n",&sw,&sh))		&&
				! (sscanf(datfile,"set outputs %s %s\n",tv1,tv2))		&&
				! (sscanf(datfile,"set current view %f\n",&fact))		&&
				! (sscanf(datfile,"set next view %f\n",&pfact))			&&
				! (sscanf(datfile,"set font %s\n",fontstring))			&&
				! (sscanf(datfile,"set pdf %s\n",pdfname))			)
			fprintf(stderr,"(%d) ignoring unrecognized command \"%s\"\n",
				++bad_commands,datfile);
		if (bad_commands > 9) {
			fprintf(stderr,"Found %d bad commands in input file.",bad_commands);
			fprintf(stderr,"This is not likely a propper slipper data file.\n");
			exit(1);
		}
	}
	if (strncmp(xrand,"on",2) == 0) useXrandr = True;
	else useXrandr = False;
	if (strlen(tv1) < 2 || strlen(tv2) < 2) useXrandr = False;
	else {
		video1 = (char *) calloc(strlen(tv1)+1,sizeof(char));
		video2 = (char *) calloc(strlen(tv2)+1,sizeof(char));
		strcpy(video1,tv1);
		strcpy(video2,tv2);
	}
	if (strlen(pdfname) > 2) {
		pdf = (char *) calloc(strlen(pdfname)+1,sizeof(char));
		strcpy(pdf,pdfname);
	}
}

void expose(XEvent *ev) { draw(); }

void draw() {
	XFillRectangle(dpy,buffer,gc,0,0,sw,sh);
	XCopyArea(dpy,current,pix_current,gc,0,0,aw-1,ah,0,0);
	cairo_set_source_surface(cairo_current,current_c,0,0);
	cairo_paint(cairo_current);
	if (preview != 0) {
		XFillRectangle(dpy,buffer,pgc,sw-sw*pfact-8,
			sh-sh*pfact-8,sw*pfact+8,sh*pfact+8);
		XCopyArea(dpy,preview,pix_preview,gc,0,0,aw,ah,0,0);
		cairo_set_source_surface(cairo_preview,preview_c,0,0);
		cairo_paint(cairo_preview);
	}
	static char line[255];
	int i;
	fseek(dat,0,SEEK_SET);
	for (i = 0; i <= cur_slide; i++)
		while (fgets(line,254,dat))
			if ( strncmp(line,"slide {",7)==0 ) break;
	i = sh*fact;
	while (fgets(line,254,dat)) {
		if (line[0] == '#' || line[0] == '\n') continue;
		if (line[0] == '}') break;
		line[strlen(line)-1]='\0';
		XDrawString(dpy,buffer,ngc,10,(i+=20),line,strlen(line));
	}
	int used_time = time(NULL) - start_time;
	if (used_time > duration) used_time = duration;
	char stat_string[100];
	sprintf(stat_string,"Slide %d/%d, Time: %d/%d",
		cur_slide,last_slide,used_time,duration);
	XDrawString(dpy,buffer,ngc,sw*fact+10,20,"SLIDES",6);
	XDrawRectangle(dpy,buffer,pgc,sw*fact+10,30,sw-sw*fact-20,20);
	XFillRectangle(dpy,buffer,pgc,sw*fact+10,30,
		(sw-sw*fact-20)*cur_slide/last_slide,20);
	XDrawString(dpy,buffer,ngc,sw*fact+10,80,"TIME",4);
	XDrawRectangle(dpy,buffer,pgc,sw*fact+10,90,sw-sw*fact-20,20);
	XFillRectangle(dpy,buffer,pgc,sw*fact+10,90,
		(sw-sw*fact-20)*used_time/duration,20);
	if (strlen(msg) > 1)
		XDrawString(dpy,buffer,ngc,sw*fact+10,200,msg,strlen(msg));
	XCopyArea(dpy,buffer,win,gc,0,0,sw,sh,0,0);
	XFlush(dpy);
}

static void usage(int exit_code) {
	fprintf(stderr,"\nSlipper\n=======\nby Jesse McClure, copyright 2012\n");
	fprintf(stderr,"License: GPLv3\n\nUSAGE: slipper <datafile>\n\nsee ");
	fprintf(stderr,"`man slipper` for more information.\n\n");
	if (exit_code) exit(exit_code);
}

int main(int argc, const char **argv) {
	/* open X connection & create window */
	if (!(dpy= XOpenDisplay(NULL))) exit(1);
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	sw = DisplayWidth(dpy,scr);
	sh = DisplayHeight(dpy,scr);

	command_line(argc,argv);
	win = XCreateSimpleWindow(dpy,root,0,0,sw,sh,1,0,0);
	/* set attributes and map */
	XStoreName(dpy,win,"Slipper");
	XSetWindowAttributes wa;
	if (useXrandr) {
		wa.override_redirect = True;
		XChangeWindowAttributes(dpy,win,CWOverrideRedirect,&wa);
	}
	XMapWindow(dpy, win);
	/* set up screens */
	char *cmd;
	if (useXrandr) {
		cmd = (char *) calloc(60+strlen(video1)+strlen(video2),sizeof(char));
		sprintf(cmd,"xrandr --output %s --auto --output %s --auto --below %s",
			video1,video2,video1);
		system(cmd);
		free(cmd);
	}
	int num_sizes;
	XRRScreenSize *xrrs = XRRSizes(dpy,scr,&num_sizes);
	XRRScreenConfiguration *config = XRRGetScreenInfo(dpy,root);
	Rotation rotation;
	int sizeID = XRRConfigCurrentConfiguration(config,&rotation);
	XRRFreeScreenConfigInfo(config);
	int slider_w = xrrs[sizeID].width;
	int slider_h = xrrs[sizeID].height;
	/* set up Xlib graphics context(s) */
	XGCValues val;
	XColor color;
	Colormap cmap = DefaultColormap(dpy,scr);
	val.foreground = BlackPixel(dpy,scr);
	gc = XCreateGC(dpy,root,GCForeground,&val);
	val.foreground = 122122;
	pgc = XCreateGC(dpy,root,GCForeground,&val);
	val.foreground = WhitePixel(dpy,scr);
	val.font = XLoadFont(dpy,fontstring);
	ngc = XCreateGC(dpy,root,GCForeground|GCFont,&val);
	/* connect to slider */
	cmd = (char *) calloc(32+strlen(pdf),sizeof(char));
	sprintf(cmd,"slider -p -g %dx%d ",slider_w,slider_h);
	strcat(cmd,pdf);
	slider = popen(cmd,"r");
	free(cmd);
	char line[255];
	fgets(line,254,slider);
	while( strncmp(line,"SLIDER START",11) != 0) fgets(line,254,slider);
	sscanf(line,"SLIDER START (%dx%d) win=%lu slides=%d",
		&aw,&ah,&slider_win,&last_slide);
	last_slide--;
	fgets(line,254,slider);
	sscanf(line,"SLIDER: %d current=%lu, next=%lu",&cur_slide,&current,&preview);
	pix_current = XCreatePixmap(dpy,root,aw,ah,DefaultDepth(dpy,scr));
	pix_preview = XCreatePixmap(dpy,root,aw,ah,DefaultDepth(dpy,scr));
	buffer = XCreatePixmap(dpy,root,sw,sh,DefaultDepth(dpy,scr));
	if (useXrandr) {
		XMoveWindow(dpy,slider_win,0,sh);
		XMoveResizeWindow(dpy,win,0,0,sw,sh);
		XRaiseWindow(dpy,win);
//		XFlush(dpy);
	}
	/* mirror slider output scaled down */
	target_current = cairo_xlib_surface_create(dpy,buffer,
		DefaultVisual(dpy,scr),sw,sh);
	target_preview = cairo_xlib_surface_create(dpy,buffer,
		DefaultVisual(dpy,scr),sw,sh);
	cairo_current = cairo_create(target_current);
	cairo_preview = cairo_create(target_preview);
	cairo_scale(cairo_current,sw*fact/aw,sh*fact/ah);
	cairo_translate(cairo_preview,sw-sw*pfact-4,sh-sh*pfact-4);
	cairo_scale(cairo_preview,sw*pfact/aw,sh*pfact/ah);
	current_c = cairo_xlib_surface_create(dpy,pix_current,
		DefaultVisual(dpy,scr),aw,ah);
	preview_c = cairo_xlib_surface_create(dpy,pix_preview,
		DefaultVisual(dpy,scr),aw,ah);
	/* main loop */
	XEvent ev;
	int xfd,sfd,r;
	struct timeval tv;
	fd_set rfds;
	xfd = ConnectionNumber(dpy);
	sfd = fileno(slider);
	start_time = time(NULL);
	running = True;
	draw();
	while (running) {
		memset(&tv,0,sizeof(tv));
		tv.tv_sec=1;
		FD_ZERO(&rfds);
		FD_SET(xfd,&rfds);
		FD_SET(sfd,&rfds);
		r = select(sfd+1,&rfds,0,0,&tv);
		if (r == 0)
			draw();
		if (FD_ISSET(xfd,&rfds)) while (XPending(dpy)) { /* x input */
			XNextEvent(dpy,&ev);
			if (ev.type < 33)
				if (handler[ev.type]) handler[ev.type](&ev);
		}
		if (FD_ISSET(sfd,&rfds)) { /* slider input */
			msg[0] = '\0';
			fgets(line,254,slider);
			if (strncmp(line,"SLIDER END",9)==0) {
				running = False;
				break;
			}
			else if (strncmp(line,"SLIDER: ",7)==0)
				sscanf(line,"SLIDER: %d current=%lu, next=%lu",
					&cur_slide,&current,&preview);
			else if (strncmp(line,"MUTE: black",8)==0)
				strcpy(msg,"== SCREEN BLACK MUTED ==");
			else if (strncmp(line,"MUTE: white",8)==0)
				strcpy(msg,"== SCREEN WHITE MUTED ==");
			draw();
		}
	}
	/* clean up */
	cairo_surface_destroy(current_c);
	pclose(slider);
	if (pdf) free(pdf);
	if (useXrandr) {
		cmd = (char *) calloc(60+strlen(video1)+strlen(video2),sizeof(char));
		sprintf(cmd,"xrandr --output %s --auto --output %s --off",video1,video2);
		system(cmd);
		free(cmd);
		free(video1);
		free(video2);
	}
	XCloseDisplay(dpy);
	return 0;
}

