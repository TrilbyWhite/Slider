/*****************************************************\
* SLIDER-REMOTE.C
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-ft.h>
#include <poppler.h>

#define TMP_NAME  "/tmp/slider-remote"
#define MAX_LINE	256

#define TIME_BAR	0x01
#define SLIDE_BAR	0x02
#define TIME_NUM	0x04
#define SLIDE_NUM	0x08
#define X11_FLAGS	0x0F
#define AUTO_PLAY	0x10

static int create_win();
static int command_line(int, char **);
static int help();
static int slider(char **);
static int get_wshow();
static int run_commands();

static const char *_fmt = "%H:%M";
static const char *fmt;
static const char *_dur = "45:00";
static const char *dur;
static const char **command = NULL;

static Display *dpy;
static Window root, wshow = None, win;
static int opts = 0, scr, sec = 30;
static Atom COM_ATOM;
static FILE *fifo = NULL;

/*
slider-remote [flags,commands...] [-- options passed to slider]
*/

int main(int argc, char **argv) {
	command_line(argc, argv);
	if (!opts && !fifo) return 0;
	if (!(dpy=XOpenDisplay(0x0))) {
		fprintf(stderr,"Unable to open display\n");
		return 1;
	}
	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);
	COM_ATOM = XInternAtom(dpy, "Command", False);
	if ((opts & X11_FLAGS)) create_win();
	get_wshow();
	run_commands();
	// TODO Xlib main loop (check AUTO_PLAY)
	XSetInputFocus(dpy, wshow, RevertToPointerRoot, CurrentTime);
	XCloseDisplay(dpy);
	return 0;
}

int create_win() {
	// TODO
}

int command_line(int argc, char **argv) {
	fifo = fopen(TMP_NAME, "w");
	fmt = _fmt; dur = _dur;
	char *c;
	int i, inc = 0, n = 0;
	FILE *script = NULL;
	char line[MAX_LINE];
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == '-' && argv[i][2] == '\0') {
				slider(&argv[i]);
				break;
			}
			for (c = &argv[i][1]; *c != '\0'; c++) {
				switch (*c) {
					case 'h': case 'v': help(); break;
					case 't': opts |= TIME_BAR; break;
					case 's': opts |= SLIDE_BAR; break;
					case 'b': opts |= TIME_BAR | SLIDE_BAR; break;
					case 'T': opts |= TIME_NUM; break;
					case 'S': opts |= SLIDE_NUM; break;
					case 'n': opts |= TIME_NUM | SLIDE_NUM; break;
					case 'a': opts |= AUTO_PLAY; break;
					case 'd':
						if (i + inc < argc) sec = atoi(argv[i + (++inc)]);
						else fprintf(stderr, "missing required parameter\n");
						break;
					case 'f':
						if (i + inc < argc) fmt = argv[i + (++inc)];
						else fprintf(stderr, "missing required parameter\n");
						break;
					case 'l':
						if (i + inc < argc) dur = argv[i + (++inc)];
						else fprintf(stderr, "missing required parameter\n");
						break;
					case 'F':
						if (i + inc < argc)
							script = fopen(argv[i + (++inc)], "r");
						else
							fprintf(stderr, "missing required parameter\n");
						if (script)
							while (fgets(line, MAX_LINE, script) != NULL) {
								fprintf(fifo, line);
								n++;
							}
						break;

				}
			}
			i += inc;
			inc = 0;
		}
		else {
			fprintf(fifo, "%s\n", argv[i]);
			n++;
		}
	}
	fclose(fifo);
	if (!n) fifo = NULL;
	return 0;
}

int get_wshow() {
	Window w, *ws;
	int i, n, loop;
	char *name;
	for (loop = 0; !wshow && loop < 20; loop++) {
		XFlush(dpy);
		XQueryTree(dpy, root, &w, &w, &ws, &n);
		for (i = 0; i < n; i++) {
			w = ws[i];
			XFetchName(dpy, w, &name);
			if (name) {
				if (strncmp(name,"Slider",6)==0) wshow = w;
				XFree(name);
			}
		}
		if (ws) XFree(ws);
		usleep(50000);
	}
	if (!wshow)
		fprintf(stderr,"Cannot find to slider presentation\n");
	return 0;
}

int help() {
	fprintf(stdout, "SLIDER-REMOTE\n");
}

int run_commands() {
	XTextProperty tprop;
	char *com = calloc(MAX_LINE, sizeof(char));
	int sleep_sec, sleep_check = 0;
	fifo = fopen(TMP_NAME, "r");
	while (fgets(com, MAX_LINE, fifo) != NULL) {
		com[strlen(com)-1] = '\0'; /* remove newline */
		if (sscanf(com, "sleep %d", &sleep_sec) == 1) {
			sleep(sleep_sec);
			sleep_check = 0;
		}
		else {
			if (sleep_check) sleep(1);
			XStringListToTextProperty(&com, 1, &tprop);
			XSetTextProperty(dpy, wshow, &tprop, COM_ATOM);
			XFlush(dpy);
			XFree(tprop.value);
			sleep_check = 1;
		}
	}
	free(com);
	fclose(fifo);
}

int slider(char **args) {
	if (!args || !args[0]) return -1;
	if (!fork()) {
		//args[0] = "/usr/bin/slider";
		args[0] = "/home/jmcclure/code/slider/slider";
		setsid();
		execvp(args[0], (char * const *) args);
	}
}
