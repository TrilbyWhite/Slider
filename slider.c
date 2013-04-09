
#include "slider.h"

#define RUNNING		0x00000001
#define CONTINUOUS	0x00000002
#define AUTOPLAY	0x00000004	
#define OVERVIEW	0x00000010
#define PRESENTER	0x00000020
#define WHITEMUTE	0x00000100
#define BLACKMUTE	0x00000200
#define UNMUTE		0xFFFFFCFF

#define MAX_VIEW	4

typedef struct {
	unsigned int mod;
	KeySym key;
	void (*func)(const char *);
	const char *arg;
} Key;

typedef struct View View;
struct View {
	int x, y, w, h;
	cairo_surface_t *dest_c, *src_c;
	cairo_t *cairo;
	Pixmap buf;
};

static void buttonpress(XEvent *);
static void cleanup();
static void command_line(int, const char **);
static void draw(const char *);
static void init_views();
static void init_X();
static void keypress(XEvent *);
static void move(const char *);
static void mute(const char *);
static void overview(const char *);
static void quit(const char *);
static void rectangle(const char *);
static void zoom(const char *);

static Window wshow, wnote;
static Pixmap bshow, bnote;
static Cursor invisible_cursor, crosshair_cursor;
static XRectangle r;
static int mode = RUNNING, v1 = 1;
static Show *show;
static Bool muted = False;
static View *view;
static float pscale = 1.8;
#include "config.h"
static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress]	= buttonpress,
	[KeyPress]		= keypress,
};

void buttonpress(XEvent *ev) {
	XButtonEvent *e = &ev->xbutton;
	int x,y,n;
	if (mode & OVERVIEW) {
		if (e->button == 1 || e->button == 3) {
			x = (e->x - (sw-show->sorter->flag[0]*(show->sorter->w+1))/2)/
					(show->sorter->w + 10);
			y = (e->y -10)/(show->sorter->h + 10);
			n = y*show->sorter->flag[0]+x;
			if (show->flag[n] & RENDERED) {
				show->cur = n;
				overview(NULL);
			}
		}
		if (e->button == 2 || e->button == 3) draw(NULL);
	}
	else {
		if (e->button == 1) move("r");
		else if (e->button == 2) overview(NULL);
		else if (e->button == 3) move("l");
	}
}

GC cgc(int col) {
	XColor color;
	XAllocNamedColor(dpy,DefaultColormap(dpy,scr),colors[col],&color,&color);
	XSetForeground(dpy,gc,color.pixel);
	return gc;
}

void cleanup() {
	int i;
	for (i = 0; i < MAX_VIEW && view[i].w && view[i].h; i++) {
		cairo_surface_destroy(view[i].dest_c);
		cairo_surface_destroy(view[i].src_c);
		XFreePixmap(dpy,view[i].buf);
		cairo_destroy(view[i].cairo);
	}
	free_renderings(show);
	free(show->uri);
	free(show->sorter);
	if (show->notes) {
		free(show->notes->uri);
		free(show->notes);
	}
	free(view);
	free(show);
	XFreePixmap(dpy,bnote);
	XFreePixmap(dpy,bshow);
	XDestroyWindow(dpy,wnote);
	XDestroyWindow(dpy,wshow);
}

static inline char *get_uri(const char *arg) {
	char *fullpath = realpath(arg,NULL);
	if (fullpath == NULL) die("Cannot find file \"%s\"\n",arg);
	char *uri = (char *) calloc(strlen(fullpath)+8,sizeof(char));
	strcpy(uri,"file://");
	strcat(uri,fullpath);
	free(fullpath);
	return uri;
}

void command_line(int argc, const char **argv) {
	prerender = 1;
	int i, nv = 0, n;
	float f;
	show = (Show *) calloc(1,sizeof(Show));
	show->sorter = (Show *) calloc(1,sizeof(Show));
	view = (View *) calloc(MAX_VIEW,sizeof(View));
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'g') {
				if (4 == sscanf(argv[i],"-g%dx%d+%d+%d",
						&view[nv].w,&view[nv].h,&view[nv].x,&view[nv].y))
					nv++;
				if (nv >= MAX_VIEW) nv = MAX_VIEW - 1;
			}
			else if (argv[i][1] == 's') {
				if (sscanf(argv[i],"-s%f",&f) == 1) pscale = f;
			}
			else if (argv[i][1] == 'p') {
				if (sscanf(argv[i],"-p%d",&n) == 1) prerender = n;
				else prerender = 0;
			}
			else if (argv[i][1] == 'h') {
				//	-h	help
			}
			else if (argv[i][1] == 'v') {
				//	-v	version
			}
			else {
				fprintf(stderr,"Unused parameter \"%s\"\n",argv[i]);
			}
		}
		else if (!show->uri) {
			show->uri = get_uri(argv[i]);
		}
		else if (!show->notes) {
			show->notes = (Show *) calloc(1,sizeof(Show));
			show->notes->uri = get_uri(argv[i]);
			if (show->notes) v1 = 0;
		}
		else fprintf(stderr,"Unused parameter \"%s\"\n",argv[i]);
	}
	if (v1) for (i = MAX_VIEW - 1; i; i--) {
		view[i].w = view[i-1].w; view[i].h = view[i-1].h;
		view[i].x = view[i-1].x; view[i].y = view[i-1].y;
	}
}

