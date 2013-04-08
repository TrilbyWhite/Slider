
static const char colors[][9] = {
	[Black]		= "#000000",
	[White]		= "#FFFFFF",
	[ScreenBG]	= "#000000",
	[SlideBG]	= "#FFFFFF",
	[Empty]		= "#182436",
	[Highlight]	= "#FF8800",
};

static Key keys[] = {
	{ ControlMask,		XK_q,		quit,		NULL		},
	{ 0,				XK_Tab,		overview,	NULL		},
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
};
