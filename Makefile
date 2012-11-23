
APP1 = slider
LIBS1 = `pkg-config --cflags --libs x11 poppler-glib cairo` -pthread -lm
APP2 = slipper
LIBS2 = `pkg-config --cflags --libs x11 cairo` -lXrandr
VER = 1.0b

all: ${APP1} ${APP2}

${APP1}: ${APP1}.c config.h ${APP1}.1
	@gcc -o ${APP1} ${APP1}.c ${LIBS1}
	@strip ${APP1}
	@gzip -c ${APP1}.1 > ${APP1}.1.gz

${APP2}: ${APP2}.c ${APP2}.1
	@gcc -o ${APP2} ${APP2}.c ${LIBS2}
	@strip ${APP2}
	@gzip -c ${APP2}.1 > ${APP2}.1.gz

${APP1}_forms: ${APP1}_forms.c config.h
	@gcc -DSLIDER_FORMFILL -o ${APP1} ${APP1}_forms.c ${LIBS1}
	@strip ${APP1}

clean:
	@rm -f ${APP1} ${APP2} ${APP1}-${VER}.tar.gz
	@rm -f ${APP1}.1.gz ${APP2}.1.gz

tarball: clean
	@tar -czf ${APP1}-${VER}.tar.gz *

install: all
	@install -Dm755 ${APP1} ${DESTDIR}${PREFIX}/bin/${APP1}
	@install -m755 ${APP2} ${DESTDIR}${PREFIX}/bin/${APP2}
	@install -Dm666 ${APP1}.1.gz ${DESTDIR}${PREFIX}/share/man/man1/${APP1}.1.gz
	@install -m666 ${APP2}.1.gz ${DESTDIR}${PREFIX}/share/man/man1/${APP2}.1.gz
	@install -Dm666 ${APP2}.dat ${DESTDIR}${PREFIX}/share/${APP1}/${APP2}.dat
