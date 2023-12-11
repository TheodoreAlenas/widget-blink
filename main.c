#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

int percentage = 101;
double timeout = 0.5;

void die(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

static void usage(const char *program_name) {
	fprintf(stderr,
			"Usage:\n"
			"  %s -p 100 -t 1.5  to show 100%% for 1.5 sec,\n"
			"  %s -v             for version information,\n"
			"  %s                for default behavior\n"
			,
			program_name, program_name, program_name);
	exit(1);
}

int main(int argc, char *argv[]) {
	int screen, w, h;
	Display *dpy;
	Window root;
	XWindowAttributes wa;
	Drawable drawable;
	GC gc;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			puts("albatpop-"VERSION);
			exit(0);
		}
		else if (i + 1 == argc)  /* the next arguments take values */
			usage(argv[0]);
		else if (!strcmp(argv[i], "-p"))
			percentage = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-t"))
			timeout = atof(argv[++i]);
		else
			usage(argv[0]);
	}
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("cannot open display");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	if (!XGetWindowAttributes(dpy, root, &wa))
		die("could not get embedding window attributes: 0x%lx",
		    root);
	w = wa.width;
	h = wa.height / 4;
	drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
	gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
	return 0;
}
