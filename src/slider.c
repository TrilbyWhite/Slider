/**********************************************************************\
* SLIDER - PDF presentation tool
*
* Author: Jesse McClure, copyright 2012-2014
* License: GPLv3
*
*    This program is free software: you can redistribute it and/or
*    modify it under the terms of the GNU General Public License as
*    published by the Free Software Foundation, either version 3 of the
*    License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see
*    <http://www.gnu.org/licenses/>.
*
\**********************************************************************/

#include "slider.h"

#define STRING(s)		STRINGIFY(s)
#define STRINGIFY(s)	#s

void die(const char *msg, ...) {
	va_list arg;
	va_start(arg, msg);
	vfprintf(stderr, msg, arg);
	va_end(arg);
	exit(1);
}

static int help() {
	printf("Help info coming soon ...\n");
	exit(0);
}

static int version() {
	printf(STRING(PROGRAM_NAME) " v" STRING(PROGRAM_VER)
", Copyright © 2013-2014 Jesse McClure <www.mccluresk9.com>\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see	<http://www.gnu.org/licenses/>.\n");
	exit(0);
}

int main(int argc, const char **argv) {
	int i, j;
	show = NULL;
	const char *mode = NULL;
	const char *dbname = NULL;
	const char *pdfname = NULL, *notename = NULL;
	for (i = 1; i < argc; i++) {
		if (argv[i][j] == '-') {
			j = 1;
			if (argv[i][j] == '-') j++;
			if (argv[i][j] == 'F' && (++i) < argc) dbname = argv[i];
			else if (argv[i][j] == 'c' && (++i) < argc) mode = argv[i];
			else if (argv[i][j] == 'h') help();
			else if (argv[i][j] == 'v') version();
			else fprintf(stderr, "Unrecognized argument \"%s\"\n", argv[i]);
		}
		else if (!pdfname)
			pdfname = argv[i];
		else if (!notename)
			notename = argv[i];
		else
			fprintf(stderr, "Unused argument \"%s\"\n", argv[i]);
	}
	config_init(mode, dbname);
	if (!pdfname) die("No presentation found\n");
	show = render_init(pdfname, notename);
	if (!show) die("No presentation found\n");
	xlib_main_loop();
	render_free(show);
	config_free();
	return 0;
}

