/*  SLIDER by Jesse McClure, Copyright 2012-2013
	See COPYING for license information */

#include "slider.h"

static pthread_t show_render, note_render;

void *render_threaded(void *arg) {
	Show *show = (Show *) arg;
	int i, j, n, x, y, grid = 0;
	int sw = show->w, sh = show->h;
	/* open pdf and create Show */
	PopplerDocument *pdf;
	if ( !(pdf=poppler_document_new_from_file(show->uri,NULL,NULL)) )
		die("\"%s\" is not a pdf file\n",show->uri);
	if ( !(show->count=poppler_document_get_n_pages(pdf)) )
		die("getting number of pages from \"%s\"\n",show->uri);
	if ( !(show->slide=(Pixmap *) calloc(show->count, sizeof(Pixmap))) )
		die("allocating pixmap memory");
	if ( !(show->flag=(int *) calloc(show->count,sizeof(int))) )
		die("allocating flags memory");
	/* scaling calculations */
	double pdfw, pdfh;
	PopplerPage *page = poppler_document_get_page(pdf,0);
	poppler_page_get_size(page,&pdfw,&pdfh);
	float hsc = show->w/pdfw, vsc = show->h/pdfh;
	if (hsc > vsc) {
		show->scale = vsc;
		show->w = pdfw * show->scale;
		show->x = (sw - show->w)/2;
	}
	else {
		show->scale = hsc;
		show->h = pdfh * show->scale;
		show->y = (sh - show->h)/2;
	}
	/* create sorter / overview */
	cairo_surface_t *tsorter = NULL;
	cairo_t *csorter = NULL;
	if (show->sorter) {
		/* scaling calculations for sorter */
		show->sorter->count = 1;
		show->sorter->slide = (Pixmap *) calloc(1, sizeof(Pixmap));
		show->sorter->flag = (int *) calloc(1,sizeof(int));
		grid = (int) ceil(sqrt(show->count)) ;
		vsc = ( (sh-10)/grid - 10) / pdfh;
		hsc = ( (sw-10)/grid - 10) / pdfw;
		show->sorter->flag[0] = grid;
		show->sorter->scale = (vsc > hsc ? hsc : vsc);
		show->sorter->h = pdfh * show->sorter->scale;
		show->sorter->w = pdfw * show->sorter->scale;
		show->sorter->x = (sw - (show->sorter->w+10)*grid + 10)/2;
		show->sorter->y = (sh - (show->sorter->h+10)*grid + 10)/2;
		/* create empty sorter frame */
		show->sorter->slide[0] = XCreatePixmap(dpy,root,sw,sh,
				DefaultDepth(dpy,scr));
		tsorter = cairo_xlib_surface_create(dpy,show->sorter->slide[0],
				DefaultVisual(dpy,scr),sw,sh);
		if (cairo_surface_status(tsorter) != CAIRO_STATUS_SUCCESS)
			die("creating cairo xlib surface for sorter\n");
		csorter = cairo_create(tsorter);
		cairo_set_source_rgba(csorter,0,0,0,1);
		cairo_rectangle(csorter,0,0,sw,sh);
		cairo_fill(csorter);
		cairo_set_source_rgba(csorter,0.2,0.2,0.4,1);
		n = 0; y = show->sorter->y;
		for (i = 0; i < grid; i++, y += show->sorter->h + 10) {
			x = show->sorter->x;
			for (j = 0; j < grid; j++, x+= show->sorter->w + 10) {
				if (++n > show->count) break;
				cairo_rectangle(csorter,x+2,y+2,
						show->sorter->w-4,show->sorter->h-4);
			}
		}
		cairo_fill(csorter);
		cairo_scale(csorter,show->sorter->scale,show->sorter->scale);
	}	
	/* render pages */
	cairo_surface_t *target, *xtarget;
	cairo_t *cairo;
	n = 0; x = (show->sorter ? show->sorter->x : 0);
	y = (show->sorter ? show->sorter->y : 0);
	for (i = 0; i < show->count && !(show->flag[0] & STOP_RENDER); i++) {
		show->slide[i] = XCreatePixmap(dpy,root,show->w,show->h,
				DefaultDepth(dpy,scr));
		page = poppler_document_get_page(pdf,i);
/* not sure why the "middle man" image surface is needed here
	but fontconf replacements don't seem to work without it.
	This may be a limitation of rendering on xlib surfaces.		*/
		target = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				show->w,show->h);
		if (cairo_surface_status(target) != CAIRO_STATUS_SUCCESS)
			die("creating cairo image surface\n");
		cairo = cairo_create(target);
		cairo_scale(cairo,show->scale,show->scale);
		cairo_set_source_rgba(cairo,1.0,1.0,1.0,1.0);
		cairo_rectangle(cairo,0,0,pdfw,pdfh);
		cairo_fill(cairo); // Segfault here in notes thread ??
		poppler_page_render(page,cairo);
		cairo_destroy(cairo);
		xtarget = cairo_xlib_surface_create(dpy,show->slide[i],
				DefaultVisual(dpy,scr),show->w,show->h);
		if (cairo_surface_status(xtarget) != CAIRO_STATUS_SUCCESS)
			die("creating cairo xlib surface\n");
		cairo = cairo_create(xtarget);
		cairo_surface_destroy(xtarget);
		cairo_set_source_surface(cairo,target,0,0);
		cairo_paint(cairo);
		cairo_destroy(cairo);
		show->flag[i] |= RENDERED;
		if (show->sorter) {
			cairo_save(csorter);
			cairo_translate(csorter,x/show->sorter->scale,
					y/show->sorter->scale);
			cairo_set_source_rgba(csorter,1.0,1.0,1.0,1.0);
			cairo_rectangle(csorter,0,0,pdfw,pdfh);
			cairo_fill(csorter);
			poppler_page_render(page,csorter);
			cairo_restore(csorter);
			x += show->sorter->w + 10;
			if (++n == grid) {
				n = 0;
				x = show->sorter->x;
				y += show->sorter->h + 10;
			}
		}
		cairo_surface_destroy(target);
	}
	if (show->sorter) {
		cairo_surface_destroy(tsorter);
		cairo_destroy(csorter);
	}
	return NULL;
}


