
const char colors[][9] = {
/* soon all colors will be replaced by config strings */
	[Black]		= "#000000",
	[White]		= "#FFFFFF",
	[ScreenBG]	= "#000000",
	[SlideBG]	= "#FFFFFF",
	[Empty]		= "#182436",
};

#define CURSOR_STRING_MAX	12

/* PLAY_AUD is backgrounded to allow the slideshow to continue
   while the audio is playing.  You could also background the
   other options, however you will then *NOT* be able to control
   or interact with the external program.  Slider "ungrabs" the
   keyboard only for the duration of the launched program - if
   the launched program is backgrounded, it's duration is only
   the time it takes to start. */
#define SHOW_URI		"luakit %s >/dev/null 2>&1"
#define SHOW_MOV		"mplayer -fs %s >/dev/null 2>&1"
#define PLAY_AUD		"mplayer %s >/dev/null 2>&1 &"

/*     CONFIG STRINGS:
  Each of the following sets a config-string for the rectangle in place of
  empty views, the zoom selection rectangle, and the overview highlighter.
  These strings encode line (W)idth, (R)ed, (G)reen, (B)lue color codes,
  and and (A)lpha opacity setting. W is an integer in pixels, RGBA are double
  precision floating point numbers between 0 and 1.

				 W   R    G    B    A
				-----------------------*/
#define EMPTY_RECT		"10  1.0, 1.0, 1.0  0.5"
#define ZOOM_RECT		"10  0.0, 0.8, 1.0  0.8"
#define OVERVIEW_RECT	        "10  1.0, 1.0, 0.0  0.6"
#define ACTION_RECT		"1   0.0, 0.2, 1.0  0.3"
#define ACTION_FONT		"8   0.0, 0.4, 1.0  1.0"

static Key keys[] = {
	{ ControlMask,		XK_q,		quit,		NULL		},
	{ 0,			XK_Tab,		overview,	NULL		},
	{ 0,			XK_Return,	draw,		NULL		},
	{ ShiftMask,		XK_Return,	draw,		"render"	},
	{ 0,			XK_space,	move,		"right"		},
	{ 0,			XK_Right,	move,		"right"		},
	{ 0,			XK_Left,	move,		"left"		},
	{ 0,			XK_Up,		move,		"up"		},
	{ 0,			XK_Down,	move,		"down"		},
	{ 0,			XK_b,		mute,		"black"		},
	{ 0,			XK_w,		mute,		"white"		},

/* ------------ CONDITIONALLY COMPILED FEATURES: ------------ */
#ifdef ACTION_LINKS
	{ 0,			XK_a,		action,		"mouse"		},
	{ ShiftMask,		XK_a,		action,		"keys"		},
#endif /* ACTION_LINKS */

#ifdef FORM_FILL
	{ 0,			XK_f,		fillfield,	"mouse"		},
	{ ShiftMask,		XK_f,		fillfield,	"keys"		},
#endif /* FORM_FILL */

#ifdef ZOOMING
	{ 0,			XK_z,		zoom,		NULL		},
	{ ShiftMask,		XK_z,		zoom,		"lock"		},
#endif /*ZOOMING */

#ifdef DRAWING
	/* the following bindings take config-strings as described above
	   the perm_pen or perm_rect makes the drawing persistent for the
	   duration of the presentation session.
	   Such persistent drawings can be "erased" with draw("render")  */
	/* draw a rectanle */
	{ 0,			XK_r,		rectangle,	"20  0.0, 0.8, 1.0  0.8" },
	{ ShiftMask,		XK_r,		rectangle,	"20  0.0, 1.0, 0.2  0.8" },
	{ ControlMask,		XK_r,		rectangle,	"20  1.0, 0.0, 0.0  0.8" },
	{ Mod1Mask,		XK_r,		perm_rect,	"20  0.0, 0.8, 1.0  0.8" },
	/* "Polka-dot" cursor/pointers */
	{ 0,			XK_1,		polka,		"40  0.0, 0.2, 0.5  0.7" },
	{ 0,			XK_2,		polka,		"40  1.0, 1.0, 0.1  0.5" },
	{ 0,			XK_3,		polka,		"40  1.0, 0.0, 0.0  0.3" },
	{ 0,			XK_4,		polka,		"10  0.0, 0.2, 0.5  0.7" },
	{ 0,			XK_5,		polka,		"10  1.0, 1.0, 0.1  0.5" },
	{ 0,			XK_6,		polka,		"10  1.0, 0.0, 0.0  0.3" },
	/* String cursor/pointers: Put any text string at the end of the config string */
	/*   Strings for cursors must be less that CURSOR_STRING_MAX bytes.
	     Note that unicode characters may count for more than one byte each. */
	{ ShiftMask,		XK_1,		string,		"60   0.0, 0.3, 1.0  0.7 ↖" },
	{ ShiftMask,		XK_2,		string,		"80   0.0, 0.3, 1.0  0.7 ↖" },
	{ ShiftMask,		XK_3,		string,		"100  0.0, 0.3, 1.0  0.7 ↖" },
	{ ShiftMask,		XK_4,		string,		"60   1.0, 0.0, 0.0  0.7 ↗" },
	{ ShiftMask,		XK_5,		string,		"80   1.0, 0.0, 0.0  0.7 ↗" },
	{ ShiftMask,		XK_6,		string,		"100  1.0, 0.0, 0.0  0.7 ↗" },
	{ ShiftMask,		XK_7,		string,		"80   0.0, 1.0, 0.0  0.8 SLIDER" },
	/* Drawing pens */
	{ 0,			XK_F1,		pen,		"4   1.0, 0.0, 0.0  0.9" },
	{ 0,			XK_F2,		pen,		"8   1.0, 0.0, 0.0  0.7" },
	{ 0,			XK_F3,		pen,		"24  1.0, 0.0, 0.0  0.5" },
	{ 0,			XK_F4,		pen,		"4   0.0, 0.2, 0.5  0.9" },
	{ 0,			XK_F5,		pen,		"8   0.0, 0.2, 0.5  0.7" },
	{ 0,			XK_F6,		pen,		"24  0.0, 0.2, 0.5  0.5" },
	{ ShiftMask,		XK_F1,		perm_pen,	"4   1.0, 0.0, 0.0  0.9" },
	{ ShiftMask,		XK_F2,		perm_pen,	"8   1.0, 0.0, 0.0  0.7" },
	{ ShiftMask,		XK_F3,		perm_pen,	"24  1.0, 0.0, 0.0  0.5" },
	{ ShiftMask,		XK_F4,		perm_pen,	"4   0.0, 0.2, 0.5  0.9" },
	{ ShiftMask,		XK_F5,		perm_pen,	"8   0.0, 0.2, 0.5  0.7" },
	{ ShiftMask,		XK_F6,		perm_pen,	"24  0.0, 0.2, 0.5  0.5" },
#endif /* DRAWING */
/* --------------------- END FEATURES: ---------------------- */

}; /* end keys */
