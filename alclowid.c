#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
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

void draw_bar(Display *dpy, Drawable *drawable, GC *gc,
			  float radients, float length, float width,
			  int xoff, int yoff) {

	XPoint bar[4];

	bar[0].x = xoff + length * cos(radients + width / length / 2.0);
	bar[0].y = yoff - length * sin(radients + width / length / 2.0);

	bar[1].x = xoff + width * cos(radients + 3.0 * M_PI / 4.0);
	bar[1].y = yoff - width * sin(radients + 3.0 * M_PI / 4.0);

	bar[2].x = xoff + width * cos(radients + 5.0 * M_PI / 4.0);
	bar[2].y = yoff - width * sin(radients + 5.0 * M_PI / 4.0);

	bar[3].x = xoff + length * cos(radients - width / length / 2.0);
	bar[3].y = yoff - length * sin(radients - width / length / 2.0);

	XFillPolygon(dpy, *drawable, *gc, bar, 4, Convex, CoordModeOrigin);
}

void draw_balls_around(Display *dpy, Drawable *drawable, GC *gc,
					   float radius, float width,
					   int xoff, int yoff) {

	int i, n = 12;
	XArc balls[12];

	for (i = 0; i < n; i++) {
		balls[i].x = xoff - width/2 + radius * cos(2.0 * M_PI * i / n);
		balls[i].y = yoff - width/2 + radius * sin(2.0 * M_PI * i / n);
		balls[i].width = width;
		balls[i].height = width;
		balls[i].angle1 = 0;
		balls[i].angle2 = 64 * 360;
	}
	XFillArcs(dpy, *drawable, *gc, balls, n);
}

void draw_hour_number(Display *dpy, XftFont *xfont, Drawable *drawable,
                      GC *gc, XftColor fg, XftDraw *d,
                      int number, float radius,
                      int xoff, int yoff) {

	char text[16];
	XGlyphInfo ext;
	int x, y;

	sprintf(text, "%d", number);
	XftTextExtentsUtf8(dpy, xfont, (XftChar8 *)text, strlen(text), &ext);
	x = xoff - ext.xOff / 4 +
		radius * cos(M_PI / 6.0 * number - M_PI / 2.0);
	y = yoff + (xfont->ascent + xfont->descent) / 2 +
		radius * sin(M_PI / 6.0 * number - M_PI / 2.0);
	XftDrawStringUtf8(d, &fg, xfont, x, y,
	                  (XftChar8 *)text, strlen(text));

}

typedef struct {
	Display *dpy; int screen; Drawable *drawable; GC *gc;
	XftColor fg; XftColor bg_charged; XftDraw *d;
	float hour_angle; float minute_angle;
	int hl_hour_1; int hl_hour_2;
	int yoff;
	int w;
} AlDraw;

void draw_clock(AlDraw *a, int xoff) {

#define SOURCE_CODE_PRO_SIZE "Source Code Pro:size="
	char font[32] = SOURCE_CODE_PRO_SIZE;
	int l;
	XftFont *xfont;
	for (l = 0; font[l]; l++);
	sprintf(font + l, "%d", 10 * a->w / 100);
	if (!(xfont = XftFontOpenName(a->dpy, a->screen, font)))
		die("cannot load font\n");

	XSetForeground(a->dpy, *(a->gc), a->fg.pixel);
	draw_bar(a->dpy, a->drawable, a->gc, a->hour_angle,
	         25 * a->w / 100,
	         5  * a->w / 100, xoff, a->yoff);
	draw_bar(a->dpy, a->drawable, a->gc, a->minute_angle,
	         39 * a->w / 100,
	         4  * a->w / 100, xoff, a->yoff);

	XSetForeground(a->dpy, *a->gc, a->bg_charged.pixel);
	draw_balls_around(a->dpy, a->drawable, a->gc,
	                  45 * a->w / 100,
	                  5 * a->w / 100, xoff, a->yoff);

	draw_hour_number(a->dpy, xfont, a->drawable,
	                 a->gc, a->fg, a->d, a->hl_hour_1,
	                 60 * a->w / 100, xoff, a->yoff);
	draw_hour_number(a->dpy, xfont, a->drawable,
	                 a->gc, a->fg, a->d, a->hl_hour_2,
	                 60 * a->w / 100, xoff, a->yoff);

	XftFontClose(a->dpy, xfont);
	free(xfont);
}

void albatwid_draw(float hour_angle,
				   float minute_angle,
				   int hl_hour_1,
				   int hl_hour_2,
				   const char *fg_color,
				   const char *bg_color,
				   const char *charged_bg_color,
				   double timeout_seconds) {

	int screen, w, h;
	Display *dpy;
	Window root, win;
	XWindowAttributes wa;
	Drawable drawable;
	GC gc;
	XftColor fg, bg, bg_charged;
	XftDraw *d = NULL;
	XSetWindowAttributes swa;
	XClassHint ch = {"dmenu", "dmenu"};
	AlDraw aldraw;

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

	d = XftDrawCreate(dpy, drawable,
	                  DefaultVisual(dpy, screen),
	                  DefaultColormap(dpy, screen));

#define ALD_R(x) aldraw.x = x
#define ALD_P(x) aldraw.x = &x
	ALD_R(dpy); ALD_R(screen); ALD_P(drawable); ALD_P(gc); ALD_R(fg);
	ALD_R(bg_charged); ALD_R(d); ALD_R(hour_angle); ALD_R(minute_angle);
	ALD_R(hl_hour_1); ALD_R(hl_hour_2);
	aldraw.yoff = h/2;
	aldraw.w = 60 * h / 100;

	/*
	draw_clock(&aldraw, 1*w/6);
	draw_clock(&aldraw, 3*w/6);
	draw_clock(&aldraw, 5*w/6);
	*/
	draw_clock(&aldraw, w/2);

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
}

static void usage(const char *program_name) {
	fprintf(stderr,
			"Usage:\n"
			"  %s [ARGS]\n"
			"Without arguments, briefly shows a clock on\n"
			"an X11 window that goes above full screen\n"
			"windows and status bars.\n"
			"\n"
			"ARGS:\n"
			"  -v                for version information,\n"
			"  -h 12 -m 5        to display 12:05\n"
			"  -t 0.5            to close the clock in 0.5 sec\n"
			,
			program_name);
	exit(1);
}

int main(int argc, char *argv[]) {

	int i, hour = -1, min = -1;
	float timeout = 1.0;
	time_t now;
	struct tm *tm_struct;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			puts("alclowid-"ALCLOWID_VERSION);
			exit(0);
		}
		else if (i + 1 == argc) usage(argv[0]);
		else if (!strcmp(argv[i], "-h")) hour = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-m")) min = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-t")) timeout = atof(argv[++i]);
		else usage(argv[0]);
	}
	now = time(NULL);
	tm_struct = localtime(&now);

	if (hour == -1)
		hour = tm_struct->tm_hour;

	if (min == -1)
		min = tm_struct->tm_min;

	albatwid_draw(-(hour + min / 60.0) / 12.0 * M_PI * 2.0 + M_PI / 2.0,
				  -min / 60.0 * M_PI * 2.0 + M_PI / 2.0,
				  hour, hour + 1,
				  "#379cf6", "#000e17", "#004065", timeout);
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
