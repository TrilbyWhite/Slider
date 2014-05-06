/*****************************************************\
* ACTIONS.C
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"
#include "xlib-actions.h"

static void grab_mouse() {
	XGrabPointer(dpy, wshow, True, PointerMotionMask | ButtonPressMask |
			ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			wshow, None, CurrentTime);
}

#define MAX_COMMAND	256
static void action_link(const char *cmd) {
	PopplerDocument *pdf;
	if (!(pdf=poppler_document_new_from_file(show->uri, NULL, NULL)))
		return;
	PopplerPage *page = poppler_document_get_page(pdf, show->cur);
	double pdfw, pdfh;
	poppler_page_get_size(page, &pdfw, &pdfh);
	GList *amap, *list;
	GList *al_pos = NULL;
	amap = poppler_page_get_link_mapping(page);
	PopplerAction *act = NULL;
	PopplerLinkMapping *pmap;
	PopplerRectangle r;
	/* check for "mouse" or number in command */
	int n, nn = 0;
	char *opt = strchr(cmd,' ');
	while (opt && *opt == ' ') opt++;
	if (!opt || *opt == '\0') {
		poppler_page_free_link_mapping(amap);
		return;
	}
	/* prep cairo context for drawing if in mouse mode */
	cairo_surface_t *t;
	cairo_t *ctx;
	if (!(n=atoi(opt))) {
		Theme q;
		sscanf(opt,"%*s %lf %lf %lf %lf %lf\n",
				&q.R, &q.G, &q.B, &q.A, &q.e);
		t = cairo_xlib_surface_create(dpy, wshow, vis, sw, sh);
		ctx = cairo_create(t);
		cairo_surface_destroy(t);
		cairo_scale(ctx, show->w/pdfw, show->h/pdfh);
		cairo_set_source_rgba(ctx, q.R, q.G, q.B, q.A);
		cairo_set_line_join(ctx, CAIRO_LINE_JOIN_ROUND);
		cairo_set_line_cap(ctx, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(ctx, q.e);
	}
	/* draw indicators or identify selected action */
	for (list = amap; list; list = list->next) {
		pmap = list->data;
		r = pmap->area;
		r.y1 = pdfh - r.y1; // TODO Check these
		r.y2 = pdfh - r.y2;
		if (pmap->action->type == POPPLER_ACTION_LAUNCH) {
			al_pos = g_list_append(al_pos, pmap);
		}
		if (!n) {
			cairo_rectangle(ctx, r.x1, r.y1, r.x2-r.x1, r.y2-r.y1);
			cairo_stroke(ctx);
		}
		else {
			act = pmap->action;
			if ( (++nn) == n ) break;
		}
	}
	if (n && !act) {
		poppler_page_free_link_mapping(amap);
		return;
	}
	/* get mouse selection */
	if (!n) {
		XUndefineCursor(dpy, wshow);
		XEvent ev;
		XMaskEvent(dpy, ButtonPressMask | KeyPressMask, &ev);
		if (ev.type == KeyPress) XPutBackEvent(dpy, &ev);
		else if (ev.type == ButtonPress) {
			for (list = amap; list; list = list->next) {
				pmap = list->data;
				r = pmap->area;
				r.y1 = pdfh - r.y1;
				r.y2 = pdfh - r.y2;
				if ( (r.x1 * show->w / pdfw < ev.xbutton.x) &&
						(r.x2 * show->w / pdfw > ev.xbutton.x) &&
						(r.y2 * show->h / pdfh < ev.xbutton.y) &&
						(r.y1 * show->h / pdfh > ev.xbutton.y) )
					act = pmap->action;
			}
		}
	}
	if (!act) {
		if (!n) cairo_destroy(ctx);
		poppler_page_free_link_mapping(amap);
		return;
	}
	char sys_cmd[MAX_COMMAND];
	memset(sys_cmd, 0, MAX_COMMAND);
	if (act->type == POPPLER_ACTION_GOTO_DEST) {
		PopplerDest *d, *dest = (&act->goto_dest)->dest;
		if (dest->type == POPPLER_DEST_NAMED) {
			d = poppler_document_find_dest(pdf, dest->named_dest);
			if (d) {
				show->cur = d->page_num - 1;
				poppler_dest_free(d);
			}
		}
		else {
			show->cur = dest->page_num - 1;
		}
	}
	else if (act->type == POPPLER_ACTION_NAMED) {
		PopplerActionNamed *n = &act->named;
		PopplerDest *d = poppler_document_find_dest(pdf, n->named_dest);
		if (d) {
			show->cur = d->page_num - 1;
			poppler_dest_free(d);
		}
	}
	else if (act->type == POPPLER_ACTION_LAUNCH) {
		PopplerActionLaunch *l = &act->launch;
		pmap = g_list_first(al_pos)->data;
		r = pmap->area;
		r.y1 = pdfh - r.y1;
		r.y2 = pdfh - r.y2;
		sprintf(sys_cmd,"%s %s %04g %04g %04g %04g",
		        l->file_name,
		        !l->params?"":l->params,
		        round(r.x1*show->w/pdfw),
		        round(r.y2*show->h/pdfh),
		        round((r.x2-r.x1)*show->w/pdfw),
		        round((r.y1-r.y2)*show->h/pdfh));
	}
	else if (act->type == POPPLER_ACTION_URI) {
		PopplerActionUri *u = &act->uri;
		sprintf(sys_cmd, conf.url_handler, u->uri);
	}
	else if (act->type == POPPLER_ACTION_MOVIE) {
		PopplerActionMovie *m = &act->movie;
		sprintf(sys_cmd, conf.mov_handler,
				poppler_movie_get_filename(m->movie));
	}
	else if (act->type == POPPLER_ACTION_RENDITION) {
		PopplerActionRendition *r = &act->rendition;
		sprintf(sys_cmd, conf.aud_handler,
				poppler_media_get_filename(r->media));
	}
	else {
		fprintf(stderr,"Action type %d not recognized\n", act->type);
	}
	if (sys_cmd[0] != '\0') system(sys_cmd);
	poppler_page_free_link_mapping(amap);
	if (!n) cairo_destroy(ctx);
	XSetInputFocus(dpy, wshow, RevertToPointerRoot, CurrentTime);
	draw(wshow);
}