static inline void draw_view(Show *set,int vnum, int snum) {
	if (!view[vnum].w || !view[vnum].h) return;
	View *v = &view[vnum];
	XFillRectangle(dpy,v->buf,cgc(ScreenBG),0,0,2*set->w,2*set->h);
	int n = (snum > 0 ? snum : 0);
	if ( !(n<set->count && (set->flag[n]&RENDERED)) )
		XFillRectangle(dpy,v->buf,cgc(Empty),80,80,set->w-160,set->h-160);
	else
		XCopyArea(dpy, set->slide[n], v->buf,gc,0,0,set->w,set->h,0,0);
	cairo_set_source_surface(v->cairo,v->src_c,0,0);
	cairo_paint(v->cairo);
}

void draw(const char *arg) {
	/* draw show */
	XFillRectangle(dpy,bshow,cgc(ScreenBG),0,0,sw,sh);
	if (!muted) {
		XCopyArea(dpy,show->slide[show->cur],bshow,gc,0,0,
				show->w,show->h,show->x,show->y);
		XCopyArea(dpy,bshow,wshow,gc,0,0,sw,sh,0,0);
		XFlush(dpy);
	}
	/* draw previews */
	if (!swnote || !shnote) return;
	XFillRectangle(dpy,bnote,cgc(ScreenBG),0,0,swnote,shnote);
	if (show->notes) draw_view(show->notes,0,show->cur);
	draw_view(show,1,show->cur);
	draw_view(show,2,show->cur + 1);
	draw_view(show,3,show->cur - 1);
	XCopyArea(dpy,bnote,wnote,gc,0,0,swnote,shnote,0,0);
	XFlush(dpy);
}

void init_views() {
	if (!swnote || !shnote) return;
	View *v; int i;
	if (!view[v1].w && !view[v1].h) { /* no geometry selected, used default */
		int d1 = swnote/((float)4*pscale+4);
		int d2 = shnote/((float)3*pscale);
		int d = (d1 < d2 ? d1 : d2);
		view[v1].w = 4*pscale*d; view[v1].h = 3*pscale*d;
		view[v1].y = (shnote - view[v1].h)/2;
		view[v1+1].w = 4*d; view[v1+1].h = 3*d;
		view[v1+1].x = view[v1].w;
		if (v1) {
			view[v1+1].y = (shnote - view[v1+1].h)/2;
		}
		else {
			view[v1+2].w = view[v1+1].w; view[v1+2].h = view[v1+1].h;
			view[v1+2].x = view[v1+1].x;
			view[v1+1].y = (shnote - 2*view[v1+1].h)/3;
			view[v1+2].y = 2*view[v1+1].y + view[v1+1].h;
		}
	}
	for (i = v1, v = &view[i]; i < MAX_VIEW && v->w && v->h; v = &view[++i]) {
		v->buf = XCreatePixmap(dpy,root,show->w,show->h,DefaultDepth(dpy,scr));
		v->dest_c = cairo_xlib_surface_create(dpy,bnote,DefaultVisual(dpy,scr),
				swnote,shnote);
		v->cairo = cairo_create(v->dest_c);
		cairo_translate(v->cairo,v->x,v->y);
		cairo_scale(v->cairo, (float) v->w / (i ? show->w : show->notes->w),
				(float) v->h / (i ? show->h : show->notes->h) );
		v->src_c = cairo_xlib_surface_create(dpy,v->buf,DefaultVisual(dpy,scr),
				show->w,show->h);
	}
}

