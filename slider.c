/**************************************************************************\
* SLIDER - pdf presentation tool
*
* Author: Jesse McClure, copyright 2012-2013
* License: GPLv3
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
\**************************************************************************/


#include "slider.h"

#define RUNNING		0x00000001
#define CONTINUOUS	0x00000002
#define AUTOPLAY	0x00000004
#define OVERVIEW	0x00000010
#define PRESENTER	0x00000020
#define MUTED		0x00000100

#define MAX_VIEW	4

typedef void (*Bind)(const char *);

typedef struct {
	unsigned int mod;
	KeySym key;
	Bind func;
	const char *arg;
} Key;

typedef struct {
	int x, y, w, h;
	cairo_surface_t *dest_c, *src_c;
	cairo_t *cairo;
	Pixmap buf;
} View;

enum { Black, White, ScreenBG, SlideBG, Empty };

#ifdef ACTION_LINKS
static void action(const char *);
#endif /* ACTION_LINKS */
static void buttonpress(XEvent *);
static GC cgc(int);
static void cleanup();
static void command_line(int, const char **);
static void config(const char *);
static void draw(const char *);
#ifdef FORM_FILL
static void fillfield(const char *);
#endif /* FORM_FILL */
static void grab_keys(Bool);
static void init_views();
static void init_X();
static void keypress(XEvent *);
static void move(const char *);
static void mute(const char *);
static void overview(const char *);
#ifdef DRAWING
static void pen(const char *);
static void perm_pen(const char *);
static void polka(const char *);
static void perm_rect(const char *);
static void rectangle(const char *);
static void string(const char *);
#endif /* DRAWING */
static void quit(const char *);
static void usage(const char *);
static void warn();
#ifdef ZOOMING
static void zoom(const char *);
#endif /* ZOOMING */

static Window wshow, wnote;
static Pixmap bshow, bnote;
static Cursor invisible_cursor, crosshair_cursor;
static XRectangle r;
static int mode = RUNNING, v1 = 1, delay = 10;
static Show *show;
static View *view;
static float pscale = 1.8;
static char *monShow = NULL, *monNote = NULL;
int nkeys = 0, nbtns = 0;
Key *keys = NULL, *btns = NULL;
char colors[5][9], *SHOW_URI, *SHOW_MOV, *PLAY_AUD;
char *EMPTY_RECT, *ZOOM_RECT, *OVERVIEW_RECT, *ACTION_RECT, *ACTION_FONT;
#define CURSOR_STRING_MAX	12
static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress]	= buttonpress,
	[KeyPress]		= keypress,
};

