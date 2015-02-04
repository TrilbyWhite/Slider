/*****************************************************\
* RANDR.C
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"
#include <X11/extensions/Xrandr.h>

typedef struct Rect {
	int x, y, w, h;
} Rect;

int _screen_info(const char *, int, Rect *);

int randr_init() {
	Rect r;
// TODO strtok_r from get_s(videoOut);
	/* try to put presentation on specified output: */
	int i, n = _screen_info("LVDS1", 0, &r);
	/* if that failed, get the first available output: */
	if (!(r.w && r.h)) for (i = 1; i < n; ++i) _screen_info(NULL, i, &r);
	if (!(r.w && r.h)) return 1;
	set(presX, r.x); set(presY, r.y); set(presW, r.w); set(presH, r.h);
	/* try to put notes on the specified output: */
	r.x = r.y = r.w = r.h = 0;
	_screen_info("VGA1", 0, &r);
	/* if that failed, get the next available output (if any): */
	if (!(r.w && r.h)) for (i = 1; i < n; ++i) _screen_info(NULL, i, &r);
	if (!(r.w && r.h)) return 0;
	set(noteX, r.x); set(noteY, r.y);
	return 0;
}

int randr_free() {
	return 0;
}

int _screen_info(const char *name, int num, Rect *r) {
	XRRScreenResources *res = XRRGetScreenResources(dpy, root);
	if (!res) return 0;
	int i, n = res->noutput;
	XRROutputInfo *info;
	XRRCrtcInfo *crtc;
	r->w = r->h = 0;
	for (i = 0; i < n; ++i) {
		/* skip if output lacks info or crtc */
		if (!(info=XRRGetOutputInfo(dpy, res, res->outputs[i]))) continue;
		if (!info->crtc || !(crtc=XRRGetCrtcInfo(dpy,res,info->crtc))) {
			XRRFreeOutputInfo(info);
			continue;
		}
		/* skip if name is provided and doesn't match */
		/* and skip if number is provided and doesn't match */
		if ( !(name && strncmp(name, info->name, strlen(name))) ||
				!(num && num - 1 != i) ) {
			/* match found, return size */
			r->x = crtc->x;
			r->y = crtc->y;
			r->w = crtc->width;
			r->h = crtc->height;
		}
		XRRFreeCrtcInfo(crtc);
		XRRFreeOutputInfo(info);
		if (r->w && r->h) break;
	}
	XRRFreeScreenResources(res);
	return n;
}


