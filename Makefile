
APP1 = slider
LIBS1 = `pkg-config --cflags --libs x11 poppler-glib cairo` -pthread -lm
APP2 = slipper
LIBS2 = `pkg-config --cflags --libs x11 cairo` 

all: ${APP1} ${APP2}

${APP2}: ${APP2}.c
	@gcc -o ${APP2} ${APP2}.c ${LIBS2}
	@strip ${APP2}

${APP1}: ${APP1}.c config.h
	@gcc -o ${APP1} ${APP1}.c ${LIBS1}
	@strip ${APP1}

${APP1}_forms: ${APP1}_forms.c config.h
	@gcc -DSLIDER_FORMFILL -o ${APP1} ${APP1}.c ${LIBS1}
	@strip ${APP1}

clean:
	@rm -f ${APP1} ${APP1}.tar.gz slipper

tarball: clean
	@tar -czf ${APP1}.tar.gz *

install: ${APP1}
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@install -m755 slider ${DESTDIR}${PREFIX}/bin/slider
