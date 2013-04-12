
PROG	= slider
VER		= 0.1a
CFLAGS	= -Os -Wall
PREFIX	?= /usr
FLAGS	= `pkg-config --cflags --libs x11 xrandr poppler-glib cairo` -pthread -lm
SOURCE	= slider.c render.c
HEADER	= slider.h config.h

${PROG}: ${SOURCE} ${HEADER}
	@gcc -o ${PROG} ${SOURCE} ${CFLAGS} ${FLAGS}

debug: ${SOURCE} ${HEADER}
	@gcc -o ${PROG} ${SOURCE} ${CFLAGS} ${FLAGS} -DDEBUG

tarball: clean
	@rm -f ${PROG}-${VER}.tar.gz
	@tar -czf ${PROG}-${VER}.tar.gz *

clean:
	@rm -f ${PROG}

install:
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}

