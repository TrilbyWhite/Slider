/*****************************************************\
* RENDER.C
* By: Jesse McClure (c) 2012-2014
* See slider.c or COPYING for license information
\*****************************************************/

#include "slider.h"

int main(int argc, const char **argv) {
	config_init(argc, argv);
	int ret = xlib_init();
	if (ret == 0) {
		xlib_mainloop();
		xlib_free(0);
	}
	return ret;
}

