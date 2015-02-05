/*****************************************************\
  BINDS.C
  By: Jesse McClure (c) 2015
  See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

typedef struct Bind {
	uint mod, button, key, cmd;
	char *arg;
} Bind;

static Bind *bind = NULL;
static int nbind = 0;

int binds_init() {
	return 0;
}

int binds_free() {
	int i;
	for (i = 0; i < nbind; ++i)
		if (bind[i].arg) free(bind[i].arg);
	free(bind);
	bind = NULL;
	nbind = 0;
	return 0;
}

int binds_add(uint mod, uint button, uint key, uint cmd, const char *arg) {
	bind = realloc(bind, (++nbind) * sizeof(Bind));
	Bind *b = &bind[nbind-1];
	b->mod = mod;
	b->button = button;
	b->key = key;
	b->cmd = cmd;
	b->arg = NULL;
	if (arg) b->arg = strdup(arg);
	return 0;
}

int binds_exec(uint mod, uint button, uint key) {
	int i;
	for (i = 0; i < nbind; ++i) {
		if (button && button != bind[i].button) continue;
		if (key && key != bind[i].key) continue;
		if (mod == bind[i].mod && bind[i].cmd) command(bind[i].cmd, bind[i].arg);
	}
	return 0;
}


