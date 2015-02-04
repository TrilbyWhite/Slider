
PROG     =  slider
VER      =  4.0
MODULES  ?= command config cursor randr render slider sorter xlib
MODDEFS  =  $(foreach mod, ${MODULES}, -Dmodule_${mod})
CC       ?= gcc
DEFS     =  -DPROGRAM_NAME=${PROG} -DPROGRAM_VER=${VER} ${MODDEFS}
DEPS     =  x11 cairo poppler-glib xrandr
CFLAGS   += $(shell pkg-config --cflags ${DEPS}) ${DEFS}
LDLIBS   += $(shell pkg-config --libs ${DEPS}) -lm
PREFIX   ?= /usr
HEADERS  =  slider.h
VPATH    =  src

${PROG}: ${MODULES:%=%.o}

%.o: %.c ${HEADERS}

clean:
	@rm -f ${PROG}-${VER}.tar.gz ${MODULES:%=%.o}

distclean: clean
	@rm -f ${PROG}

tarball: distclean
	@tar -czf ${PROG}-${VER}.tar.gz *

.PHONY: clean distclean tarball
