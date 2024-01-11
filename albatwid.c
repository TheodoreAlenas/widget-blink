#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <locale.h>
#include <X11/Xft/Xft.h>

/* License at the bottom */

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

static void get_percentage(char *percentage) {
	FILE *f;

	if (!(f = fopen("/sys/class/power_supply/BAT1/capacity", "r"))) {
		perror("fopen");
		exit(-1);
	}
	fread(percentage, 4, 1, f);
	fclose(f);
	for (; *percentage != '\0'; percentage++)
		if (*percentage == '\n')
			*percentage = '\0';
}
static int get_is_charging(void) {
	FILE *f;
	char first_letter = '\0';

	if (!(f = fopen("/sys/class/power_supply/BAT1/status", "r"))) {
		perror("fopen");
		exit(-1);
	}
	fread(&first_letter, 1, 1, f);
	fclose(f);
	return first_letter == 'C';
}

void albatwid_draw(const char *text,
				   const char *fg_color,
				   const char *bg_color,
				   const char *charged_bg_color,
				   double timeout_seconds) {

	int screen, w, h, charged, charged_percent;
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

	charged_percent = atoi(text);

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
		die("cannot load font\n");
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
	                       DefaultColormap(dpy, screen),
						   bg_color, &bg))
		die("cannot allocate background color");
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
	                       DefaultColormap(dpy, screen),
						   charged_bg_color, &bg_charged))
		die("cannot allocate background color");
	if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen),
	                       DefaultColormap(dpy, screen),
	                       fg_color, &fg))
		die("error, cannot allocate foreground color");
	XSetForeground(dpy, gc, bg.pixel);
	XFillRectangle(dpy, drawable, gc, 0, 0, w, h);
	charged = charged_percent * h / 100;
	XSetForeground(dpy, gc, bg_charged.pixel);
	XFillRectangle(dpy, drawable, gc, 0, h - charged, w, charged);
	d = XftDrawCreate(dpy, drawable,
					  DefaultVisual(dpy, screen),
					  DefaultColormap(dpy, screen));
	XftTextExtentsUtf8(dpy, xfont, (XftChar8 *)text, strlen(text), &ext);
	XftDrawStringUtf8(d, &fg, xfont,
					  (w - ext.xOff) / 2,
					  (h + xfont->ascent + xfont->descent) * 4/10,
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
			"  %s [ARGS]\n"
			"Without arguments, gathers battery statistics and\n"
			"shows them briefly on an X11 window that goes\n"
			"above full screen windows and status bars.\n"
			"\n"
			"ARGS:\n"
			"  -v                for version information\n"
			"  -c -p 100 -t 1.5  to show charging 100%% for 1.5 sec\n"
			"  -C -p 0 -t 3      to show not charging 0%% for 3 sec\n"
			"  -o 30             only appear if discharging & <30%%\n"
			,
			program_name);
	exit(1);
}

int main(int argc, char *argv[]) {
	char *percentage = NULL, to_fill[5] = {'\0', '\0', '\0', '\0', '\0'};
	int is_charging = -1, only_below = -1;
	double timeout = 0.5;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			puts("albatwid-"ALBATWID_VERSION);
			exit(0);
		}
		else if (!strcmp(argv[i], "-c")) is_charging = 1;
		else if (!strcmp(argv[i], "-C")) is_charging = 0;
		else if (i + 1 == argc) usage(argv[0]);
		else if (!strcmp(argv[i], "-p")) percentage = argv[++i];
		else if (!strcmp(argv[i], "-t")) timeout = atof(argv[++i]);
		else if (!strcmp(argv[i], "-o")) only_below = atoi(argv[++i]);
		else usage(argv[0]);
	}
	if (!percentage) {
		get_percentage(to_fill);
		percentage = to_fill;
	}
	if (only_below >= 0 && atoi(percentage) > only_below)
		exit(0);
	if (is_charging == -1)
		is_charging = get_is_charging();
	if (only_below >= 0 && is_charging)
		exit(0);
	if (is_charging)
		albatwid_draw(percentage, "#cf9f7f", "#000e17", "#904f30", timeout);
	else
		albatwid_draw(percentage, "#379cf6", "#000e17", "#004065", timeout);
	return 0;
}

/*
Copyright (c) 2024 Dimakopoulos Theodoros

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