void init_X() {
	XInitThreads();
	g_type_init();
	if (!(dpy=XOpenDisplay(NULL))) die("Failed to open display\n");
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	XSetWindowAttributes wa;
	wa.event_mask = ExposureMask|KeyPressMask|ButtonPressMask|StructureNotifyMask;
	wa.override_redirect = True;
	/* get RandR info */
	XRRScreenResources *xrr_sr = XRRGetScreenResources(dpy,root);
	XRROutputInfo *xrr_out_info = NULL;
	XRRCrtcInfo *xrr_crtc_info;
	swnote = 0; shnote = 0;
	int i = xrr_sr->ncrtc;
	while ( --i > -1 ) {
		xrr_out_info = XRRGetOutputInfo(dpy,xrr_sr,xrr_sr->outputs[i]);
		if (xrr_out_info->crtc) break;
		XRRFreeOutputInfo(xrr_out_info);
	}
	/* Presentation monitor */
	xrr_crtc_info = XRRGetCrtcInfo(dpy,xrr_sr,xrr_out_info->crtc);
	sw = xrr_crtc_info->width;
	sh = xrr_crtc_info->height;
	show->w = sw; show->h = sh;
	wshow = XCreateSimpleWindow(dpy,root,xrr_crtc_info->x,xrr_crtc_info->y,
			sw,sh,1,0,0);
	bshow = XCreatePixmap(dpy,wshow,sw,sh,DefaultDepth(dpy,scr));
	XRRFreeCrtcInfo(xrr_crtc_info);
	XRRFreeOutputInfo(xrr_out_info);
	XStoreName(dpy,wshow,"Slider");
	XChangeWindowAttributes(dpy,wshow,CWEventMask|CWOverrideRedirect,&wa);
	/* Notes monitor */
	if (i) {
		xrr_out_info = XRRGetOutputInfo(dpy,xrr_sr,xrr_sr->outputs[0]);
		if (!xrr_out_info->crtc) die("No RandR info for screen 1\n");
		xrr_crtc_info = XRRGetCrtcInfo(dpy,xrr_sr,xrr_out_info->crtc);
		swnote = xrr_crtc_info->width;
		shnote = xrr_crtc_info->height;
		if (show->notes) { show->notes->w = swnote; show->notes->h = shnote; }
		wnote = XCreateSimpleWindow(dpy,root,xrr_crtc_info->x,xrr_crtc_info->y,
				swnote,shnote,1,0,0);
		bnote = XCreatePixmap(dpy,wshow,swnote,shnote,DefaultDepth(dpy,scr));
		XRRFreeCrtcInfo(xrr_crtc_info);
		XRRFreeOutputInfo(xrr_out_info);
		XStoreName(dpy,wnote,"Slipper");
		XChangeWindowAttributes(dpy,wnote,CWEventMask|CWOverrideRedirect,&wa);
		XMapWindow(dpy,wnote);
	}
	else if (show->notes) {
		free(show->notes->uri);
		free(show->notes);
		show->notes = NULL;
	}
	XRRFreeScreenResources(xrr_sr);
	/* Graphic context */
	gc = DefaultGC(dpy,scr);
	XSetLineAttributes(dpy,gc,2,LineSolid,CapButt,JoinRound);
	XColor color;
	char cursor_data = 0;
	Pixmap curs_map = XCreateBitmapFromData(dpy,wshow,&cursor_data,1,1);
	invisible_cursor = XCreatePixmapCursor(dpy,curs_map,curs_map,&color,&color,0,0);
	crosshair_cursor = XCreateFontCursor(dpy,XC_crosshair);
	XDefineCursor(dpy,wshow,invisible_cursor);
	XMapWindow(dpy,wshow);
	XFlush(dpy);
	XSetInputFocus(dpy,wshow,RevertToPointerRoot,CurrentTime);
}

void keypress(XEvent *ev) {
	XKeyEvent *e = &ev->xkey;
	int i;
	KeySym key = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
	unsigned int mod = (e->state&~Mod2Mask)&~LockMask;
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
		if ( (key==keys[i].key) && keys[i].func && (keys[i].mod==mod) )
			keys[i].func(keys[i].arg);
}

void move(const char *arg) {
	if (mode & OVERVIEW) {
		if (arg[0] == 'r') show->cur++;
		else if (arg[0] == 'l') show->cur--;
		else if (arg[0] == 'd') show->cur += show->sorter->flag[0];
		else if (arg[0] == 'u') show->cur -= show->sorter->flag[0];
		if (show->cur < 0) show->cur = 0;
		while (show->cur > show->count || !(show->flag[show->cur] & RENDERED))
			show->cur--;
	}
	else {
		if (arg[0] == 'd' || arg[0] == 'r') show->cur++;
		else if (arg[0] == 'u' || arg[0] == 'l') show->cur--;
		if (show->cur < 0) show->cur = 0;
		if (show->cur >= show->count) {
			if (mode & CONTINUOUS) show->cur = 0;
			else show->cur = show->count - 1;
		}
		if (!(show->flag[show->cur] & RENDERED)) show->cur--;
		draw(NULL);
	}
}