void render(Show *show) {
/* note: unique times in usleeps make filtering through
strace output easier */
	/* render show */
	pthread_create(&show_render,NULL,&render_threaded,(void *) show);
	while (!(show->flag)) usleep(5100);
	if (prerender <= 0 || prerender > show->count) prerender = show->count;
	while (!(show->flag[(prerender>0?prerender:1)-1] & RENDERED))
		usleep(5200);
	/* render notes */
	if (show->notes && show->notes->uri) {
/* ensure show is fully rendered before rendering notes
I'm not sure why this is needed yet, but it prevents 
sporadic segfaults */
while (!(show->flag[show->count-1] & RENDERED))
usleep(5250);
		pthread_create(&note_render,NULL,render_threaded,
				(void *) show->notes);
		while (!(show->notes->flag)) usleep(5300);
		if (prerender > show->notes->count) prerender = show->notes->count;
		while (!(show->notes->flag[(prerender>0?prerender:1)-1] & RENDERED))
			usleep(54000);
	}
}

void free_renderings(Show *show) {
	if (!show) return;
	show->flag[0] |= STOP_RENDER;
	if (note_render) show->notes->flag[0] |= STOP_RENDER;
	pthread_join(show_render,NULL);
	if (note_render) pthread_join(note_render,NULL);
	int i;
	for (i = 0; i < show->count; i++) {
		XFreePixmap(dpy,show->slide[i]);
	}
	free(show->slide);
	free(show->flag);
	if (show->sorter) {
		XFreePixmap(dpy,show->sorter->slide[0]);
		free(show->sorter->slide); free(show->sorter->flag);
	}
	if (show->notes) {
		for (i = 0; i < show->notes->count; i++)
			XFreePixmap(dpy,show->notes->slide[i]);
		free(show->notes->slide); free(show->notes->flag);
	}
}

void die(const char *msg,...) {
	va_list arg;
	va_start(arg,msg);
	vfprintf(stderr,msg,arg);
	va_end(arg);
	exit(1);
}

