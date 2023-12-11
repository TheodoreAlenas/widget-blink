#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <locale.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

static void die(const char *fmt, ...) {
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

void albatwid_draw(const char *text,
				   const char *fg_color,
				   const char *bg_color,
				   const char *charged_bg_color,
				   double timeout_seconds) {

	int screen, w, h, charged;
	Display *dpy;
	Window root, win;
	XWindowAttributes wa;
	Drawable drawable;
	GC gc;
	XftFont *xfont;
	XftColor fg, bg, bg_charged;
	XftDraw *d = NULL;
	XGlyphInfo ext;
	XSetWindowAttributes swa;
	XClassHint ch = {"dmenu", "dmenu"};

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
	h = wa.height * 6/10;
	drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
	gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, gc, 1, LineSolid, CapButt, JoinMiter);
	if (!(xfont = XftFontOpenName(dpy, screen, "Source Code Pro:size=300")))
		die("error, cannot load monospace font\n");
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
	                       DefaultColormap(dpy, screen),
						   bg_color, &bg))
		die("error, cannot allocate background color");
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
	                       DefaultColormap(dpy, screen),
						   charged_bg_color, &bg_charged))
		die("error, cannot allocate background color");
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
	                       DefaultColormap(dpy, screen),
	                       fg_color, &fg))
		die("error, cannot allocate foreground color");
	XSetForeground(dpy, gc, bg.pixel);
	XFillRectangle(dpy, drawable, gc, 0, 0, w, h);
	charged = h * 30 / 100;
	XSetForeground(dpy, gc, bg_charged.pixel);
	XFillRectangle(dpy, drawable, gc, 0, h - charged, w, charged);
	d = XftDrawCreate(dpy, drawable,
					  DefaultVisual(dpy, screen),
					  DefaultColormap(dpy, screen));
	XftTextExtentsUtf8(dpy, xfont, (XftChar8 *)text, strlen(text), &ext);
	XftDrawStringUtf8(d, &fg, xfont,
					  (w - ext.xOff) / 2,
					  (h + xfont->ascent + xfont->descent) * 4/10,  /* fiddly */
					  (XftChar8 *)text, strlen(text));
	swa.override_redirect = True;
	swa.background_pixel = bg.pixel;
	swa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask;
	win = XCreateWindow(dpy, root, 0, (wa.height - h) / 2, w, h, 0,
	                    CopyFromParent, CopyFromParent, CopyFromParent,
	                    CWOverrideRedirect | CWBackPixel | CWEventMask, &swa);
	XSetClassHint(dpy, win, &ch);
	XMapRaised(dpy, win);
	XCopyArea(dpy, drawable, win, gc, 0, 0, w, h, 0, 0);
	XSync(dpy, False);
	usleep(1000000L * timeout_seconds);

	XftFontClose(dpy, xfont);
	free(xfont);
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
	char *percentage = "69";
	double timeout = 0.5;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {puts("albatpop-"VERSION); exit(0);}
		else if (i + 1 == argc) usage(argv[0]);
		else if (!strcmp(argv[i], "-p")) percentage = argv[++i];
		else if (!strcmp(argv[i], "-t")) timeout = atof(argv[++i]);
		else usage(argv[0]);
	}
	albatwid_draw(percentage, "#afbcbf", "#000e17", "#004065", timeout);

	return 0;
}
