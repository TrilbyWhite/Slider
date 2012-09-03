
/* color for dotted line placeholders in slide overview prior to slide loading.
   color for highlighting ring in overview.	*/
static const char colors[][9] = {	"#202040",	"#FFDD0E" };

static Key keys[] = {
	/* modifier		key				function		argument */
	{ Mod1Mask,		XK_q,			quit,			NULL			},
	{ 0,			XK_Tab,			overview,		NULL			},
	{ 0,			XK_f,			fullscreen,		NULL			},
	{ 0,			XK_b,			mute,			"black"			},
	{ 0,			XK_w,			mute,			"white"			},
	{ 0,			XK_r,			draw,			NULL			},
	{ 0,			XK_Return,		draw,			NULL			},
	/* keyboard movement controls */
	{ 0,			XK_space,		move,			"right"			},
	{ 0,			XK_Down,		move,			"down"			},
	{ 0,			XK_Up,			move,			"up"			},
	{ 0,			XK_Left,		move,			"left"			},
	{ 0,			XK_Right,		move,			"right"			},
	{ 0,			XK_j,			move,			"down"			},
	{ 0,			XK_k,			move,			"up"			},
	{ 0,			XK_l,			move,			"left"			},
	{ 0,			XK_h,			move,			"right"			},
	/* pen drawing: select a pen, then draw with mouse.  Any key exits	*/
	/* (W)idth must be two numerical characters, COLOR is RGB			*/
	/* These are just examples of the variety, they may not be useful
		as they are - feel free to modify width and colors.				*/
												/*	 W  COLOR	*/
	{ 0,			XK_1,			pen,			"02 #FF0000"	},
	{ 0,			XK_2,			pen,			"08 #FF0000"	},
	{ 0,			XK_3,			pen,			"32 #FF0000"	},
	{ 0,			XK_4,			pen,			"02 #0000FF"	},
	{ 0,			XK_5,			pen,			"08 #0000FF"	},
	{ 0,			XK_6,			pen,			"32 #0000FF"	},
	{ 0,			XK_7,			pen,			"02 #FFFF00"	},
	{ 0,			XK_8,			pen,			"08 #FFFF00"	},
	{ 0,			XK_9,			pen,			"32 #FFFF00"	},
};
