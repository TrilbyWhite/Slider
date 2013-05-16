
PROG	= slider
VER		= 2.0a
CFLAGS	= -Os -Wall
PREFIX	?= /usr
FLAGS	= `pkg-config --cflags --libs x11 xrandr poppler-glib cairo` -pthread -lm
SOURCE	= slider.c render.c
HEADER	= slider.h config.h
DEFS	= -DFADE_TRANSITION

${PROG}: ${SOURCE} ${HEADER}
	@gcc -o ${PROG} ${SOURCE} ${CFLAGS} ${FLAGS}

debug: ${SOURCE} ${HEADER}
	@gcc -o ${PROG} ${SOURCE} ${CFLAGS} ${FLAGS} -DDEBUG

tarball: clean
	@rm -f ${PROG}-${VER}.tar.gz
	@tar -czf ${PROG}-${VER}.tar.gz *

clean:
	@rm -f ${PROG}

experimental: ${SOURCE} ${HEADER}
	@gcc -o ${PROG} ${SOURCE} ${CFLAGS} ${FLAGS} ${DEFS}

install:
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@install -Dm666 ${PROG}.1 ${DESTDIR}${PREFIX}/share/man/man1/${PROG}.1

