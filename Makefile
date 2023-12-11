VERSION=0.0

STD_WARNS = -std=c99 -pedantic -Wall
INCS = -I/usr/X11R6/include -I/usr/include/freetype2
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L -DVERSION=$V

CC = cc
CFLAGS = $(STD_WARNS) $(INCS) $(CPPFLAGS)
LDFLAGS = -L/usr/X11R6/lib -lX11 -lfontconfig -lXft

albatwid: main.c
	$(CC) $(CFLAGS) -o $@ -DVERSION=$V $^ $(LDFLAGS)

test: albatwid
	! ./albatwid --nonexistent-flag
	./albatwid
