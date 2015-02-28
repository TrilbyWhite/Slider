/*****************************************************\
  RENDER.C
  By: Jesse McClure (c) 2015
  See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

typedef struct PDF {
	PopplerDocument *doc;
	int npage;
} PDF;

static PDF *_pdf = NULL;
static Window _fade_win = None;
static int _n = 0, _fade_steps;

int render_set_fader(Window win, int steps) {
	_fade_win = win;
	_fade_steps = steps;
	return 0;
}

PopplerDocument *render_get_pdf_ptr() {
	return _pdf[0].doc;
}

int render_page(int pg, Window win, bool fixed) {
	render_pdf_page(0, pg, win, fixed);
	//notes_draw(pg);
	return 0;
}

int render_pdf_page(int pdf, int pg, Window win, bool fixed) {
	if (pg >= _pdf[pdf].npage) return 1;
	cairo_t *ctx;
	cairo_surface_t *s, *img;
	double pdfw, pdfh, scx, scy;
	int ig;
	unsigned int uig, ww, wh;
	/* get page size and scale to window */
	PopplerPage *page = poppler_document_get_page(_pdf[pdf].doc, pg);
	poppler_page_get_size(page, &pdfw, &pdfh);
	XGetGeometry(dpy, win, (Window *) &ig, &ig, &ig, &ww, &wh, &uig, &uig);
	scx = ww / pdfw; scy = wh / pdfh;
	//if (pdf == 0) { _ww = ww; _wh = wh; }
	if (fixed) { /* adjust window size for fixed aspect ratio */
		scx = (scx < scy ? (scy=scx) : scy);
		ww = scx * pdfw + 0.5; wh = scy * pdfh + 0.5;
		XResizeWindow(dpy, win, ww, wh);
	}
	/* create background pixmap and render page to it */
	Pixmap pix = XCreatePixmap(dpy, win, ww, wh, DefaultDepth(dpy,scr));
	s = cairo_xlib_surface_create(dpy, pix, DefaultVisual(dpy,scr), ww, wh);
	ctx = cairo_create(s);
	cairo_surface_destroy(s);
	cairo_scale(ctx, scx, scy);
	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_paint(ctx);
	poppler_page_render(page, ctx);
	cairo_destroy(ctx);
	/* if this is the fader window, fade in */
	if (win == _fade_win) {
		/* hide cursor for fade transitions */
#ifdef module_cursor
		bool pre = cursor_visible(query);
		cursor_visible(false);
#endif
		/* get image, set up context, and render page */
		img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ww, wh);
		ctx = cairo_create(img);
		cairo_scale(ctx, scx, scy);
		cairo_set_source_rgb(ctx, 1, 1, 1);
		cairo_paint(ctx);
		poppler_page_render(page, ctx);
		cairo_destroy(ctx);
		/* get window surface, reset context to draw img to window */
		s = cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy,scr), ww, wh);
		ctx = cairo_create(s);
		cairo_surface_destroy(s);
		cairo_set_source_surface(ctx, img, 0, 0); // memory leak?!
		/* fade in */
		for (ig = _fade_steps; ig; --ig) {
			cairo_paint_with_alpha(ctx, 1 / (float) ig); // memory leak?!
			XFlush(dpy);
			usleep(10000);
		}
		/* clean up */
		cairo_destroy(ctx);
		cairo_surface_destroy(img);
#ifdef module_cursor
		cursor_visible(pre);
#endif
	}
	g_object_unref(page);
	/* set the pixmap to be the window background */
	XSetWindowBackgroundPixmap(dpy, win, pix);
	XFreePixmap(dpy, pix);
	XClearWindow(dpy, win);
#ifdef module_cursor
	if (pdf == 0) cursor_draw(-1, -1);
#endif
	return 0;
}

int render_init(char *fname) {
	char *path = realpath(fname, NULL);
	if (!path) return 1;
	char *uri = malloc(strlen(path) + 8);
	sprintf(uri, "file://%s", path);
	free(path);
_pdf = realloc(_pdf, (_n + 1) * sizeof(PDF));
	_pdf[_n].doc = poppler_document_new_from_file(uri, NULL, NULL);
	_pdf[_n].npage = poppler_document_get_n_pages(_pdf[_n].doc);
++_n;
	free(uri);
	return 0;
}

int render_free() {
	if (_pdf) free(_pdf);
	return 0;
}

Window *render_create_sorter(Window win) {
	int i;
	PopplerPage *page;
	double pdfw, pdfh, scale;
	cairo_surface_t *pix_s;
	cairo_t *pix_c;

	XSetWindowAttributes wa;
	wa.backing_store = Always;
	wa.event_mask = ButtonPress | ExposureMask | PointerMotionMask;
	Window *wins = malloc((_pdf[0].npage + 1) * sizeof(Window));
	Pixmap pix;
	int ww, wh;
	for (i = 0; i < _pdf[0].npage; ++i) {
		page = poppler_document_get_page(_pdf[0].doc, i);
		poppler_page_get_size(page, &pdfw, &pdfh);
		scale = get_d(presH) / (6 * pdfh);
		ww = scale * pdfw; wh = scale * pdfh;
		wins[i] = XCreateWindow(dpy, win, 0, 0, ww, wh, 1, DefaultDepth(dpy,scr),
				InputOutput, DefaultVisual(dpy,scr), CWBackingStore | CWEventMask, &wa);
		XDefineCursor(dpy, wins[i], XCreateFontCursor(dpy, 68));
		pix = XCreatePixmap(dpy, wins[i], ww, wh, DefaultDepth(dpy,scr));
		pix_s = cairo_xlib_surface_create(dpy, pix, DefaultVisual(dpy,scr), ww, wh);
		pix_c = cairo_create(pix_s);
		cairo_surface_destroy(pix_s);
		cairo_set_source_rgb(pix_c, 1, 1, 1);
		cairo_paint(pix_c);
		cairo_scale(pix_c, scale, scale);
		poppler_page_render(page, pix_c);
		cairo_destroy(pix_c);
		XSetWindowBackgroundPixmap(dpy, wins[i], pix);
		XFreePixmap(dpy, pix);
		g_object_unref(page);
	}
	wins[_pdf[0].npage] = None;
	return wins;
}

