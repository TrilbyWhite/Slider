/*****************************************************\
* RENDER.C
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

static Show *render(const char *, Bool);

Show *render_init(const char *fshow, const char *fnote) {
	Show *show;
	if (conf.interleave && !fnote) {
		show = render(fshow, True);
		show->notes = render(fshow, False);
	}
	else {
		conf.interleave = False;
		show = render(fshow, True);
		if (fnote) show->notes = render(fnote, False);
	}
	int i; Target *trg;
	for (i = 1; i < show->ntargets; i++) {
		trg = &show->target[i];
		trg->show = (conf.view[i-1].show ?
				(show->notes ? show->notes : show) : show);
		if (trg->show == show)
			cairo_scale(trg->ctx, trg->w / (float) show->w,
					trg->h / (float) show->h);
	}
	return show;
}

int render_free(Show *show) {
	int i;
	if (show->notes) {
		for (i = 0; i < show->notes->nslides; i++)
			cairo_surface_destroy(show->notes->slide[i]);
		free(show->notes->slide);
		free(show->notes->uri);
		free(show->notes);
	}
	for (i = 0; i < show->nslides; i++)
		cairo_surface_destroy(show->slide[i]);
	free(show->slide);
	free(show->uri);
	free(show);
	xlib_free();
	return 0;
}

Show *render(const char *fname, Bool X) {
	char *rpath = realpath(fname, NULL);
	if (!rpath) return NULL;
	Show *show = calloc(1, sizeof(Show));
	show->uri = malloc(strlen(rpath)+8);
	strcpy(show->uri,"file://"); strcat(show->uri,rpath);
	free(rpath);
	PopplerDocument *pdf;
	if (X) {
		xlib_init(show);
	}
	else {
		show->w = conf.view[0].w;
		show->h = conf.view[0].h;
	}
	if ( !(pdf=poppler_document_new_from_file(show->uri, NULL, NULL)) ) {
		fprintf(stderr, "\"%s\' is not a pdf file\n", show->uri);
		return NULL;
	}
	if ( !(show->nslides=poppler_document_get_n_pages(pdf)) ) {
		fprintf(stderr, "No pages found in \"%s\"\n", show->uri);
		return NULL;
	}
	if (conf.interleave) show->nslides /= 2;
	show->slide = calloc(show->nslides, sizeof(cairo_surface_t *));
	PopplerPage *page;
	double pdfw, pdfh;
	int i, p;
	cairo_t *ctx;
	for (i = 0; i < show->nslides; i++) {
		p = (conf.interleave ? 2 * i + (X ? 0 : 1) : i);
		page = poppler_document_get_page(pdf, p);
		poppler_page_get_size(page, &pdfw, &pdfh);
		show->slide[i] = cairo_image_surface_create(0, show->w, show->h);
		ctx = cairo_create(show->slide[i]);
if (conf.lock_aspect && X) {
	double scx = show->w / pdfw, scy = show->h / pdfh;
	if (scx > scy) {
		cairo_translate(ctx, (show->w - pdfw * scy) / 2.0, 0);
		cairo_scale(ctx, scy, scy);
	}
	else {
		cairo_translate(ctx, 0, (show->h - pdfh * scx) / 2.0);
		cairo_scale(ctx, scx, scx);
	}
}
else {
		cairo_scale(ctx, show->w / pdfw, show->h / pdfh);
}
		cairo_set_source_rgba(ctx, 1, 1, 1, 1);
		cairo_paint(ctx);
		poppler_page_render(page, ctx);
		cairo_destroy(ctx);
	}
	return show;
}

