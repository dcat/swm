PREFIX	 = /usr

CC	?= cc
LD	 = ${CC}

CFLAGS	+= -Wall -I/opt/X11/include -I/usr/X11R6/include
LDFLAGS	+= -L/opt/X11/lib -L/usr/X11R6/lib