#ifdef ACTION_LINKS
void action(const char *arg) {
	if (mode & OVERVIEW) return;
	if (!(show->flag[show->count-1] & RENDERED)) { warn(); return; }
	char input = (arg ? arg[0] : 'm');
	/* create cairo context */
	int w,tw; double R,G,B,A,tR,tG,tB,tA; char sym[2] = "x";
	sscanf(ACTION_RECT,"%d %lf,%lf,%lf %lf",&w,&R,&G,&B,&A);
	sscanf(ACTION_FONT,"%d %lf,%lf,%lf %lf",&tw,&tR,&tG,&tB,&tA);
	cairo_surface_t *t = cairo_xlib_surface_create(dpy,wshow,
			DefaultVisual(dpy,scr),sw+swnote,sh+shnote);
	cairo_t *c = cairo_create(t);
	cairo_translate(c,show->x,show->y);
	cairo_scale(c,show->scale,show->scale);
	cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_width(c,w);
	cairo_set_font_size(c,tw);
	cairo_select_font_face(c,"sans-serif",
			CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_BOLD);
	/* get pdf page and action links */
	PopplerDocument *pdf;
	pdf = poppler_document_new_from_file(show->uri,NULL,NULL);
	PopplerPage *page = poppler_document_get_page(pdf,show->cur);
	GList *amap, *list;
	amap = poppler_page_get_link_mapping(page);
	PopplerAction *act = NULL;
	PopplerRectangle r;
	int n, nsel = 0;
	if (input == 'a') {	/* autoplay ACTION_LAUNCH action */
	      for (list = amap; list; list = list->next) {
		      if ((((PopplerLinkMapping *)list->data)->action)->type == POPPLER_ACTION_LAUNCH) {
			act = ((PopplerLinkMapping *)list->data)->action;
			r = ((PopplerLinkMapping *)list->data)->area;
			r.y1 = show->h / show->scale - r.y1;
			r.y2 = show->h / show->scale - r.y2;
		      }
	      }
	}
	else {			/* keyboard or mouse ACTION_LAUNCH action */
	        for (list = amap, n = 0; list && n < 26; list = list->next, n++) {
		        r = ((PopplerLinkMapping *)list->data)->area;
			r.y1 = show->h / show->scale - r.y1;
			r.y2 = show->h / show->scale - r.y2;
			/* draw links */
			cairo_set_source_rgba(c,R,G,B,A);
			cairo_rectangle(c,r.x1,r.y1,r.x2-r.x1,r.y2-r.y1);
			cairo_stroke(c);
			if (input == 'k') {
			        cairo_arc(c,r.x1,r.y2,tw,0,2*M_PI);
				cairo_fill(c);
				sym[0] = n + 65; /* ascii 65 = 'A' */
				cairo_set_source_rgba(c,tR,tG,tB,tA);
				cairo_move_to(c,r.x1-tw/2.0,r.y2+tw/2.0);
				cairo_show_text(c,sym);
				cairo_stroke(c);
			}
		}
		if (!n) return;
		XEvent ev;
		if (input == 'k') { /* get key */
		        while (!XCheckTypedEvent(dpy,KeyPress,&ev));
			XKeyEvent *e = &ev.xkey;
			nsel = XKeysymToString(XkbKeycodeToKeysym(dpy,e->keycode,0,0))[0]-97;
			/* get selected action link */
			for (list = amap, n = 0; list && n!=nsel; list = list->next, n++);
			if (n == nsel) {
			      act = ((PopplerLinkMapping *)list->data)->action;
			      r = ((PopplerLinkMapping *)list->data)->area;
			      r.y1 = show->h / show->scale - r.y1;
			      r.y2 = show->h / show->scale - r.y2;
			}
		}
		else { /* get mouse click */
		        int mx=0,my=0;
			XUndefineCursor(dpy,wshow);
			XMaskEvent(dpy,ButtonPressMask|KeyPressMask,&ev);
			if (ev.type == KeyPress) XPutBackEvent(dpy,&ev);
			else if (ev.type == ButtonPress) {
			        mx = (ev.xbutton.x - show->x) / show->scale;
				my = (ev.xbutton.y - show->y) / show->scale;
			}
			XDefineCursor(dpy,wshow,invisible_cursor);
			draw(NULL);
			if (mx || my) for (list = amap; list; list = list->next) {
			          act = ((PopplerLinkMapping *)list->data)->action;
				  r = ((PopplerLinkMapping *)list->data)->area;
				  r.y1 = show->h / show->scale - r.y1;
				  r.y2 = show->h / show->scale - r.y2;
				  if (mx > r.x1 && mx < r.x2 && my > r.y2 && my < r.y1) break;
			  }
		}
	}
	/* folow link */
	if (act) {
		grab_keys(False); /* release keyboard for external prog. */
		if (act->type == POPPLER_ACTION_GOTO_DEST) {
			PopplerActionGotoDest *a = &act->goto_dest;
			if (a->dest->type == POPPLER_DEST_NAMED) {
				PopplerDest *d = poppler_document_find_dest(pdf,
						a->dest->named_dest);
				if (d) {
					show->cur = d->page_num - 1;
					poppler_dest_free(d);
				}
			}
			else {
				show->cur = a->dest->page_num - 1;
			}
		}
		else if (act->type == POPPLER_ACTION_NAMED) {
			PopplerActionNamed *n = &act->named;
			PopplerDest *d = poppler_document_find_dest(pdf,n->named_dest);
			if (d) {
				show->cur = d->page_num - 1;
				poppler_dest_free(d);
			}
		}
		else if (act->type == POPPLER_ACTION_LAUNCH) {
			PopplerActionLaunch *l = &act->launch;
/* Action Launch Links will never be allowed by default as they
   can potentially pose a security risk.  If you are sure your
   pdfs can all be trusted, and you want to activate this feature
   yourself, simply compile Slider with the -DALLOW_PDF_ACTION_LAUNCH
   flag.  Do this only AT YOUR OWN RISK.
       - J McClure (10 June 2013)    */
#ifdef ALLOW_PDF_ACTION_LAUNCH
			/* fashion the area coordinates as params */
			/* topl_x topl_y width height - each 4 digit - hardwired to 20 */
			gchar *coords = calloc(20,sizeof(char));
			sprintf(coords,"%04g %04g %04g %04g",
				round(r.x1*show->scale),
				round(r.y2*show->scale),
				round((r.x2-r.x1)*show->scale),
				round((r.y1-r.y2)*show->scale));
			size_t pl = !l->params ? 0 : strlen(l->params);
			char *cmd = calloc(strlen(l->file_name)+pl+strlen(coords)+2,sizeof(char));
			sprintf(cmd,"%s %s %s",l->file_name,!l->params?"":l->params,coords);
			system(cmd); free(cmd); free(coords);
#else
			fprintf(stderr,"[SLIDER] blocked launch of \"%s %s\"\n",
					l->file_name,l->params);
#endif /* ALLOW_PDF_ACTION_LAUNCH */
		}
		else if (act->type == POPPLER_ACTION_URI) {
			PopplerActionUri *u = &act->uri;
			char *cmd = calloc(strlen(u->uri)+strlen(SHOW_URI)+2,
					sizeof(char));
			sprintf(cmd,SHOW_URI,u->uri); system(cmd); free(cmd);
		}
		else if (act->type == POPPLER_ACTION_MOVIE) {
			PopplerActionMovie *m = &act->movie;
			const char *mov = poppler_movie_get_filename(m->movie);
			char *cmd = calloc(strlen(mov)+strlen(SHOW_MOV)+2,sizeof(char));
			sprintf(cmd,SHOW_MOV,mov);
			system(cmd); free(cmd);
		}
		else if (act->type == POPPLER_ACTION_RENDITION) {
			PopplerActionRendition *r = &act->rendition;
			const char *med = poppler_media_get_filename(r->media);
			char *cmd = calloc(strlen(med)+strlen(PLAY_AUD)+2,sizeof(char));
			sprintf(cmd,PLAY_AUD,med); system(cmd); free(cmd);
		}
		else {
			fprintf(stderr,"unsupported action selected (type=%d)\n",
					act->type);
		}
		grab_keys(True);
		XRaiseWindow(dpy,wshow);
		if (swnote) XRaiseWindow(dpy,wnote);
	}
	poppler_page_free_link_mapping(amap);
	cairo_destroy(c);
	draw(NULL);
}
#endif /* ACTION_LINKS */