static void custom(cairo_t *ctx, cairo_surface_t *buf,
		cairo_surface_t *cbuf, Theme *q, const char *str) {
	XEvent ev;
	double qe = q->e;
	while (!XNextEvent(dpy, &ev)) {
		if (ev.type == KeyPress) { XPutBackEvent(dpy, &ev); break; }
		else if (ev.type == ButtonPress) {
			if (ev.xbutton.button == 1) q->e += 4;
			else if (ev.xbutton.button == 2) q->e = qe;
			else if (ev.xbutton.button == 3) q->e -= 4;
			if (q->e < 4) q->e = 4;
			else if (q->e > sh/4.0) q->e = sh/4.0;
			ev.type = MotionNotify;
		}
		if (ev.type == MotionNotify) {
			cairo_set_source_surface(ctx, cbuf, 0, 0);
			cairo_paint(ctx);
			cairo_set_source_rgba(ctx, q->R, q->G, q->B, q->A);
			cairo_move_to(ctx, ev.xbutton.x, ev.xbutton.y);
			cairo_set_font_size(ctx, q->e);
			cairo_show_text(ctx, str);
			cairo_fill(ctx);
			cairo_set_source_surface(show->target[0].ctx, buf, 0, 0);
			cairo_paint(show->target[0].ctx);
		}
	}
}

static void dot(cairo_t *ctx, cairo_surface_t *buf,
		cairo_surface_t *cbuf, Theme *q) {
	XEvent ev;
	double qe = q->e;
	while (!XNextEvent(dpy, &ev)) {
		if (ev.type == KeyPress) { XPutBackEvent(dpy, &ev); break; }
		else if (ev.type == ButtonPress) {
			if (ev.xbutton.button == 1) q->e += 4;
			else if (ev.xbutton.button == 2) q->e = qe;
			else if (ev.xbutton.button == 3) q->e -= 4;
			if (q->e < 4) q->e = 4;
			else if (q->e > sh/4.0) q->e = sh/4.0;
			ev.type = MotionNotify;
		}
		if (ev.type == MotionNotify) {
			cairo_set_source_surface(ctx, cbuf, 0, 0);
			cairo_paint(ctx);
			cairo_set_source_rgba(ctx, q->R, q->G, q->B, q->A);
			cairo_arc(ctx, ev.xbutton.x, ev.xbutton.y, q->e, 0, 2*M_PI);
			cairo_fill(ctx);
			cairo_set_source_surface(show->target[0].ctx, buf, 0, 0);
			cairo_paint(show->target[0].ctx);
		}
	}
}

