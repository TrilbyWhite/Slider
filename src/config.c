/*****************************************************\
* CONFIG.C
* By: Jesse McClure (c) 2015
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

static int _read_config_file(const char *);

static int *_d = NULL;
static float *_f = NULL;
static char **_s = NULL;
static int _nd = 0, _nf = 0, _ns = 0;
static int _var[LASTVar];
static char _type[LASTVar] = {
	[noteX]       = 'd',
	[noteY]       = 'd',
	[noteFile]    = 's',
	[presX]       = 'd',
	[presY]       = 'd',
	[presW]       = 'd',
	[presH]       = 'd',
	[presFile]    = 's',
	[self]        = 's',
	[videoOut]    = 's',
};
static const char *_name[LASTVar] = {
	[noteX]       = "noteX",
	[noteY]       = "noteY",
	[noteFile]    = "noteFile",
	[presX]       = "presX",
	[presY]       = "presY",
	[presW]       = "presW",
	[presH]       = "presH",
	[presFile]    = "presFile",
	[videoOut]    = "display",
	[self]        = "self",
};

int get_d(int var) {
	if (_type[var] != 'd') return 0;
	if (_var[var] < 0 || _var[var] >= _nd) return 0;
	return _d[_var[var]];
}

float get_f(int var) {
	if (_type[var] != 'f') return 0.0;
	if (_var[var] < 0 || _var[var] >= _nf) return 0.0;
	return _f[_var[var]];
}

const char *get_s(int var) {
	if (_type[var] != 's') return NULL;
	if (_var[var] < 0 || _var[var] >= _ns) return NULL;
	return _s[_var[var]];
}

int config_init(int argc, const char **argv) {
	int i;
	set(self, argv[0]);
	int file_count = 0;
	char *conf = NULL;
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == '\0')
			break;
		else if (argv[i][0] == '-' && argv[i][1] == 'c' && i + 1 < argc)
			_read_config_file(argv[++i]);
		else if (++file_count == 1)
			set(presFile, argv[i]);
		else if (file_count == 2)
			set(noteFile, argv[i]);
		else
			fprintf(stderr, "unused parameter \"%s\"\n", argv[i]);
	}
	for (i; i < argc; ++i)
		_parse_config_string(argv[i]);
	return 0;
}

int config_free() {
	int i;
	for (i = 0; i < _ns; ++i) free(_s[i]);
	free(_s);
	free(_f);
	free(_d);
	return 0;
}

void config_set(int var, config_union u) {
	switch (_type[var]) {
		case 'd':
			_d = realloc(_d, (_nd+1) * sizeof(int));
			_d[_nd] = u.d;
			_var[var] = _nd;
			++_nd;
			break;
		case 'f':
			_f = realloc(_f, (_nf+1) * sizeof(float));
			_f[_nf] = u.f;
			_var[var] = _nf;
			++_nf;
			break;
		case 's':
			if (!u.s) return;
			_s = realloc(_s, (_ns+1) * sizeof(char *));
			_s[_ns] = strdup(u.s);
			_var[var] = _ns;
			++_ns;
			break;
	}
}

int _parse_config_string(const char *arg) {
	char *ptr, *key, *value, *str = strdup(arg);
	key = strtok_r(str, " =\t", &ptr);
	value = strtok_r(NULL, " =\t", &ptr);
	int i;
	for (i = 0; i < LASTVar; ++i)
		if (strncasecmp(_name[i], key, strlen(key)) == 0) break;
	if (i < LASTVar) switch (_type[i]) {
		case 'd': set(i, atoi(value)); break;
		case 'f': set(i, (float) atof(value)); break;
		case 's': set(i, (const char *) value); break;
	}
	free(str);
	return 0;
}

int _read_config_file(const char *arg) {
	FILE *f = fopen(arg, "r");
	if (!f) return 1;
	char line[256];
	while (fgets(line, 256, f))
		_parse_config_string(line);
	fclose(f);
	return 0;
}
