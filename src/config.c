/*****************************************************\
  CONFIG.C
  By: Jesse McClure (c) 2015
  See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

typedef struct Bind {
	uint mod, button, key, cmd;
	char *arg;
} Bind;

typedef struct Variable {
	int index;
	char type;
	const char *name;
} Variable;

static bool _read_config_file(const char *);
static int _parse_config_string(const char *);

static int *_d = NULL;
static float *_f = NULL;
static char **_s = NULL;
static int _nd = 0, _nf = 0, _ns = 0;
static Bind *bind = NULL;
static int nbind = 0;
static Variable _var[LASTVar] = {
	[linkAction]  = { 0, 's', "ActionHandler" },
	[linkAudio]   = { 0, 's', "AudioHandler" },
	[linkMovie]   = { 0, 's', "MovieHandler" },
	[linkUri]     = { 0, 's', "LinkHandler" },
	[noteX]       = { 0, 'd', "noteX" },
	[noteY]       = { 0, 'd', "noteY" },
	[noteFile]    = { 0, 's', "noteFile" },
	[presX]       = { 0, 'd', "presX" },
	[presY]       = { 0, 'd', "presY" },
	[presW]       = { 0, 'd', "presW" },
	[presH]       = { 0, 'd', "presH" },
	[presFile]    = { 0, 's', "presFile" },
	[self]        = { 0, 's', "self" },
	[videoOut]    = { 0, 's', "Display" },
};


int get_d(int var) {
	if (_var[var].type != 'd') return 0;
	if (_var[var].index < 0 || _var[var].index >= _nd) return 0;
	return _d[_var[var].index];
}

float get_f(int var) {
	if (_var[var].type != 'f') return 0.0;
	if (_var[var].index < 0 || _var[var].index >= _nf) return 0.0;
	return _f[_var[var].index];
}

const char *get_s(int var) {
	if (_var[var].type != 's') return NULL;
	if (_var[var].index < 0 || _var[var].index >= _ns) return NULL;
	return _s[_var[var].index];
}

int config_init(int argc, const char **argv) {
	dpy = XOpenDisplay(0x0);
	if (!dpy) return 1;
	int i, file_count = 0;
	bool conf_file = false;
	for (i = 0; i < LASTVar; ++i) _var[i].index = -1;
	set(self, argv[0]);
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == '\0')
			break;
		else if (argv[i][0] == '-' && argv[i][1] == 'F' && i + 1 < argc)
			conf_file |= _read_config_file(argv[++i]);
		else if (++file_count == 1)
			set(presFile, argv[i]);
		else if (file_count == 2)
			set(noteFile, argv[i]);
		else
			fprintf(stderr, "unused parameter \"%s\"\n", argv[i]);
	}
	char *dir, *old_dir = getenv("PWD");
	if (!conf_file && (dir=getenv("XDG_CONFIG_HOME"))) {
		chdir(dir); chdir(STRING(PROGRAM_NAME));
		conf_file |= _read_config_file("config");
	}
	if (!conf_file && (dir=getenv("HOME"))) {
		chdir(dir); chdir(".config"); chdir(STRING(PROGRAM_NAME));
		conf_file |= _read_config_file("config");
	}
	if (!conf_file)
		conf_file |= _read_config_file(STRING(DEFAULT_CONFIG));
	chdir(old_dir);
	for (; i < argc; ++i)
		_parse_config_string(argv[i]);
	return 0;
}

int config_free() {
	int i;
	/* free bindings */
	for (i = 0; i < nbind; ++i)
		if (bind[i].arg) free(bind[i].arg);
	free(bind);
	bind = NULL;
	nbind = 0;
	/* free configurations */
	for (i = 0; i < _ns; ++i) if (_s[i]) free(_s[i]);
	free(_s); _s = NULL; _ns = 0;
	free(_f); _f = NULL; _nf = 0;
	free(_d); _d = NULL; _nd = 0;
	XCloseDisplay(dpy);
	return 0;
}