static void move(const char *cmd) {
	if (cmd[0] == 'p') show->cur --;
	else if (cmd[0] == 'n') show->cur ++;
	if (show->cur < 0) show->cur = 0;
	else if (show->cur >= show->nslides) {
		if (conf.loop) show->cur = 0;
		else show->cur = show->nslides - 1;
	}
	draw(None);
}

static void mute(const char *cmd) {
	// TODO
}

static void pen(cairo_t *ctx, cairo_surface_t *buf,
		cairo_surface_t *cbuf, Theme *q) {
	XEvent ev;
	Bool on = False;
	XDefineCursor(dpy, wshow, crosshair_cursor);
	while (!XNextEvent(dpy, &ev)) {
		if (ev.type == KeyPress) { XPutBackEvent(dpy, &ev); break; }
		else if (ev.type == ButtonPress) {
			if (ev.xbutton.button == 1 && (on = !on) ) {
				cairo_new_sub_path(ctx);
				cairo_move_to(ctx, ev.xbutton.x, ev.xbutton.y);
			}
			else if (ev.xbutton.button == 2) { break; }
			else if (ev.xbutton.button == 3) {
				// TODO save changes to slide
				break;
			}
		}
		else if (ev.type == MotionNotify && on) {
			cairo_set_source_surface(ctx, cbuf, 0, 0);
			cairo_paint(ctx);
			cairo_set_source_rgba(ctx, q->R, q->G, q->B, q->A);
			cairo_line_to(ctx, ev.xbutton.x, ev.xbutton.y);
			cairo_stroke_preserve(ctx);
			cairo_set_source_surface(show->target[0].ctx, buf, 0, 0);
			cairo_paint(show->target[0].ctx);
		}
	}
}

static void toggle_override() {
	static Bool on = True;
	XUnmapWindow(dpy,wshow);
	XFlush(dpy);
	XSetWindowAttributes wa;
	wa.override_redirect = (on = !on);
	XChangeWindowAttributes(dpy, wshow, CWOverrideRedirect, &wa);
	XMoveResizeWindow(dpy, wshow, (on ? 0 : show->w / 4),
			(on ? 0 : show->h / 4), (on ? show->w : show->w / 2),
			(on ? show->h : show->h / 2) );
	XMapWindow(dpy, wshow);
	XFlush(dpy);
	draw(wshow);
	XFlush(dpy);
	XSetInputFocus(dpy, wshow, RevertToPointerRoot, CurrentTime);
}

static void zoom_rect(int x1, int y1, int x2, int y2) {
	cairo_t *ctx;
	cairo_surface_t *buf, *t;
	t = cairo_xlib_surface_create(dpy, wshow, vis, sw, sh);
	buf = cairo_surface_create_similar(t, CAIRO_CONTENT_COLOR, sw, sh);
	ctx = cairo_create(buf);
	cairo_surface_destroy(t);
	PopplerDocument *pdf;
	pdf = poppler_document_new_from_file(show->uri, NULL, NULL);
	PopplerPage *page = poppler_document_get_page(pdf, show->cur);
	double pdfw, pdfh;
	poppler_page_get_size(page, &pdfw, &pdfh);
	cairo_set_source_rgba(ctx, 1, 1, 1, 1);
	cairo_paint(ctx);
	cairo_scale(ctx, show->w/(double) (x2-x1), show->h/(double) (y2-y1));
	cairo_translate(ctx, -1.0 * x1, -1.0 * y1);
	cairo_scale(ctx, show->w / pdfw, show->h / pdfh);
	poppler_page_render(page, ctx);
	cairo_set_source_surface(show->target[0].ctx, buf, 0, 0);
	int i;
	for (i = conf.fade; i; i--) {
		cairo_paint_with_alpha(show->target[0].ctx, 1/(float)i);
		XFlush(dpy);
		usleep(5000);
	}
	cairo_destroy(ctx);
	cairo_surface_destroy(buf);
}

