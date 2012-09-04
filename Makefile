
APP = slider
LIBS = `pkg-config --cflags --libs x11 poppler-glib cairo` -pthread -lm

all: slipper slider

slipper: slipper.c
	@gcc -o slipper slipper.c -lX11
	@strip slipper

${APP}: ${APP}.c config.h
	@gcc -o ${APP} ${APP}.c ${LIBS}
	@strip ${APP}

${APP}_forms: ${APP}_forms.c config.h
	@gcc -DSLIDER_FORMFILL -o ${APP} ${APP}.c ${LIBS}
	@strip ${APP}

clean:
	@rm -f ${APP} ${APP}.tar.gz slipper

tarball: clean
	@tar -czf ${APP}.tar.gz *

install: ${APP}
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@install -m755 slider ${DESTDIR}${PREFIX}/bin/slider
