
static const char colors[][9] = {
	[Black]		= "#000000",
	[White]		= "#FFFFFF",
	[ScreenBG]	= "#000000",
	[SlideBG]	= "#FFFFFF",
	[Empty]		= "#182436",
	[Highlight]	= "#FF8800",
	[Warn]		= "#FF2222",
};

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
	{ 0,				XK_z,		zoom,		NULL		},
	{ ShiftMask,		XK_z,		zoom,		"lock"		},
	{ 0,				XK_r,		rectangle,	NULL		},
	{ ShiftMask,		XK_r,		rectangle,	"lock"		},
	/* pen parameters in order: "w r,g,b a"
		w: integer width, r,g,b,a: double (0-1) values for color */
	{ 0,				XK_1,		polka,		"40 0,0.25,0.5 0.5"	},
	{ 0,				XK_2,		polka,		"40 1,0.75,0.1 0.4"	},
	{ 0,				XK_3,		polka,		"40 1,0,0 0.3"		},
	{ 0,				XK_4,		polka,		"10 0,0.25,0.5 0.5"	},
	{ 0,				XK_5,		polka,		"10 1,0.75,0.1 0.4"	},
	{ 0,				XK_6,		polka,		"10 1,0,0 0.3"		},
	{ 0,				XK_F1,		pen,		"4 1,0,0 0.9"	},
	{ 0,				XK_F2,		pen,		"8 1,0,0 0.7"	},
	{ 0,				XK_F3,		pen,		"24 1,0,0 0.5"	},
	{ 0,				XK_F4,		pen,		"4 0,0.25,0.5 0.9"	},
	{ 0,				XK_F5,		pen,		"8 0,0.25,0.5 0.7"	},
	{ 0,				XK_F6,		pen,		"24 0,0.25,0.5 0.5"	},
};
