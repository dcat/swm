CFLAGS += -std=c99 -g -pedantic -Wall -Os -I/usr/X11R6/include
LDFLAGS	+= -lxcb -L/usr/X11R6/lib
SRC	=  swm.c
OBJ	=  ${SRC:.c=.o}
RM	?= /bin/rm
PREFIX	?= /usr

all: swm

.c.o: ${SRC}
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h

swm: ${OBJ}
	@echo LD $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean: ${OBJ} swm
	${RM} ${OBJ} swm

install: swm
	install -m 755 swm ${DESTDIR}${PREFIX}/bin/swm

uninstall: ${DESTDIR}${PREFIX}/bin/swm
	${RM} ${DESTDIR}${PREFIX}/bin/swm
