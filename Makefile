
APP = slider
LIBS = `pkg-config --cflags --libs x11 poppler-glib cairo` -pthread -lm

${APP}: ${APP}.c config.h
	@gcc -o ${APP} ${APP}.c ${LIBS}
	@strip ${APP}

clean:
	@rm -f ${APP}
	@rm -f ${APP}.tar.gz

tarball: clean
	@tar -czf ${APP}.tar.gz *