void buttonpress(XEvent *ev) {
	XButtonEvent *e = &ev->xbutton;
	int i,x,y,n;
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
		unsigned int mod = (e->state&~Mod2Mask)&~LockMask;
		for (i = 0; i < nbtns; i++) {
			if (btns[i].mod==mod && btns[i].key==e->button && btns[i].func)
				btns[i].func(btns[i].arg);
		}
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
	if (show->uri) {
		free(show->uri);
		free_renderings(show);
		XFreePixmap(dpy,bnote);
		XFreePixmap(dpy,bshow);
		XDestroyWindow(dpy,wnote);
		XDestroyWindow(dpy,wshow);
	}
	free(view);
	free(show->sorter);
	if (show->notes) {
		free(show->notes->uri);
		free(show->notes);
	}
	free(show);
	if (monShow) free(monShow);
	if (monNote) free(monNote);
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
	prerender = 2;
	int i, nv = 0, n;
	char s1[25],s2[25];
	float f;
	const char *rc = NULL;
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
			else if (argv[i][1] == 'c') {
				mode |= CONTINUOUS;
			}
			else if (argv[i][1] == 'a') {
				mode |= AUTOPLAY;
				if (argv[i][2] != '\0') delay = atoi(argv[i] + 2);
			}
			else if (argv[i][1] == 's') {
				if (sscanf(argv[i],"-s%f",&f) == 1) pscale = f;
			}
			else if (argv[i][1] == 'p') {
				if (sscanf(argv[i],"-p%d",&n) == 1) prerender = n;
				else prerender = 0;
			}
			else if (argv[i][1] == 'm') {
				n = sscanf(argv[i],"-m%[^,],%s",s1,s2);
				if (n == 2) monNote = strdup(s2);
				if (n) monShow = strdup(s1);
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
	if (!show->uri) { cleanup(); usage(argv[0]); exit(1); }
	config(rc);
}

#define MAX_LINE 255
static Bind config_helper(const char *name, int ln) {
	if (strstr(name,"quit"))			return quit;
	else if (strstr(name,"overview"))	return overview;
	else if (strstr(name,"mute"))		return mute;
#ifdef ACTION_LINKS
	else if (strstr(name,"draw"))		return draw;
	else if (strstr(name,"move"))		return move;
	else if (strstr(name,"action"))	return action;
#endif /* ACTION_LINKS */
#ifdef FORM_FILL
	else if (strstr(name,"form"))		return fillfield;
#endif /* FORM_FILL */
#ifdef DRAWING
	else if (strstr(name,"zoom"))		return zoom;
	else if (strstr(name,"rectangle"))	return rectangle;
	else if (strstr(name,"Rectangle"))	return perm_rect;
	else if (strstr(name,"string"))	return string;
	else if (strstr(name,"polka"))		return polka;
	else if (strstr(name,"pen"))		return pen;
	else if (strstr(name,"Pen"))		return perm_pen;
#endif /* DRAWING */
	fprintf(stderr,"config [%d]: %s is not a bindable function\n", ln,name);
	return NULL;
}

void config(const char *fname) {
	FILE *rc = NULL;
	if (fname && !(rc=fopen(fname,"r")))
		die("unable to open \"%s\"\n",fname);
	const char *cwd = getenv("PWD");
	const char *cfg = getenv("XDG_CONFIG_HOME");
	const char *hom = getenv("HOME");
	if (!rc && cfg && !chdir(cfg) && !chdir("slider"))
		rc = fopen("config","r");
	if (!rc && hom && !chdir(hom) && !chdir(".config/slider"))
		rc = fopen("config","r");
	if (!rc && hom && !chdir(hom))
		rc = fopen(".sliderrc","r");
	if (!rc && !chdir("/usr/share/slider"))
		rc = fopen("config","r");
	if (cwd) chdir(cwd);
	if (!rc) die("unable to find configuration file\n");
	int ln = 0, n = 0, b = 0;
	char in[5][MAX_LINE+1];
	KeySym keysym;
	while (fgets(in[0],MAX_LINE,rc)) {
		in[0][strlen(in[0])-1] = '\0';
		ln++;
		if (in[0][0] == '#' || in[0][0] == '\n') continue;
		else if (in[0][0] == 'c') { /* COLOR */
			if (sscanf(in[0],"color %s %s",in[1],in[2]) == 2) {
				if (strncmp("blackMute",in[1],strlen(in[1]))==0)
					strncpy(colors[Black],in[2],8);
				else if (strncmp("whiteMute",in[1],strlen(in[1]))==0)
					strncpy(colors[White],in[2],8);
				else if (strncmp("screenBG",in[1],strlen(in[1]))==0)
					strncpy(colors[ScreenBG],in[2],8);
				else if (strncmp("slideBG",in[1],strlen(in[1]))==0)
					strncpy(colors[SlideBG],in[2],8);
				else if (strncmp("empty",in[1],strlen(in[1]))==0)
					strncpy(colors[Empty],in[2],8);
				else
					fprintf(stderr,
					"config [%d]: color target \"%s\" not recognized\n",
							ln,in[1]);
			}
			else fprintf(stderr,"config [%d]: syntax error in \"%s\"\n",
					ln,in[0]);
		}
		else if (in[0][0] == 's') { /* SET */
			if (sscanf(in[0],"set %s = %[^\n]",in[1],in[2]) == 2) {
				if (strncmp("showURI",in[1],strlen(in[1]))==0)
					SHOW_URI = strdup(in[2]);
				else if (strncmp("showMovie",in[1],strlen(in[1]))==0)
					SHOW_MOV = strdup(in[2]);
				else if (strncmp("playAudio",in[1],strlen(in[1]))==0)
					PLAY_AUD = strdup(in[2]);
				else
					fprintf(stderr,"config [%d]: cannot set \"%s\"\n",
							ln,in[1]);
			}
			else fprintf(stderr,"config [%d]: syntax error in \"%s\"\n",
					ln,in[0]);
		}
		else if (in[0][0] == 'd') { /* DRAW */
			if (sscanf(in[0],"draw %s %[^\n]",in[1],in[2]) == 2) {
				if (strncmp("emptyRect",in[1],strlen(in[1]))==0)
					EMPTY_RECT = strdup(in[2]);
				else if (strncmp("zoomRect",in[1],strlen(in[1]))==0)
					ZOOM_RECT = strdup(in[2]);
				else if (strncmp("overviewRect",in[1],strlen(in[1]))==0)
					OVERVIEW_RECT = strdup(in[2]);
				else if (strncmp("actionRect",in[1],strlen(in[1]))==0)
					ACTION_RECT = strdup(in[2]);
				else if (strncmp("actionFont",in[1],strlen(in[1]))==0)
					ACTION_FONT = strdup(in[2]);
				else
					fprintf(stderr,"config [%d]: \"%s\" is not a drawable\n",
							ln,in[1]);
			}
			else fprintf(stderr,"config [%d]: syntax error in \"%s\"\n",
					ln,in[0]);
		}
		else if (in[0][0] == 'k') { /* KEY BIND */
			in[1][0] = in[4][0] = '\0';
			if (
	((sscanf(in[0],"key %s %s = %s %[^\n]",in[1],in[2],in[3],in[4])== 4)	||
	 (sscanf(in[0],"key %s %s = %s",		in[1],in[2],in[3])		== 3)	||
	 (sscanf(in[0],"key %s = %s %[^\n]",	in[2],in[3],in[4])		== 3)	||
	 (sscanf(in[0],"key %s = %s",			in[2],in[3])			== 2))	&&
	((keysym=XStringToKeysym(in[2])) != NoSymbol ) ) {
				keys = realloc(keys,(n+1) * sizeof(Key));
				keys[n].key = keysym; keys[n].mod = 0;
				if (in[1][0] != '\0') {
					if (strcasestr(in[1],"Shift")) keys[n].mod|=ShiftMask;
					if (strcasestr(in[1],"Control")) keys[n].mod|=ControlMask;
					if (strcasestr(in[1],"Alt")) keys[n].mod|=Mod2Mask;
					if (strcasestr(in[1],"Super")) keys[n].mod|=Mod4Mask;
				}
				keys[n].func = config_helper(in[3],ln);
				if (in[4][0] != '\0') keys[n].arg = strdup(in[4]);
				else keys[n].arg = NULL; n++;
			}
			else fprintf(stderr,"config [%d]: syntax error in \"%s\"\n",
					ln,in[0]);
		}
		else if (in[0][0] == 'm') { /* MOUSE BIND */
			in[1][0] = in[4][0] = '\0';
			if (
	((sscanf(in[0],"mouse %s %s = %s %[^\n]",in[1],in[2],in[3],in[4])== 4)	||
	 (sscanf(in[0],"mouse %s %s = %s",		in[1],in[2],in[3])		== 3)	||
	 (sscanf(in[0],"mouse %s = %s %[^\n]",	in[2],in[3],in[4])		== 3)	||
	 (sscanf(in[0],"mouse %s = %s",			in[2],in[3])			== 2))) {
				btns = realloc(btns,(b+1) * sizeof(Key));
				btns[b].key = atoi(in[2]); btns[b].mod = 0;
				if (in[1][0] != '\0') {
					if (strcasestr(in[1],"Shift")) btns[b].mod|=ShiftMask;
					if (strcasestr(in[1],"Control")) btns[b].mod|=ControlMask;
					if (strcasestr(in[1],"Alt")) btns[b].mod|=Mod2Mask;
					if (strcasestr(in[1],"Super")) btns[b].mod|=Mod4Mask;
				}
				btns[b].func = config_helper(in[3],ln);
				if (in[4][0] != '\0') btns[b].arg = strdup(in[4]);
				else btns[b].arg = NULL; b++;
			}
			else fprintf(stderr,"config [%d]: syntax error in \"%s\"\n",
					ln,in[0]);
		}
	}
	fclose(rc);
	nkeys = n;
	nbtns = b;
}

static inline void draw_view(Show *set,int vnum, int snum) {
	if (!view[vnum].w || !view[vnum].h) return;
	View *v = &view[vnum];
	XFillRectangle(dpy,v->buf,cgc(ScreenBG),0,0,2*set->w,2*set->h);
	int n = (snum > 0 ? snum : 0);
	if ( n<set->count && (set->flag[n]&RENDERED) ) {
		XCopyArea(dpy, set->slide[n], v->buf,gc,0,0,set->w,set->h,0,0);
		cairo_set_source_surface(v->cairo,v->src_c,0,0);
		cairo_paint(v->cairo);
	}
	else {
		cairo_rectangle(v->cairo,40,40,v->w-80,v->h-80);
		cairo_fill(v->cairo);
	}
}

void draw(const char *arg) {
	cairo_surface_t *t;
	cairo_t *c;
	if (arg && arg[0] == 'r') { /* rerender */
		if (!(show->flag[show->count-1] & RENDERED)) { warn(); return; }
		t = cairo_xlib_surface_create(dpy,show->slide[show->cur],
				DefaultVisual(dpy,scr),sw,sh);
		c = cairo_create(t);
		cairo_surface_destroy(t);
		cairo_set_source_rgba(c,1,1,1,1);
			cairo_rectangle(c,0,0,sw,sh);
		cairo_fill(c);
		cairo_scale(c,show->scale,show->scale);
		PopplerDocument *pdf;
		PopplerPage *page;
		pdf = poppler_document_new_from_file(show->uri,NULL,NULL);
		page = poppler_document_get_page(pdf,show->cur);
		poppler_page_render(page,c);
		cairo_destroy(c);
	}
	if (mode & OVERVIEW) {
		mode &= ~OVERVIEW;
		XDefineCursor(dpy,wshow,invisible_cursor);
	}
	/* draw show */
#ifdef FADE_TRANSITION
	t = cairo_xlib_surface_create(dpy,wshow,DefaultVisual(dpy,scr),sw,sh);
	c = cairo_create(t);
	cairo_surface_destroy(t);
	cairo_set_source_rgba(c,0,0,0,1);
	cairo_rectangle(c,0,0,show->x,sh);
	cairo_rectangle(c,show->x+show->w,0,show->x,sh);
	cairo_rectangle(c,0,0,sw,show->y);
	cairo_rectangle(c,0,show->y+show->h,sw,show->y);
	cairo_fill(c);
	cairo_translate(c,show->x,show->y);
	int i;
	t = cairo_xlib_surface_create(dpy,show->slide[show->cur],
			DefaultVisual(dpy,scr),show->w,show->h);
	for (i = 20; i > 0; i--) {
		cairo_set_source_surface(c,t,0,0);
		cairo_paint_with_alpha(c,1.0/(float)i);
		cairo_fill(c);
		XFlush(dpy);
		usleep(5000);
	}
	cairo_surface_destroy(t);
	cairo_destroy(c);
#else /* FADE_TRANSITION */
	XFillRectangle(dpy,bshow,cgc(ScreenBG),0,0,sw,sh);
	if (!(mode & MUTED)) {
		XCopyArea(dpy,show->slide[show->cur],bshow,gc,0,0,
				show->w,show->h,show->x,show->y);
		XCopyArea(dpy,bshow,wshow,gc,0,0,sw,sh,0,0);
		XFlush(dpy);
	}
#endif /* FADE_TRANSITION */
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

#ifdef FORM_FILL
void fillfield(const char *arg) {
	static PopplerDocument *modPDF = NULL;
	if (!modPDF) {
		PopplerDocument *pdf;
		pdf = poppler_document_new_from_file(show->uri,NULL,NULL);
		poppler_document_save_a_copy(pdf,"file:///tmp/fill.pdf",NULL);
		modPDF = poppler_document_new_from_file("file:///tmp/fill.pdf",
				NULL,NULL);
	}
	if (mode & OVERVIEW) return;
	if (!(show->flag[show->count-1] & RENDERED)) { warn(); return; }
	/* create cairo context */
	int w, tw; double R,G,B,A;
	cairo_surface_t *t = cairo_xlib_surface_create(dpy,wshow,
			DefaultVisual(dpy,scr),sw,sh);
	cairo_t *c = cairo_create(t);
	cairo_surface_destroy(t);
	cairo_translate(c,show->x,show->y);
	cairo_scale(c,show->scale,show->scale);
	/* font ? */
	PopplerPage *page = poppler_document_get_page(modPDF,show->cur);
	GList *fmap, *list;
	PopplerRectangle r;
	PopplerFormField *f;
	fmap = poppler_page_get_form_field_mapping(page);
	for (list = fmap; list; list = list->next) {
		r = ((PopplerFormFieldMapping *)list->data)->area;
		r.y1 = show->h / show->scale - r.y1;
		r.y2 = show->h / show->scale - r.y2;
		cairo_set_source_rgba(c,0.0,1.0,1.0,0.4);
		cairo_rectangle(c,r.x1,r.y1,r.x2-r.x1,r.y2-r.y1);
		cairo_fill(c);
		/* key shortcuts ? */
	}
	/* get a location from the mouse */
	int mx=0, my=0;
	XDefineCursor(dpy,wshow,crosshair_cursor);
	XEvent ev;
	XMaskEvent(dpy,ButtonPressMask|KeyPressMask,&ev);
	if (ev.type == KeyPress) XPutBackEvent(dpy,&ev);
	else if (ev.type == ButtonPress) {
		mx = (ev.xbutton.x - show->x) / show->scale;
		my = (ev.xbutton.y - show->y) / show->scale;
	}
	XDefineCursor(dpy,wshow,invisible_cursor);
	//XSync(dpy,True);
	draw(NULL);
	list = NULL;
	if (mx || my) for (list = fmap; list; list = list->next) {
		f = ((PopplerFormFieldMapping *)list->data)->field;
		r = ((PopplerFormFieldMapping *)list->data)->area;
		r.y1 = show->h / show->scale - r.y1;
		r.y2 = show->h / show->scale - r.y2;
		if (mx > r.x1 && mx < r.x2 && my > r.y2 && my < r.y1) break;
	}
	if (!list) {
		cairo_destroy(c);
		poppler_page_free_form_field_mapping(fmap);
		return;
	}
	/* get field type and allow entry */
	PopplerFormFieldType ft = poppler_form_field_get_field_type(f);
	KeySym key;
	if (ft == POPPLER_FORM_FIELD_BUTTON) {
		PopplerFormButtonType bt;
		bt = poppler_form_field_button_get_button_type(f);
		if (bt == POPPLER_FORM_BUTTON_PUSH) printf("push button: ");
		else if (bt == POPPLER_FORM_BUTTON_RADIO) printf("radio button: ");
		else if (bt == POPPLER_FORM_BUTTON_CHECK) printf("check button: ");
		printf("%d -> ",poppler_form_field_button_get_state(f));
		poppler_form_field_button_set_state(f,
				!poppler_form_field_button_get_state(f));
		printf("%d\n",poppler_form_field_button_get_state(f));
	}
	else if (ft == POPPLER_FORM_FIELD_CHOICE) {
printf("form fill type=choice not implemented yet\n");
		// TODO
	}
	else if (ft == POPPLER_FORM_FIELD_SIGNATURE) {
printf("form fill type=signature not implemented yet\n");
		// TODO
	}
	else if (ft == POPPLER_FORM_FIELD_TEXT) { /* text fill */
		/* set up locale and input context */
		if (!setlocale(LC_CTYPE,"") ||
				!XSupportsLocale() ||
				!XSetLocaleModifiers("") ) {
			fprintf(stderr,"locale failure\n");
			return;
		}
		XIM xim = XOpenIM(dpy,NULL,NULL,NULL);
		XIC xic = XCreateIC(xim,XNInputStyle,
				XIMPreeditNothing|XIMStatusNothing, XNClientWindow,
				wshow, XNFocusWindow, wshow, NULL);
		/* get field info */
		float sz = poppler_form_field_get_font_size(f);
		cairo_set_font_size(c, (sz = (sz ? sz : 10)) );
		cairo_select_font_face(c,"sans-serif",
				CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		int len = poppler_form_field_text_get_max_len(f);
		//char *txt = malloc(((len=(len?len:255))+1)*sizeof(char));
		char *txt = malloc(((len=(len?len:255))+1)*sizeof(char));
		gchar *temp = poppler_form_field_text_get_text(f);
		if (temp) { strncpy(txt,temp,len); g_free(temp); }
		else txt[0] = '\0';
		/* event loop */
		KeySym key; Status stat;
		Bool multi = (poppler_form_field_text_get_text_type(f) ==
				POPPLER_FORM_TEXT_MULTILINE);
		int inlen;
		char *ch = &txt[strlen(txt)], *ts;
		char in[8];
		cairo_set_line_width(c,1);
		cairo_text_extents_t ext;
		cairo_font_extents_t fext;
		cairo_font_extents(c,&fext);
		double x,y;
		while (key != XK_Escape) {
			/* draw text */
			cairo_set_source_rgba(c,0.8,1.0,1.0,1.0);
				// TODO get a back color?
			cairo_rectangle(c,r.x1,r.y1,r.x2-r.x1,r.y2-r.y1);
			cairo_fill_preserve(c);
			cairo_set_source_rgba(c,0.0,0.0,0.0,1.0);
				// TODO get a font color?
			cairo_stroke(c);
			cairo_move_to(c,r.x1,r.y2+fext.height);
			if (multi) {
				char *nl, *start = strdup(txt), *part;
				part = start;
				while ( (nl=strchr(part,'\n')) ) {
					*nl = '\0';
					cairo_show_text(c,part);
					cairo_text_extents(c,part,&ext);
					cairo_rel_move_to(c,-ext.x_advance,fext.height);
					for (++nl; *nl == '\n'; *nl = '\0', nl++)
						cairo_rel_move_to(c,0,fext.height);
					part = nl;
				}
				if (nl) *nl = '\0';
				cairo_show_text(c,part);
				free(start);
			}
			else cairo_show_text(c,txt);
			cairo_get_current_point(c,&x,&y);
			if (y > r.y1) r.y1 = y+sz/9.0;
			/* draw a (ugly) cursor */
			if (ch > txt) {
				cairo_set_source_rgba(c,1.0,0.0,0.0,0.8);
				cairo_text_extents(c,ch,&ext);
				cairo_rel_move_to(c,-ext.x_advance,sz/10.0);
				cairo_rel_line_to(c,0,-sz);
				cairo_stroke(c);
			}
			XFlush(dpy);
			/* wait for keypress and process event */
			XMaskEvent(dpy,KeyPressMask,&ev);
			key = NoSymbol;
			inlen = XmbLookupString(xic,&ev.xkey,in,sizeof(in),&key,&stat);
			if (stat == XBufferOverflow) continue;
			else if (!iscntrl(*in)) {
				ts = strdup(ch); *ch = '\0';
				strcat(txt,in); strcat(txt,ts);
				free(ts); ch += strlen(in);
			}
			else if (key == XK_Return) {
				if (!multi) break;
				ts = strdup(ch); *ch = '\0';
				strcat(txt,"\n"); strcat(txt,ts);
				free(ts); ch += strlen(in);
			}
			else if (key == XK_Delete && *ch != '\0') {
				ts = strdup(ch+1); *ch = '\0';
				strcat(txt,ts); free(ts);
			}
			else if (key == XK_BackSpace && ch > txt) {
				ts = strdup(ch); *(--ch) = '\0';
				strcat(txt,ts); free(ts);
			}
			else if (key == XK_Left) { ch--; if (ch < txt) ch = txt; }
			else if (key == XK_Right && *ch != '\0') ch++;
			if (strlen(txt) > len - 2) {
				warn();
				txt[strlen(txt)-1] = '\0';
				if (ch > txt+strlen(txt)) ch=txt+strlen(txt);
			}
			if (multi) {
				char *nl;
				if ( !(nl=strrchr(txt,'\n')) ) nl=txt;
				cairo_text_extents(c,nl,&ext);
				if (ext.width > r.x2-r.x1 && (nl=strrchr(nl,' ')) )
					*nl = '\n';
			}
		}
		poppler_form_field_text_set_text(f,txt);
		/* clean up */
		free(txt);
		XDestroyIC(xic);
	}
	cairo_destroy(c);
	poppler_page_free_form_field_mapping(fmap);
	/* rerender page changes */
	t = cairo_xlib_surface_create(dpy,show->slide[show->cur],
			DefaultVisual(dpy,scr),sw,sh);
	c = cairo_create(t);
	cairo_surface_destroy(t);
	cairo_set_source_rgba(c,1,1,1,1);
		cairo_rectangle(c,0,0,sw,sh);
	cairo_fill(c);
	cairo_scale(c,show->scale,show->scale);
	poppler_page_render(page,c);
	cairo_destroy(c);
	draw(NULL);
	return;
}
#endif /* FORM_FILL */


void grab_keys(Bool grab) {
	if (grab) {
		int i; for (i = 0; i < 1000; i++) {
			if (XGrabKeyboard(dpy,root,True,GrabModeAsync,GrabModeAsync,
					CurrentTime) == GrabSuccess) break;
			usleep(5000);
		}
	}
	else {
		XUngrabKeyboard(dpy,CurrentTime);
	}
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
		v->buf = XCreatePixmap(dpy,root,show->w,show->h,
				DefaultDepth(dpy,scr));
		v->dest_c = cairo_xlib_surface_create(dpy,bnote,
				DefaultVisual(dpy,scr),swnote,shnote);
		v->cairo = cairo_create(v->dest_c);
		cairo_translate(v->cairo,v->x,v->y);
		cairo_scale(v->cairo, (float) v->w / (i ? show->w : show->notes->w),
				(float) v->h / (i ? show->h : show->notes->h) );
		v->src_c = cairo_xlib_surface_create(dpy,v->buf,
				DefaultVisual(dpy,scr),show->w,show->h);
		cairo_surface_t *t = cairo_xlib_surface_create(dpy,wshow,
				DefaultVisual(dpy,scr),sw+swnote,sh+shnote);
		cairo_t *c = cairo_create(t);
		cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
		cairo_set_source_rgba(c,1,1,1,0.5);
		cairo_set_line_width(c,10);
	}
}


void init_X() {
	XInitThreads();
	//g_type_init(); /* DEPRECATED IN GLIB 2.36 */
	if (!(dpy=XOpenDisplay(NULL))) die("Failed to open display\n");
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	XSetWindowAttributes wa;
	wa.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
			StructureNotifyMask;
	wa.override_redirect = True;
	/* get RandR info */
	XRRScreenResources *xrr_sr = XRRGetScreenResources(dpy,root);
	XRRCrtcInfo *xrrShow = NULL, *xrrNote = NULL, *xrrT = NULL;
	swnote = 0; shnote = 0;
	int i;
	XRROutputInfo *xrrOut;
	for (i = 0; i < xrr_sr->noutput; i++) {
		xrrOut = XRRGetOutputInfo(dpy,xrr_sr,xrr_sr->outputs[i]);
		if (xrrOut->crtc) {
			xrrT = XRRGetCrtcInfo(dpy,xrr_sr,xrrOut->crtc);
			if (monShow) {
				if (strncmp(monShow,xrrOut->name,strlen(monShow))==0)
					xrrShow = xrrT;
				if (monNote && strncmp(monNote,
						xrrOut->name,strlen(monNote))==0)
					xrrNote = xrrT;
			}
			else {
				if (!xrrNote) xrrNote = xrrT;
				else if (!xrrShow) xrrShow = xrrT;
				else { XRRFreeCrtcInfo(xrrShow); xrrShow = xrrT; }
			}
		}
		XRRFreeOutputInfo(xrrOut);
	}
	if (!xrrShow) { xrrShow = xrrNote; xrrNote = NULL; }
	/* Presentation monitor */
	if (!xrrShow) die("No monitors detected\n");
	sw = xrrShow->width;
	sh = xrrShow->height;
	show->w = sw; show->h = sh;
	wshow = XCreateSimpleWindow(dpy,root,xrrShow->x,xrrShow->y,
			sw,sh,1,0,0);
	bshow = XCreatePixmap(dpy,wshow,sw,sh,DefaultDepth(dpy,scr));
	XRRFreeCrtcInfo(xrrShow);
	XStoreName(dpy,wshow,"Slider");
	XClassHint *hint = XAllocClassHint();
	hint->res_name = "Slider";
	hint->res_class = "Slider";
	XSetClassHint(dpy,wshow,hint);
	XFree(hint);
	XChangeWindowAttributes(dpy,wshow,CWEventMask|CWOverrideRedirect,&wa);
	/* Notes monitor */
	if (xrrNote) {
		swnote = xrrNote->width;
		shnote = xrrNote->height;
		if (show->notes) { show->notes->w = swnote; show->notes->h = shnote; }
		wnote = XCreateSimpleWindow(dpy,root,xrrNote->x,xrrNote->y,
				swnote,shnote,1,0,0);
		bnote = XCreatePixmap(dpy,wshow,swnote,shnote,DefaultDepth(dpy,scr));
		XStoreName(dpy,wnote,"Slipper");
		XChangeWindowAttributes(dpy,wnote,CWEventMask|CWOverrideRedirect,&wa);
		XMapWindow(dpy,wnote);
		XRRFreeCrtcInfo(xrrNote);
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
	invisible_cursor = XCreatePixmapCursor(dpy,curs_map,curs_map,
			&color,&color,0,0);
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
	for (i = 0; i < nkeys; i++) {
		if ( (key==keys[i].key) && keys[i].func && (keys[i].mod==mod) )
			keys[i].func(keys[i].arg);
	}
}

void move(const char *arg) {
	if (mode & OVERVIEW) {
		if (arg[0] == 'r' || arg[0] == 'n') show->cur++;
		else if (arg[0] == 'l' || arg[0] == 'p') show->cur--;
		else if (arg[0] == 'd') show->cur += show->sorter->flag[0];
		else if (arg[0] == 'u') show->cur -= show->sorter->flag[0];
		if (show->cur < 0) show->cur = 0;
		while (show->cur >= show->count ||
				!(show->flag[show->cur] & RENDERED))
			show->cur--;
		overview(NULL);
	}
	else {
		if (arg[0] == 'd' || arg[0] == 'r' || arg[0] == 'n') show->cur++;
		else if (arg[0] == 'u' || arg[0] == 'l' || arg[0] == 'p') show->cur--;
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
	if ( (mode ^= MUTED) & MUTED ) {
		XFillRectangle(dpy,wshow,(arg[0]=='w'?cgc(White):cgc(Black)),
				0,0,sw,sh);
		XFlush(dpy);
	}
	else {
		draw(NULL);
	}
}

void overview(const char *arg) {
	mode |= OVERVIEW;
	XDefineCursor(dpy,wshow,None);
	XLockDisplay(dpy);
	XCopyArea(dpy,show->sorter->slide[0],wshow,gc,0,0,sw,sh,0,0);
	XUnlockDisplay(dpy);
	int grid = show->sorter->flag[0];
	int x = show->sorter->x + (show->cur % grid) * (show->sorter->w+10);
	int y = show->sorter->y + (int)(show->cur/grid) * (show->sorter->h+10);
	int w; double R,G,B,A;
	sscanf(OVERVIEW_RECT,"%d %lf,%lf,%lf %lf",&w,&R,&G,&B,&A);
	cairo_surface_t *t = cairo_xlib_surface_create(dpy,wshow,
			DefaultVisual(dpy,scr),sw+swnote,sh+shnote);
	cairo_t *c = cairo_create(t);
	cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
	cairo_set_source_rgba(c,R,G,B,A);
	cairo_set_line_width(c,w);
	cairo_rectangle(c,x,y,show->sorter->w,show->sorter->h);
	cairo_stroke(c);
	cairo_surface_destroy(t);
	cairo_destroy(c);
}

#ifdef DRAWING
#define PEN		0x01
#define POLKA	0x02
#define RECT	0x04
#define PERM	0x08
#define STRING	0x10
static void pen_polka_rect(const char *arg, int type) {
	int w; double R,G,B,A; char str[CURSOR_STRING_MAX] = "";
	sscanf(arg,"%d %lf,%lf,%lf %lf %s",&w,&R,&G,&B,&A,str);
	XWarpPointer(dpy,None,wshow,0,0,0,0,sw/2,sh/2);
	if (!(type & POLKA)) XDefineCursor(dpy,wshow,crosshair_cursor);
	XEvent ev; Bool on = False;
	Pixmap pbuf = XCreatePixmap(dpy,wshow,sw,sh,DefaultDepth(dpy,scr));
	Pixmap cbuf = XCreatePixmap(dpy,wshow,sw,sh,DefaultDepth(dpy,scr));
	XCopyArea(dpy,wshow,pbuf,gc,0,0,sw,sh,0,0);
	XCopyArea(dpy,wshow,cbuf,gc,0,0,sw,sh,0,0);
	cairo_surface_t *t = cairo_xlib_surface_create(dpy,pbuf,
			DefaultVisual(dpy,scr),sw+swnote,sh+shnote);
	cairo_t *c = cairo_create(t);
	cairo_set_line_join(c,CAIRO_LINE_JOIN_ROUND);
	cairo_set_source_rgba(c,R,G,B,A);
	cairo_set_line_width(c,w);
	if (type & POLKA) {
		if ( (type & STRING) ) {
			cairo_set_font_size(c, w);
			cairo_select_font_face(c,"sans-serif",
					CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
			cairo_move_to(c,sw/2.0,sh/2.0);
			cairo_show_text(c,str);
		}
		else {
			cairo_arc(c,sw/2.0,sh/2.0,(double)w,0,2*M_PI);
			cairo_fill(c);
		}
		XCopyArea(dpy,pbuf,wshow,gc,0,0,sw,sh,0,0);
	}
	XGrabPointer(dpy, wshow, True, PointerMotionMask | ButtonPressMask |
			ButtonReleaseMask,GrabModeAsync,GrabModeAsync,wshow,None,
			CurrentTime);
	while (!XNextEvent(dpy,&ev)) {
		if (ev.type == KeyPress) { XPutBackEvent(dpy,&ev); break; }
		else if ( !(type & POLKA) && ev.type == ButtonPress) {
			r.x = ev.xbutton.x; r.y = ev.xbutton.y;
			cairo_move_to(c,r.x,r.y);
			on = True;
		}
		else if ( (type & PEN) && ev.type == MotionNotify && on) {
			cairo_line_to(c,ev.xbutton.x,ev.xbutton.y);
			XCopyArea(dpy,cbuf,pbuf,gc,0,0,sw,sh,0,0);
			cairo_stroke_preserve(c);
			XCopyArea(dpy,pbuf,wshow,gc,0,0,sw,sh,0,0);
		}
		else if ( (type & POLKA) && ev.type == MotionNotify) {
			XCopyArea(dpy,cbuf,pbuf,gc,0,0,sw,sh,0,0);
			if ( (type & STRING) ) {
				cairo_move_to(c,ev.xbutton.x,ev.xbutton.y);
				cairo_show_text(c,str);
			}
			else /* SHAPE_DOT */
				cairo_arc(c,ev.xbutton.x,ev.xbutton.y,(double)w,0,2*M_PI);
			cairo_fill(c);
			XCopyArea(dpy,pbuf,wshow,gc,0,0,sw,sh,0,0);
		}
		else if ( (type & RECT) && ev.type == MotionNotify && on) {
			XCopyArea(dpy,cbuf,pbuf,gc,0,0,sw,sh,0,0);
			r.width = ev.xbutton.x - r.x; r.height = ev.xbutton.y - r.y;
			cairo_rectangle(c,r.x,r.y,r.width,r.height);
			cairo_stroke(c);
			XCopyArea(dpy,pbuf,wshow,gc,0,0,sw,sh,0,0);
		}
		else if ( (type & PEN) && ev.type == ButtonRelease) {
			on = False;
			XCopyArea(dpy,cbuf,pbuf,gc,0,0,sw,sh,0,0);
			cairo_stroke(c);
			XCopyArea(dpy,pbuf,wshow,gc,0,0,sw,sh,0,0);
			XCopyArea(dpy,wshow,cbuf,gc,0,0,sw,sh,0,0);
		}
		else if ( (type & RECT) && ev.type == ButtonRelease) { break; }
	}
	XUngrabPointer(dpy,CurrentTime);
	cairo_surface_destroy(t);
	cairo_destroy(c);
	XDefineCursor(dpy,wshow,invisible_cursor);
	XFreePixmap(dpy,pbuf);
	XFreePixmap(dpy,cbuf);
	if (type & PERM) XCopyArea(dpy,wshow,show->slide[show->cur],gc,
			show->x,show->y,show->w,show->h,0,0);
	if (type & POLKA) draw(NULL);
	else XFlush(dpy);
}
void pen(const char *arg) { pen_polka_rect(arg, PEN); }
void perm_pen(const char *arg) { pen_polka_rect(arg, PEN | PERM ); }
void perm_rect(const char *arg) { pen_polka_rect(arg, RECT | PERM ); }
void polka(const char *arg) { pen_polka_rect(arg, POLKA); }
void rectangle(const char *arg) { pen_polka_rect(arg, RECT); }
void string(const char *arg) { pen_polka_rect(arg, POLKA | STRING); }
#endif /* DRAWING */

void quit(const char *arg) { mode &= ~RUNNING; }

void usage(const char *prog) {
	printf("Usage: %s [OPTION...] SHOW_FILE [NOTES_FILE]\n"
			"See man page for details.\n",prog);
}

void warn() {
	Window w = wshow;
	if (swnote && shnote) w = wnote;
	cairo_surface_t *t = cairo_xlib_surface_create(dpy,w,
			DefaultVisual(dpy,scr),sw+swnote,sh+shnote);
	cairo_t *c = cairo_create(t);
	cairo_pattern_t *p = cairo_pattern_create_rgb(1,0,0);
	cairo_set_source(c,p);
	cairo_paint_with_alpha(c,0.5);
	cairo_surface_destroy(t); cairo_pattern_destroy(p); cairo_destroy(c);
	XFlush(dpy);
	usleep(500000);
	draw(NULL);
}

#ifdef ZOOMING
void zoom(const char *arg) {
	if (!(show->flag[show->count-1] & RENDERED)) { warn(); return; }
	pen_polka_rect(ZOOM_RECT, RECT);
	if (arg) { /* lock aspect ratio */
		double w_h = (double)show->w/show->h;
		if (r.width < w_h * r.height) r.width = w_h * r.height;
		else r.height = (double) r.width / w_h;
	}
	if (r.width < 20 || r.height < 20) { warn(); return; }
	r.x-=5; r.y-=5; r.width+=10; r.height+=10;
	int w,h;
	if (arg) { w = show->w; h = show->h; }
	else { w = sw; h = sh; }
	Pixmap b = XCreatePixmap(dpy,wshow,w,h,DefaultDepth(dpy,scr));
	XFillRectangle(dpy,b,cgc(SlideBG),0,0,w,h);
	PopplerDocument *pdf = poppler_document_new_from_file(show->uri,
			NULL,NULL);
	PopplerPage *page = poppler_document_get_page(pdf,show->cur);
	double pdfw, pdfh;
	poppler_page_get_size(page,&pdfw,&pdfh);
	cairo_surface_t *t=cairo_xlib_surface_create(dpy,b,
			DefaultVisual(dpy,scr),w,h);
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
#endif /* ZOOMING */

int main(int argc, const char **argv) {
	command_line(argc,argv);
	init_X();
	render(show,colors[ScreenBG],colors[SlideBG],colors[Empty]);
	/* grab keys */
	grab_keys(True);
	init_views();
	draw(NULL);
	XEvent ev;
	int xfd = ConnectionNumber(dpy);
	time_t now = time(NULL);
	struct timeval timeout;
	fd_set fds;
	while (mode & RUNNING) {
		FD_ZERO(&fds); FD_SET(xfd,&fds);
		memset(&timeout,0,sizeof(struct timeval));
		timeout.tv_sec = delay;
		select(xfd+1,&fds,0,0,(mode & AUTOPLAY ? &timeout : NULL));
		if (FD_ISSET(xfd,&fds)) while (XPending(dpy)) {
			XNextEvent(dpy,&ev);
			if (ev.type < 33 && handler[ev.type])
				handler[ev.type](&ev);
		}
		if ( (mode & AUTOPLAY) && (now + delay <= time(NULL)) ) {
			now = time(NULL);
			move("right");
		}
	}
	cleanup();
	return 0;
}
