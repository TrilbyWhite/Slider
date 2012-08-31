
/* color for dotted line placeholders in slide overview prior to slide loading.
   color for highlighting ring in overview.	*/
static const char colors[][9] = {	"#202040",	"#FFDD0E" };

static Key keys[] = {
	/* modifier		key				function		argument */
	{ 0,			XK_b,			mute,			"black"		},
	{ 0,			XK_w,			mute,			"white"		},
	{ 0,			XK_r,			draw,			NULL		},
	{ 0,			XK_f,			fullscreen,		NULL		},
#ifdef SLIDER_FORMFILL
	{ 0,			XK_e,			fill_field,		NULL		},
	{ Mod1Mask,		XK_s,			fill_field,		"save"		}, /* DO NOT USE YET - this WILL destroy your pdf */
#endif
	{ 0,			XK_Return,		draw,			NULL		},
	{ 0,			XK_Tab,			overview,		NULL		},
	{ 0,			XK_Down,		move,			"down"		},
	{ 0,			XK_Up,			move,			"up"		},
	{ 0,			XK_Left,		move,			"left"		},
	{ 0,			XK_Right,		move,			"right"		},
	{ Mod1Mask,		XK_q,			quit,			NULL		},
};
