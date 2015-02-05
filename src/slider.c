/**********************************************************************\
 SLIDER - PDF Presenter

 Author: Jesse McClure, copyright 2012-2015
 License: GPL3

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.

\**********************************************************************/

#include "slider.h"

int main(int argc, const char **argv) {
	if (config_init(argc, argv))
		return 1;
	if (xlib_init()) {
		config_free();
		return 2;
	}
	xlib_mainloop();
	xlib_free(0);
	config_free();
	return 0;
}