void mute(const char *arg) {
	if ( (muted = !muted) ) {
		XFillRectangle(dpy,wshow,(arg[0]=='w'?cgc(White):cgc(Black)),0,0,sw,sh);
		XFlush(dpy);
	}
	else {
		draw(NULL);
	}
}

void overview(const char *arg) {
	XDefineCursor(dpy,wshow,None);
	XLockDisplay(dpy);
	XCopyArea(dpy,show->sorter->slide[0],wshow,gc,0,0,sw,sh,0,0);
	XUnlockDisplay(dpy);
	XFlush(dpy);
	// TODO
}

void quit(const char *arg) {
	mode &= ~RUNNING;
}

void rectangle(const char *arg) {
	XCopyArea(dpy,bshow,wshow,gc,0,0,sw,sh,0,0);
	XWarpPointer(dpy,None,wshow,0,0,0,0,sw/2,sh/2);
	XDefineCursor(dpy,wshow,crosshair_cursor);
	XFlush(dpy);
	XEvent ev; XButtonEvent *e; Bool on = False;
	memset(&r,0,sizeof(XRectangle));
	while (!XNextEvent(dpy,&ev)) {
		if (ev.type == KeyPress) break;
		if (ev.type == ButtonPress) {
			e = &ev.xbutton;
			if (on) break;
			r.x = e->x_root; r.y = e->y_root;
			XGrabPointer(dpy,root,True,PointerMotionMask | ButtonPressMask,
					GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
			on = True;
		}
		if (ev.type == MotionNotify && on) {
			e = &ev.xbutton;
			r.width = e->x_root - r.x; r.height = e->y_root - r.y;
			if (arg && r.width && r.height) {
				if (r.width/r.height < show->w/show->h)
					r.width = r.height*show->w/show->h;
				else
					r.height = r.width*show->h/show->w;
			}
			XCopyArea(dpy,bshow,wshow,gc,0,0,sw,sh,0,0);
			XDrawRectangle(dpy,wshow,cgc(Highlight),r.x,r.y,r.width,r.height);
		}
		XFlush(dpy);
	}
	XUngrabPointer(dpy,CurrentTime);
	XDefineCursor(dpy,wshow,invisible_cursor);
	XFlush(dpy);
}

void zoom(const char *arg) {
	if (!(show->flag[show->count-1] & RENDERED)) {
		// TODO warn;
		return;
	}
	rectangle(arg);
	r.x-=5; r.y-=5; r.width+=10; r.height+=10;
	int w,h;
	if (arg) { w = show->w; h = show->h; }
	else { w = sw; h = sh; }
	Pixmap b = XCreatePixmap(dpy,wshow,w,h,DefaultDepth(dpy,scr));
	XFillRectangle(dpy,b,cgc(SlideBG),0,0,w,h);

	PopplerDocument *pdf = poppler_document_new_from_file(show->uri,NULL,NULL);
	PopplerPage *page = poppler_document_get_page(pdf,show->cur);
	double pdfw, pdfh;
	poppler_page_get_size(page,&pdfw,&pdfh);

	cairo_surface_t *t=cairo_xlib_surface_create(dpy,b,DefaultVisual(dpy,scr),w,h);
	cairo_t *c = cairo_create(t);
	double xs = (double)(show->w*w)/(pdfw*(double)r.width);
	double ys = (double)(show->h*h)/(pdfh*(double)r.height);
	double xo = (double)(show->x - r.x)*(xs/show->scale);
	double yo = (double)(show->y - r.y)*(ys/show->scale);
	
	cairo_translate(c,xo,yo);
	cairo_scale(c,xs,ys);
	
	poppler_page_render(page,c);

	cairo_surface_destroy(t);
	cairo_destroy(c);
	XFillRectangle(dpy,wshow,cgc(ScreenBG),0,0,sw,sh);
	XCopyArea(dpy,b,wshow,gc,0,0,w,h,(sw-w)/2,(sh-h)/2);
	XFreePixmap(dpy,b);
	XFlush(dpy);
}

int main(int argc, const char **argv) {
	command_line(argc,argv);
	init_X();
	render(show);
	/* grab keys */
	int i;
	for (i = 0; i < 1000; i++) {
		if (XGrabKeyboard(dpy,root,True,GrabModeAsync,GrabModeAsync,
				CurrentTime) == GrabSuccess) break;
		usleep(5000);
	}
	init_views();
	draw(NULL);
	XEvent ev;
	while ( (mode & RUNNING) && !XNextEvent(dpy,&ev) ) {
		if (ev.type > 32) continue;
		if (handler[ev.type]) handler[ev.type](&ev);
	}
	cleanup();
	return 0;
}

