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

enum { Black, White, ScreenBG, SlideBG, Empty };

#ifdef ACTION_LINKS
static void action(const char *);
#endif /* ACTION_LINKS */
static void buttonpress(XEvent *);
static GC cgc(int);
static void cleanup();
static void command_line(int, const char **);
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
#include "config.h"
static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress]	= buttonpress,
	[KeyPress]		= keypress,
};

#ifdef ACTION_LINKS
void action(const char *arg) {
	if (mode & OVERVIEW) return;
	if (!(show->flag[show->count-1] & RENDERED)) { warn(); return; }
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
	PopplerDocument *pdf = poppler_document_new_from_file(show->uri,NULL,NULL);
	PopplerPage *page = poppler_document_get_page(pdf,show->cur);
	GList *lm, *li;
	lm = poppler_page_get_link_mapping(page);
	PopplerLinkMapping *map;
	PopplerAction *act = NULL;
	PopplerRectangle r;
	int nact, nsel = 0;
	for (li = lm, nact = 0; li && nact < 26; li = li->next, nact++) {
		map = (PopplerLinkMapping *)li->data;
		r = map->area;
		r.y1 = show->h / show->scale - r.y1;
		r.y2 = show->h / show->scale - r.y2;
		/* draw links */
		cairo_set_source_rgba(c,R,G,B,A);
		cairo_rectangle(c,r.x1,r.y1,r.x2-r.x1,r.y2-r.y1);
		cairo_stroke(c);
		cairo_arc(c,r.x1,r.y2,tw,0,2*M_PI);
		cairo_fill(c);
		sym[0] = nact + 65;
		cairo_set_source_rgba(c,tR,tG,tB,tA);
		cairo_move_to(c,r.x1-tw/2.0,r.y2+tw/2.0);
		cairo_show_text(c,sym);
		cairo_stroke(c);
	}
	/* get key */
	if (!nact) return;
	XEvent ev;
	while (!XCheckTypedEvent(dpy,KeyPress,&ev));
	XKeyEvent *e = &ev.xkey;
	nsel = XKeysymToString(XkbKeycodeToKeysym(dpy,e->keycode,0,0))[0] - 97;
	/* get selected action link */
	for (li = lm, nact = 0; li; li = li->next, nact++) {
		map = (PopplerLinkMapping *)li->data;
		if (nact == nsel) act = map->action;
	}
	/* folow link */
	if (act) {
		grab_keys(False); /* release keyboard to interact with external prog. */
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
			char *cmd = calloc(strlen(l->file_name)+strlen(l->params)+2,
					sizeof(char));
			sprintf(cmd,"%s %s",l->file_name,l->params); system(cmd);free(cmd);
#else
			fprintf(stderr,"[SLIDER] blocked launch of \"%s %s\"\n",
					l->file_name,l->params);
#endif /* ALLOW_PDF_ACTION_LAUNCH */
		}
		else if (act->type == POPPLER_ACTION_URI) {
			PopplerActionUri *u = &act->uri;
			char *cmd = calloc(strlen(u->uri)+strlen(SHOW_URI)+2,sizeof(char));
			sprintf(cmd,SHOW_URI,u->uri); system(cmd); free(cmd);
		}
		else if (act->type == POPPLER_ACTION_MOVIE) {
			PopplerActionMovie *m = &act->movie;
			const char *mov = poppler_movie_get_filename(m->movie);
			char *cmd = calloc(strlen(mov)+strlen(SHOW_MOV)+2,sizeof(char));
			sprintf(cmd,SHOW_MOV,mov); system(cmd); free(cmd);
		}
		else if (act->type == POPPLER_ACTION_RENDITION) {
			PopplerActionRendition *r = &act->rendition;
			const char *med = poppler_media_get_filename(r->media);
			char *cmd = calloc(strlen(med)+strlen(PLAY_AUD)+2,sizeof(char));
			sprintf(cmd,PLAY_AUD,med); system(cmd); free(cmd);
		}
		else {
			fprintf(stderr,"unsupported action selected (type=%d)\n",act->type);
		}
		grab_keys(True);
		XRaiseWindow(dpy,wshow);
		if (swnote) XRaiseWindow(dpy,wnote);
	}
	poppler_page_free_link_mapping(lm);
	cairo_destroy(c);
	draw(NULL);
}
#endif /* ACTION_LINKS */

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
		modPDF = poppler_document_new_from_file("file:///tmp/fill.pdf",NULL,NULL);
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
	//PopplerDocument *pdf = poppler_document_new_from_file(show->uri,NULL,NULL);
	PopplerPage *page = poppler_document_get_page(modPDF,show->cur);
	GList *fmap, *list;
	PopplerRectangle r;
	PopplerFormField *f;
	fmap = poppler_page_get_form_field_mapping(page);
	for (list = fmap; list; list = list->next) {
		f = ((PopplerFormFieldMapping *)list->data)->field;
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
	XGrabPointer(dpy,wshow,True,ButtonPressMask,
			GrabModeAsync,GrabModeAsync,wshow,None,CurrentTime);
	XEvent ev;
	XMaskEvent(dpy,ButtonPressMask|KeyPressMask,&ev);
	if (ev.type == KeyPress) XPutBackEvent(dpy,&ev);
	else if (ev.type == ButtonPress) {
		mx = (ev.xbutton.x - show->x) / show->scale;
		my = (ev.xbutton.y - show->y) / show->scale;
	}
	XDefineCursor(dpy,wshow,invisible_cursor);
	XSync(dpy,True);
	draw(NULL);
	XUngrabPointer(dpy,CurrentTime);
	/* match coordinates to field */
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
		// TODO
	}
	else if (ft == POPPLER_FORM_FIELD_CHOICE) {
		// TODO
	}
	else if (ft == POPPLER_FORM_FIELD_SIGNATURE) {
		// TODO
	}
	else if (ft == POPPLER_FORM_FIELD_TEXT) { /* text fill */
		/* set up locale and input context */
		if (	!setlocale(LC_CTYPE,"") ||
				!XSupportsLocale() ||
				!XSetLocaleModifiers("") )
			fprintf(stderr,"locale failure\n");
		XIM xim = XOpenIM(dpy,NULL,NULL,NULL);
		XIC xic = XCreateIC(xim,XNInputStyle,XIMPreeditNothing|XIMStatusNothing,
				XNClientWindow, wshow, XNFocusWindow, wshow, NULL);
		/* get field info */
		float sz = poppler_form_field_get_font_size(f);
		sz = (sz ? sz : 12);
		cairo_set_font_size(c,sz);
		cairo_select_font_face(c,"sans-serif",
				CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		int len = poppler_form_field_text_get_max_len(f);
		if (!len) len = 255;
		char *txt = malloc((len+1)*sizeof(char));
		gchar *temp = poppler_form_field_text_get_text(f);
		if (temp) { strncpy(txt,temp,len); g_free(temp); }
		else txt[0] = '\0';
		/* event loop */
		KeySym key; Status stat;
		int inlen;
		char *ch = &txt[strlen(txt)], *ts;
		char in[8];
		cairo_set_line_width(c,1);
		cairo_text_extents_t ext;
		while (key != XK_Escape) {
			/* draw text and cursor */
			cairo_set_source_rgba(c,0.8,1.0,1.0,1.0); // TODO get a back color?
			cairo_rectangle(c,r.x1,r.y1,r.x2-r.x1,r.y2-r.y1);
			cairo_fill(c);
			cairo_set_source_rgba(c,0.0,0.0,0.0,1.0); // TODO get a font color?
			cairo_move_to(c,r.x1,r.y1-sz/10.0);
			cairo_show_text(c,txt);
			// an ugly cursor
			cairo_text_extents(c,ch,&ext);
			cairo_rel_move_to(c,-ext.x_advance,sz/10.0);
			cairo_rel_line_to(c,0,-sz);
			cairo_stroke(c);
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
				if (poppler_form_field_text_get_text_type(f) != 
						POPPLER_FORM_TEXT_MULTILINE) break;
				ts = strdup(ch); *ch = '\0';
				strcat(txt,"\n"); strcat(txt,ts);
				free(ts); ch += strlen(in);
			}
			else if (key == XK_Delete && *ch != '\0') {
				ts = strdup(ch+1); *ch = '\0';
				strcat(txt,ts); free(ts); 
			}
			else if (key == XK_Left) { ch--; if (ch < txt) ch = txt; }
			else if (key == XK_Right && *ch != '\0') ch++;
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
		v->buf = XCreatePixmap(dpy,root,show->w,show->h,DefaultDepth(dpy,scr));
		v->dest_c = cairo_xlib_surface_create(dpy,bnote,DefaultVisual(dpy,scr),
				swnote,shnote);
		v->cairo = cairo_create(v->dest_c);
		cairo_translate(v->cairo,v->x,v->y);
		cairo_scale(v->cairo, (float) v->w / (i ? show->w : show->notes->w),
				(float) v->h / (i ? show->h : show->notes->h) );
		v->src_c = cairo_xlib_surface_create(dpy,v->buf,DefaultVisual(dpy,scr),
				show->w,show->h);
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
#if !defined(GLIB_VERSION_2_36)
        g_type_init();
#endif
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
				if (monNote && strncmp(monNote,xrrOut->name,strlen(monNote))==0)
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
		while (show->cur >= show->count || !(show->flag[show->cur] & RENDERED))
			show->cur--;
		overview(NULL);
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
	if ( (mode ^= MUTED) & MUTED ) {
		XFillRectangle(dpy,wshow,(arg[0]=='w'?cgc(White):cgc(Black)),0,0,sw,sh);
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
static void pen_polka_rect(const char *arg, int type) {
	int w; double R,G,B,A;
	sscanf(arg,"%d %lf,%lf,%lf %lf",&w,&R,&G,&B,&A);
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
		cairo_arc(c,sw/2,sh/2,(double)w,0,2*M_PI);
		cairo_fill(c);
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
#endif /* DRAWING */

void quit(const char *arg) { mode &= ~RUNNING; }

void usage(const char *prog) {
	printf("Usage: %s [OPTION...] SHOW_FILE [NOTES_FILE]\n"
			"See man page for details.\n",prog); 
}

void warn() {
	Window w = wshow;
	if (swnote && shnote) w = wnote;
	cairo_surface_t *t = cairo_xlib_surface_create(dpy,w,DefaultVisual(dpy,scr),
			sw+swnote,sh+shnote);
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

