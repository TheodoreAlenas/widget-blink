STD_WARNS = -std=c99 -pedantic -Wall
INCS = -I/usr/X11R6/include -I/usr/include/freetype2
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L

CC = cc
CFLAGS = $(STD_WARNS) $(INCS) $(CPPFLAGS)
LDFLAGS = -L/usr/X11R6/lib -lX11 -lfontconfig -lXft -lm

all: albatwid alclowid

clean:
	rm -f albatwid alclowid

albatwid: albatwid.c
	$(CC) $(CFLAGS) -DVERSION=1.0 -o $@ -DALBATWID_VERSION=\"1.0\" $^ $(LDFLAGS)

alclowid: alclowid.c
	$(CC) $(CFLAGS) -DVERSION=1.0 -o $@ -DALCLOWID_VERSION=\"1.0\" $^ $(LDFLAGS)

try-%: %
	! ./$< --nonexistent-flag
	./$<