static void zoom(cairo_t *ctx, cairo_surface_t *buf,
		cairo_surface_t *cbuf, Theme *q) {
	XEvent ev;
	double qe = q->e;
	int x1 = -1, y1, x2, y2;
	XDefineCursor(dpy, wshow, crosshair_cursor);
	while (!XNextEvent(dpy, &ev)) {
		if (ev.type == KeyPress) { XPutBackEvent(dpy, &ev); break; }
		else if (ev.type == ButtonPress && x1 < 0) {
			if (ev.xbutton.button == 1) {
				x1 = ev.xbutton.x;
				y1 = ev.xbutton.y;
			}
		}
		else if (ev.type == ButtonPress && x1 >= 0) {
			if (ev.xbutton.button == 1) {
				x2 = ev.xbutton.x;
				y2 = ev.xbutton.y;
				break;
			}
			else {
				x1 = -1;
			}
		}
		else if (ev.type == MotionNotify && x1 >= 0) {
			cairo_set_source_surface(ctx, cbuf, 0, 0);
			cairo_paint(ctx);
			cairo_set_source_rgba(ctx, q->R, q->G, q->B, q->A);
			cairo_rectangle(ctx, x1, y1, ev.xbutton.x - x1,
					ev.xbutton.y - y1);
			cairo_stroke(ctx);
			cairo_set_source_surface(show->target[0].ctx, buf, 0, 0);
			cairo_paint(show->target[0].ctx);
		}
	}
	zoom_rect(x1, y1, x2, y2);
}

#define CURSOR_STRING_MAX	24
static void pens(const char *cmd) {
	cairo_surface_t *buf, *cbuf, *t;
	cairo_t *ctx;
	char str[CURSOR_STRING_MAX];
	Theme q;
	sscanf(cmd, "%*s %lf %lf %lf %lf %lf %s\n",
			&q.R, &q.G, &q.B, &q.A, &q.e, str);
	XWarpPointer(dpy, None, wshow, 0, 0, 0, 0, sw/2, sh/2);
	/* create duplicate buffers */
	t = cairo_xlib_surface_create(dpy, wshow, vis, sw, sh);
	buf = cairo_surface_create_similar(t, CAIRO_CONTENT_COLOR, sw, sh);
	cbuf = cairo_surface_create_similar(t, CAIRO_CONTENT_COLOR, sw, sh);
	ctx = cairo_create(cbuf);
	cairo_set_source_surface(ctx, t, 0, 0);
	cairo_paint(ctx);
	cairo_destroy(ctx);
	ctx = cairo_create(buf);
	cairo_set_source_surface(ctx, t, 0, 0);
	cairo_paint(ctx);
	cairo_surface_destroy(t);
	/* set drawing parameters */
	cairo_set_line_join(ctx, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_cap(ctx, CAIRO_LINE_CAP_ROUND);
	cairo_set_source_rgba(ctx, q.R, q.G, q.B, q.A);
	cairo_set_line_width(ctx, q.e);
	// set font
	cairo_set_font_size(ctx, q.e);
	grab_mouse();
	/* call appropriate sub function */
	cairo_move_to(ctx, sw/2, sh/2);
	if (strncasecmp(cmd,"pen",3)==0) pen(ctx, buf, cbuf, &q);
	else if (strncasecmp(cmd,"dot",3)==0) dot(ctx, buf, cbuf, &q);
	else if (strncasecmp(cmd,"cust",4)==0) custom(ctx, buf, cbuf, &q, str);
	else if (strncasecmp(cmd,"zoom",4)==0) zoom(ctx, buf, cbuf, &q);
	/* clean up */
	XUngrabPointer(dpy, CurrentTime);
	cairo_surface_destroy(buf);
	cairo_surface_destroy(cbuf);
	cairo_destroy(ctx);
	XDefineCursor(dpy, wshow, invisible_cursor);
}

#define MARGIN		0.9
static void sorter(const char *cmd) {
	int i, j, n, nn = show->cur, grid, pn = -1;
	grid = (int) ceil(sqrt(show->nslides));
	cairo_surface_t *t = cairo_image_surface_create(0, show->w, show->h);
	cairo_t *ctx = cairo_create(t);
	cairo_scale(ctx, MARGIN/(float)grid, MARGIN/(float)grid);
	cairo_translate(ctx,
			(show->w * (1-MARGIN))/2.0,
			(show->w * (1-MARGIN))/2.0);
	grab_mouse();
	XEvent ev;
	XDefineCursor(dpy, wshow, None);
	while (True) {
		if (nn != pn) {
			pn = nn;
			cairo_set_source_rgba(ctx, 0, 0, 0, 1);
			cairo_paint(ctx);
			for (j = 0; j < grid; j++) for (i = 0; i < grid; i++) {
				if ( (n=j * grid + i) >= show->nslides ) break;
				cairo_set_source_surface(ctx, show->slide[n],
					(show->w * i)/MARGIN, (show->h * j)/MARGIN);
				cairo_paint_with_alpha(ctx,(n == nn ? 1.0 : 0.5));
			}
			cairo_set_source_surface(show->target[0].ctx, t, 0, 0);
			cairo_paint(show->target[0].ctx);
			XFlush(dpy);
		}
		XMaskEvent(dpy,PointerMotionMask|ButtonPressMask|KeyPressMask,&ev);
		if (ev.type == KeyPress) {
			switch (XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0)) {
				case 'h': case XK_Right: nn++; break;
				case 'j': case XK_Down: nn += grid; break;
				case 'k': case XK_Up: nn -= grid; break;
				case 'l': case XK_Left: nn--; break;
				case XK_space: case XK_Return: goto full_break_change; break;
				default: goto full_break_no_change;
			}
			if (nn < 0) nn = 0;
			else if (nn >= show->nslides) nn = show->nslides - 1;
		}
		else if (ev.type == ButtonPress) {
			if (ev.xbutton.button == 1) break;
			else if (ev.xbutton.button == 2) goto full_break_no_change;
			else if (ev.xbutton.button == 3) goto full_break_no_change;
		}
		else if (ev.type == MotionNotify) {
			nn = (ev.xbutton.y * grid / show->h) * grid +
					(ev.xbutton.x * grid / show->w);
		}
	}
	full_break_change:
	show->cur = nn;
	full_break_no_change:
	XUngrabKeyboard(dpy, CurrentTime);
	XUngrabPointer(dpy, CurrentTime);
	cairo_destroy(ctx);
	cairo_surface_destroy(t);
	XDefineCursor(dpy, wshow, invisible_cursor);
	draw(None);
}

