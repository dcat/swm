BIN = swm
SRC = swm.c
OBJ ?= ${SRC:.c=.o}
LDFLAGS += -lxcb

include config.mk

.POSIX:
all: ${BIN}

.c.o: ${SRC}
	@echo "${CC} -c $< -o $@ ${CFLAGS}"
	@${CC} -c $< -o $@ ${CFLAGS}

${OBJ}: config.h

${BIN}: ${OBJ}
	@echo "${LD} $^ -o $@ ${LDFLAGS}"
	@${LD} -o $@ ${OBJ} ${LDFLAGS}

install: ${BIN}
	mkdir -p ${DESTDIR}${PREFIX}/bin
	install ${BIN} ${DESTDIR}${PREFIX}/bin/${BIN}
	chmod 755 ${DESTDIR}${PREFIX}/bin/${BIN}

uninstall: ${DESTDIR}${PREFIX}/bin/${BIN}
	rm -f ${DESTDIR}${PREFIX}/bin/${BIN}

clean:
	rm -f ${BIN} ${OBJ}
