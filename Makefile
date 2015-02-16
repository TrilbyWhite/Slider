
PROG     =  slider
VER      =  4.0a
CC       ?= gcc
PREFIX   ?= /usr
VPATH    =  src

MODULES  ?= command config cursor links randr render slider sorter xlib
CONFFILE =  ${PREFIX}/share/${PROG}/config
#CONFFILE =  /etc/xdg/${PROG}/config
DEFS     =  -DPROGRAM_NAME=${PROG} -DPROGRAM_VER=${VER} -DDEFAULT_CONFIG=${CONFFILE}
DEFS     += $(foreach mod, ${MODULES}, -Dmodule_${mod})
DEPS     =  x11 cairo poppler-glib xrandr
CFLAGS   ?= -march=native -O2 -pipe -fstack-protector-strong --param=ssp-buffer-size=4 -Wall -Werror -g
CFLAGS   += $(shell pkg-config --cflags ${DEPS}) ${DEFS}
LDLIBS   += $(shell pkg-config --libs ${DEPS}) -lm
HEADERS  =  slider.h

${PROG}: ${MODULES:%=%.o}

%.o: %.c ${HEADERS}

install: ${PROG}
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@install -Dm644 share/config ${DESTDIR}${CONFFILE}
	#@install -Dm644 ... manual page(s)

clean:
	@rm -f ${PROG}-${VER}.tar.gz ${MODULES:%=%.o}

distclean: clean
	@rm -f ${PROG}

tarball: distclean
	@tar -czf ${PROG}-${VER}.tar.gz *

.PHONY: clean distclean tarball

