/*****************************************************\
  RENDER.C
  By: Jesse McClure (c) 2015
  See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

static PopplerDocument *_pdf = NULL;
static Window fade_win = None;
static int _npage, fade_steps, _ww, _wh;

int render_set_fader(Window win, int steps) {
	fade_win = win;
	fade_steps = steps;
}

int render_page(int i, Window win, bool fixed) {
	if (i >= _npage) return 1;
	cairo_t *ctx;
	cairo_surface_t *s, *img;
	double pdfw, pdfh, scx, scy;
	unsigned int ig;
	/* get page size and scale to window */
	PopplerPage *page = poppler_document_get_page(_pdf, i);
	poppler_page_get_size(page, &pdfw, &pdfh);
	XGetGeometry(dpy, win, (Window *) &ig, &ig, &ig, &_ww, &_wh, &ig, &ig);
	scx = _ww / pdfw; scy = _wh / pdfh;
	if (fixed) { /* adjust window size for fixed aspect ratio */
		scx = (scx < scy ? (scy=scx) : scy);
		_ww = scx * pdfw + 0.5; _wh = scy * pdfh + 0.5;
		XResizeWindow(dpy, win, _ww, _wh);
	}
	/* create background pixmap and render page to it */
	Pixmap pix = XCreatePixmap(dpy, win, _ww, _wh, DefaultDepth(dpy,scr));
	s = cairo_xlib_surface_create(dpy, pix, DefaultVisual(dpy,scr), _ww, _wh);
	ctx = cairo_create(s);
	cairo_surface_destroy(s);
	cairo_scale(ctx, scx, scy);
	cairo_set_source_rgb(ctx, 1, 1, 1);
	cairo_paint(ctx);
	poppler_page_render(page, ctx);
	cairo_destroy(ctx);
	/* if this is the fader window, fade in */
	if (win == fade_win) {
		/* hide cursor for fade transitions */
		bool pre = cursor_visible(query);
		cursor_visible(false);
		/* get image, set up context, and render page */
		img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, _ww, _wh);
		ctx = cairo_create(img);
		cairo_scale(ctx, scx, scy);
		cairo_set_source_rgb(ctx, 1, 1, 1);
		cairo_paint(ctx);
		poppler_page_render(page, ctx);
		cairo_destroy(ctx);
		/* get window surface, reset context to draw img to window */
		s = cairo_xlib_surface_create(dpy, win, DefaultVisual(dpy,scr), _ww, _wh);
		ctx = cairo_create(s);
		cairo_surface_destroy(s);
		cairo_set_source_surface(ctx, img, 0, 0); // memory leak?!
		/* fade in */
		for (ig = fade_steps; ig; --ig) {
			cairo_paint_with_alpha(ctx, 1 / (float) ig); // memory leak?!
			XFlush(dpy);
			usleep(10000);
		}
		/* clean up */
		cairo_destroy(ctx);
		cairo_surface_destroy(img);
		cursor_visible(pre);
	}
	g_object_unref(page);
	/* set the pixmap to be the window background */
	XSetWindowBackgroundPixmap(dpy, win, pix);
	XFreePixmap(dpy, pix);
	XClearWindow(dpy, win);
	cursor_draw(-1, -1);
	return 0;
}

int render_init() {
	char *path = realpath(get_s(presFile), NULL);
	if (!path) return 1;
	char *uri = malloc(strlen(path) + 8);
	sprintf(uri, "file://%s", path);
	free(path);
	_pdf = poppler_document_new_from_file(uri, NULL, NULL);
	_npage = poppler_document_get_n_pages(_pdf);
	free(uri);
	return 0;
}

int render_free() {
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
	Window *wins = malloc((_npage + 1) * sizeof(Window));
	Pixmap pix;
	int ww, wh;
	for (i = 0; i < _npage; ++i) {
		page = poppler_document_get_page(_pdf, i);
		poppler_page_get_size(page, &pdfw, &pdfh);
		scale = _wh / (6 * pdfh);
		ww = scale * pdfw; wh = scale * pdfh;
		wins[i] = XCreateWindow(dpy, win, 0, 0, ww, wh, 1, DefaultDepth(dpy,scr),
				InputOutput, DefaultVisual(dpy,scr), CWBackingStore | CWEventMask, &wa);
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
	wins[_npage] = None;
	return wins;
}

