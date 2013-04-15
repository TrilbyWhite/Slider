
static const char colors[][9] = {
	[Black]		= "#000000",
	[White]		= "#FFFFFF",
	[ScreenBG]	= "#000000",
	[SlideBG]	= "#FFFFFF",
	[Empty]		= "#182436",
};

/*     CONFIG STRINGS:
  Each of the following sets a config-string for the rectangle in place of
  empty views, the zoom selection rectangle, and the overview highlighter.
  These strings encode line (W)idth, (R)ed, (G)reen, (B)lue color codes,
  and and (A)lpha opacity setting. W is an integer in pixels, RGBA are double
  precision floating point numbers between 0 and 1.

						 W   R    G    B    A
						 ----------------------*/
#define EMPTY_RECT		"10  1.0, 1.0, 1.0  0.5"
#define ZOOM_RECT		"10  0.0, 0.8, 1.0  0.8"
#define OVERVIEW_RECT	"10  1.0, 1.0, 0.0  0.6"
#define ACTION_RECT		"4   0.0, 0.2, 1.0  0.3"
#define ACTION_FONT		"15  0.0, 0.4, 1.0  1.0"

static Key keys[] = {
	{ ControlMask,		XK_q,		quit,		NULL		},
	{ 0,				XK_Tab,		overview,	NULL		},
	{ 0,				XK_Return,	draw,		NULL		},
	{ 0,				XK_space,	move,		"right"		},
	{ 0,				XK_Right,	move,		"right"		},
	{ 0,				XK_Left,	move,		"left"		},
	{ 0,				XK_Up,		move,		"up"		},
	{ 0,				XK_Down,	move,		"down"		},
	{ 0,				XK_b,		mute,		"black"		},
	{ 0,				XK_w,		mute,		"white"		},
	{ 0,				XK_a,		action,		NULL		},
	{ 0,				XK_z,		zoom,		NULL		},
	{ ShiftMask,		XK_z,		zoom,		"lock"		},
	/* the following bindings take config-strings as described above */
	/* draw a rectanle */
	{ 0,				XK_r,		rectangle,	"20  0.0, 0.8, 1.0  0.8" },
	{ ShiftMask,		XK_r,		rectangle,	"20  0.0, 1.0, 0.2  0.8" },
	{ ControlMask,		XK_r,		rectangle,	"20  1.0, 0.0, 0.0  0.8" },
	/* "Polka-dot" cursor/pointers */
	{ 0,				XK_1,		polka,		"40  0.0, 0.2, 0.5  0.7" },
	{ 0,				XK_2,		polka,		"40  1.0, 1.0, 0.1  0.5" },
	{ 0,				XK_3,		polka,		"40  1.0, 0.0, 0.0  0.3" },
	{ 0,				XK_4,		polka,		"10  0.0, 0.2, 0.5  0.7" },
	{ 0,				XK_5,		polka,		"10  1.0, 1.0, 0.1  0.5" },
	{ 0,				XK_6,		polka,		"10  1.0, 0.0, 0.0  0.3" },
	/* Drawing pens */
	{ 0,				XK_F1,		pen,		"4   1.0, 0.0, 0.0  0.9" },
	{ 0,				XK_F2,		pen,		"8   1.0, 0.0, 0.0  0.7" },
	{ 0,				XK_F3,		pen,		"24  1.0, 0.0, 0.0  0.5" },
	{ 0,				XK_F4,		pen,		"4   0.0, 0.2, 0.5  0.9" },
	{ 0,				XK_F5,		pen,		"8   0.0, 0.2, 0.5  0.7" },
	{ 0,				XK_F6,		pen,		"24  0.0, 0.2, 0.5  0.5" },
};
