CFLAGS	+= -std=c99 -g # -Os
LDFLAGS	+= -lxcb -lxcb-keysyms # -static
SRC	=  swm.c
OBJ	=  ${SRC:.c=.o}
RM	?= /bin/rm

all: swm

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h

swm: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	${RM} swm ${OBJ}

install:
	install -m 755 swm ${PREFIX}/bin/swm

uninstall:
	rm ${PREFIX}/bin/swm