void config_set(int var, config_union u) {
	switch (_var[var].type) {
		case 'd':
			_d = realloc(_d, (_nd+1) * sizeof(int));
			_d[_nd] = u.d;
			_var[var].index = _nd;
			++_nd;
			break;
		case 'f':
			_f = realloc(_f, (_nf+1) * sizeof(float));
			_f[_nf] = u.f;
			_var[var].index = _nf;
			++_nf;
			break;
		case 's':
			if (!u.s) return;
			_s = realloc(_s, (_ns+1) * sizeof(char *));
			_s[_ns] = strdup(u.s);
			_var[var].index = _ns;
			++_ns;
			break;
	}
}

static unsigned int _parse_modifier(const char *mod) {
	if (strncasecmp(mod, "control", 7) == 0) return ControlMask;
	else if (strncasecmp(mod, "cntrl", 5) == 0) return ControlMask;
	else if (strncasecmp(mod, "ctrl", 4) == 0) return ControlMask;
	else if (strncasecmp(mod, "shift", 5) == 0) return ShiftMask;
	else if (strncasecmp(mod, "alt", 3) == 0) return Mod1Mask;
	else if (strncasecmp(mod, "mod1", 4) == 0) return Mod1Mask;
	else if (strncasecmp(mod, "mod2", 4) == 0) return Mod2Mask;
	else if (strncasecmp(mod, "mod3", 4) == 0) return Mod3Mask;
	else if (strncasecmp(mod, "mod4", 4) == 0) return Mod4Mask;
	else if (strncasecmp(mod, "mod5", 4) == 0) return Mod5Mask;
	else if (strncasecmp(mod, "super", 5) == 0) return Mod4Mask;
	else if (strncasecmp(mod, "win", 3) == 0) return Mod4Mask;
	return 0;
}

int _binding_add(uint mod, uint button, uint key, uint cmd, const char *arg) {
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

int _parse_binding(const char *arg) {
	unsigned int mod = 0, mod_add, button = 0, key = 0, cmd = 0;
	char *ptr, *type, *value, *str = strdup(arg);
	type = strtok_r(str, " =\t", &ptr);
	value = strtok_r(NULL, " =\t", &ptr);
	while (value && (mod_add = _parse_modifier(value))) {
		mod |= mod_add;
		value = strtok_r(NULL, " =\t", &ptr);
	}
	if (strncasecmp(type, "key", 3) == 0) {
		KeySym sym = None;
		if (value && ((sym = XStringToKeysym(value)) != NoSymbol)) {
			key = XKeysymToKeycode(dpy, sym);
			value = strtok_r(NULL, " =\t", &ptr);
		}
	}
	else { /* button */
		if (value && (button = atoi(value)) )
			value = strtok_r(NULL, " =\t", &ptr);
	}
	if (value && (cmd = command_str_to_num(value)))
		value = strtok_r(NULL, " =\t", &ptr);
	if (button || key)
		_binding_add(mod, button, key, cmd, value);
	free(str);
	return 0;
}

int _parse_config_string(const char *arg) {
	char *ptr, *key, *value, *str = strdup(arg);
	int i;
	key = strtok_r(str, " =\t", &ptr);
	if (strncasecmp("key", key, 3) == 0)
		_parse_binding(arg);
	else if (strncasecmp("button", key, 6) == 0)
		_parse_binding(arg);
	else {
		value = ptr + strspn(ptr, " =\t");
		for (i = 0; i < LASTVar; ++i)
			if (strncasecmp(_var[i].name, key, strlen(key)) == 0)
				break;
		if (i < LASTVar) switch (_var[i].type) {
			case 'd': set(i, atoi(value)); break;
			case 'f': set(i, (float) atof(value)); break;
			case 's': set(i, (const char *) value); break;
		}
	}
	free(str);
	return 0;
}

bool _read_config_file(const char *arg) {
	FILE *f = fopen(arg, "r");
	if (!f) return false;
	char line[256];
	while (fgets(line, 256, f)) {
		if (line[0] == '#' || line[0] == '\n') continue;
		line[strlen(line)-1] = '\0';
		_parse_config_string(line);
	}
	fclose(f);
	return true;
}

int config_bind_exec(uint mod, uint button, uint key) {
	int i;
	for (i = 0; i < nbind; ++i) {
		if (button && button != bind[i].button) continue;
		if (key && key != bind[i].key) continue;
		if (mod == bind[i].mod && bind[i].cmd) command(bind[i].cmd, bind[i].arg);
	}
	return 0;
}


