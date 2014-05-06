/*****************************************************\
* CONFIG.C
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

static int config_buttons(XrmDatabase, const char *);
static int config_binds(XrmDatabase, const char *);
static int config_general(XrmDatabase, const char *);
static int config_views(XrmDatabase, const char *);

FT_Face face;
static XrmDatabase xrdb;

int config_init(const char *mode, const char *db) {
	const char *pwd = getenv("PWD");
	XrmInitialize();
	memset(conf.button, 0, NBUTTON);
	conf.button[0] = "next";
	conf.button[2] = "prev";
	if (db) xrdb = XrmGetFileDatabase(db);
	else if ( (!chdir(getenv("XDG_CONFIG_HOME")) && !chdir("slider")) ||
			(!chdir(getenv("HOME")) && !chdir(".config/slider")) )
		xrdb = XrmGetFileDatabase("config");
	if (!xrdb) xrdb = XrmGetFileDatabase("/usr/share/slider/config");
	if (!xrdb) die("cannot find a configuration resource database");
	char *_base = "Slider", *class = "Slider.Mode", *type;
	const char *base;
	XrmValue val;
	if (mode) base = mode;
	else if (XrmGetResource(xrdb, class, class, &type, &val))
		base = val.addr;
	else base = _base;
	config_buttons(xrdb, base);
	config_binds(xrdb, base);
	config_views(xrdb, base);
	config_general(xrdb, base);
	chdir(pwd);
	return 0;
}

int config_free() {
	XrmDestroyDatabase(xrdb);
	int i;
	cairo_font_face_destroy(conf.font);
	FT_Done_Face(face);
	free(conf.key);
	conf.key = NULL;
	conf.nkeys = 0;
	free(conf.view);
	conf.view = NULL;
	conf.nviews = 0;
	return 0;
}

/**** Local functions ****/

int config_general(XrmDatabase xrdb, const char *base) {
	char class[256], *type;
	XrmValue val;
	conf.mon = -1;
	conf.launch = conf.interleave = conf.loop = conf.lock_aspect = False;
	sprintf(class,"%s.Font",base);
	if (XrmGetResource(xrdb, class, class, &type, &val)) {
		if (FT_New_Face(ftlib, val.addr, 0, &face) |
				FT_Set_Pixel_Sizes(face, 0, conf.font_size))
			fprintf(stderr,"unable to load font \"%s\"\n", val.addr);
		else
			conf.font = cairo_ft_font_face_create_for_ft_face(face, 0);
	}
	sprintf(class,"%s.Action.URL",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		conf.url_handler = val.addr;
	sprintf(class,"%s.Action.Movie",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		conf.mov_handler = val.addr;
	sprintf(class,"%s.Action.Audio",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		conf.aud_handler = val.addr;
	sprintf(class,"%s.Fade",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		conf.fade = atoi(val.addr);
	sprintf(class,"%s.Monitor",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		conf.mon = atoi(val.addr);
	sprintf(class,"%s.Loop",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		if (*val.addr == 'T' || *val.addr == 't') conf.loop = True;
	sprintf(class,"%s.LockAspect",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		if (*val.addr == 'T' || *val.addr == 't') conf.lock_aspect = True;
	sprintf(class,"%s.Interleave",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		if (*val.addr == 'T' || *val.addr == 't') conf.interleave = True;
	sprintf(class,"%s.Action.Launch",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		if (*val.addr == 'T' || *val.addr == 't') conf.launch = True;
	return 0;
}

int config_buttons(XrmDatabase xrdb, const char *base) {
	char class[256], *type;
	XrmValue val;
	int i;
	for (i = 1; i < NBUTTON + 1; i++) {
		sprintf(class,"%s.Button.%02d",base,i);
		if (!XrmGetResource(xrdb, class, class, &type, &val)) continue;
		conf.button[i - 1] = val.addr;
	}
	return 0;
}

#define MAX_MOD	4
int config_binds(XrmDatabase xrdb, const char *base) {
	/* get modifiers */
	int mods[MAX_MOD] = {0, ControlMask, Mod1Mask, ShiftMask};
	char *ord[MAX_MOD] = { "None", "Control", "Alt", "Shift" };
	char class[256], *type;
	XrmValue val;
	int i, j, tmod;
	/* loop through bindings */
	conf.key = NULL;
	conf.nkeys = 0;
	KeySym sym;
	for (i = 0; i < 100; i++) {
		sprintf(class,"%s.Bind.%02d.Key",base,i);
		if (!XrmGetResource(xrdb, class, class, &type, &val)) continue;
		if ( (sym=XStringToKeysym(val.addr)) == NoSymbol ) continue;
		for (j = 0; j < MAX_MOD; j++) {
			sprintf(class,"%s.Bind.%02d.%s",base,i,ord[j]);
			if (!XrmGetResource(xrdb, class, class, &type, &val)) continue;
			conf.key = realloc(conf.key, (conf.nkeys+1) * sizeof(Key));
			conf.key[conf.nkeys].keysym = sym;
			conf.key[conf.nkeys].mod = mods[j];
			conf.key[conf.nkeys].arg = val.addr;
			conf.nkeys++;
		}
	}
	return 0;
}

int config_views(XrmDatabase xrdb, const char *base) {
	/* get modifiers */
	char class[256], *type;
	XrmValue val;
	int i;
	/* loop through bindings */
	conf.view = NULL;
	conf.nviews = 0;
	for (i = 0; i < 100; i++) {
		sprintf(class,"%s.View.%02d.Geometry",base, i);
		if (!XrmGetResource(xrdb, class, class, &type, &val)) continue;
		conf.view = realloc(conf.view, (conf.nviews+1) * sizeof(View));
		sscanf(val.addr, "%dx%d%d%d",
			&conf.view[conf.nviews].w,
			&conf.view[conf.nviews].h,
			&conf.view[conf.nviews].x,
			&conf.view[conf.nviews].y);
		// read val into conf.view[nviews]
		sprintf(class,"%s.View.%02d.Show",base,i);
		if (XrmGetResource(xrdb, class, class, &type, &val))
				conf.view[conf.nviews].show = atoi(val.addr);
		sprintf(class,"%s.View.%02d.Offset",base,i);
		if (XrmGetResource(xrdb, class, class, &type, &val))
			conf.view[conf.nviews].offset = atoi(val.addr);
		conf.nviews++;
	}
	return 0;
}