int command(const char *cmd) {
	if (strncasecmp(cmd,"pen",3)==0) pens(cmd);
	else if (strncasecmp(cmd,"dot",3)==0) pens(cmd);
	else if (strncasecmp(cmd,"act",3)==0) action_link(cmd);
	else if (strncasecmp(cmd,"cust",4)==0) pens(cmd);
	else if (strncasecmp(cmd,"sort",4)==0) sorter(cmd);
	else if (strncasecmp(cmd,"prev",4)==0) move(cmd);
	else if (strncasecmp(cmd,"next",4)==0) move(cmd);
	else if (strncasecmp(cmd,"redr",4)==0) draw(None);
	else if (strncasecmp(cmd,"mute",4)==0) mute(cmd);
	else if (strncasecmp(cmd,"quit",4)==0) running = False;
	else if (strncasecmp(cmd,"full",4)==0) toggle_override();
	else if (strncasecmp(cmd,"zoom",4)==0) {
		char *c;
		if ( (c=strchr(cmd, ' ')) ) {
			while (c && *c == ' ') c++;
			if (*c == '\0') return 1;
			if (*c == 'q') {
				if (!(c=strchr(c, ' '))) return 1;
				while (c && *c == ' ') c++;
				if (*c == '\0') return 1;
				else if (*c == '1') zoom_rect(0, 0, show->w/2, show->h/2);
				else if (*c == '2') zoom_rect(show->w/2, 0, show->w, show->h/2);
				else if (*c == '3') zoom_rect(0, show->h/2, show->w/2, show->h);
				else if (*c == '4') zoom_rect(show->w/2, show->h/2, show->w, show->h);
				else return 1;
			}
			else pens(cmd);
		}
		else return 1;
	}
	XSync(dpy, True);
	return 0;
}
