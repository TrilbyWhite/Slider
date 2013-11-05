
PROG     =  slider
VER      =  2.0a
CC       ?= gcc
CFLAGS   += `pkg-config --cflags x11 xrandr poppler-glib cairo`
LDFLAGS  += `pkg-config --libs x11 xrandr poppler-glib cairo` -pthread -lm
PREFIX   ?= /usr
SOURCE   = 	slider.c render.c
HEADER   = 	slider.h
OPTS     =  -DACTION_LINKS -DDRAWING -DZOOMING
EX_OPTS  =	${OPTS} -DFORM_FILL

##################################################################

${PROG}: ${SOURCE} ${HEADER}
	@${CC} -o ${PROG} ${SOURCE} ${CFLAGS} ${LDFLAGS} ${OPTS}

debug: ${SOURCE} ${HEADER}
	@${CC} -g -o ${PROG} ${SOURCE} ${CFLAGS} ${LDFLAGS} ${OPTS} -DDEBUG

minimal: ${SOURCE} ${HEADER}
	@${CC} -o ${PROG} ${SOURCE} ${CFLAGS} ${LDFLAGS}

forms: ${SOURCE} ${HEADER}
	@${CC} -o ${PROG} ${SOURCE} ${CFLAGS} ${LDFLAGS} ${OPTS} -DFORM_FILL

##################################################################

tarball: clean
	@rm -f ${PROG}-${VER}.tar.gz
	@tar -czf ${PROG}-${VER}.tar.gz *

clean:
	@rm -f ${PROG}

install:
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@install -Dm644 ${PROG}.1 ${DESTDIR}${PREFIX}/share/man/man1/${PROG}.1
	@install -Dm644 ${PROG}rc ${DESTDIR}${PREFIX}/share/slider/config

.PHONY: debug minimal experimental forms clean tarball install
