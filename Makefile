
PROG     =  slider
VER      =  3.0a
CC       ?= gcc
DEPS		= x11 cairo freetype2 poppler-glib xinerama
CFLAGS   += $(shell pkg-config --cflags ${DEPS})
LDLIBS   += $(shell pkg-config --libs ${DEPS}) -lm
PREFIX   ?= /usr
MODULES  =  actions config render slider xlib
HEADERS  =  slider.h xlib-actions.h
#MANPAGES =  slider.1
VPATH    =  src:doc

${PROG}: ${MODULES:%=%.o}

%.o: %.c ${HEADERS}

#install: ${PROG}

#${MANPAGES}: slider.%: slider-%.tex
#	@latex2man $< doc/$@

#man: ${MANPAGES}

clean:
	@rm -f ${PROG}-${VER}.tar.gz ${MODULES:%=%.o}

distclean: clean
	@rm -f ${PROG}

dist: distclean
	@tar -czf ${PROG}-${VER}.tar.gz *

.PHONY: clean dist distclean man
