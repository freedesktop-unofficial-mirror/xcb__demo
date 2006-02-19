/*
 * Copyright (C) 2001-2002 Bart Massey and Jamey Sharp.
 * All Rights Reserved.  See the file COPYING in this directory
 * for licensing information.
 */

#include <X11/XCB/xcb.h>
#include <X11/XCB/xcb_aux.h>
#include "reply_formats.h"
#include <math.h>
#include <stdlib.h> /* for free(3) */
#include <unistd.h> /* for usleep(3) */
#include <stdio.h>
#include <pthread.h>

#define LAG 0.3 /* lag angle for the follow line */

/* If I've done my math right, Linux maxes out at 100 fps on Intel (1000 fps
 * on Alpha) due to the granularity of the kernel timer. */
#define FRAME_RATE 10.0 /* frames per second */

#define PI 3.14159265

static XCBConnection *c;
static XCBSCREEN *root;
static XCBGCONTEXT white, black;
static int depth;

#define WINS 8
static struct {
	XCBDRAWABLE w;
	XCBDRAWABLE p;
	CARD16 width;
	CARD16 height;
	float angv;
} windows[WINS];

void *run(void *param);
void *event_thread(void *param);

static void get_depth()
{
	XCBDRAWABLE drawable = { root->root };
	XCBGetGeometryRep *geom;
	geom = XCBGetGeometryReply(c, XCBGetGeometry(c, drawable), 0);
	if(!geom)
	{
		perror("GetGeometry(root) failed");
		abort();
	}

	depth = geom->depth;
	fprintf(stderr, "Root 0x%lx: %dx%dx%d\n",
		 root->root.xid, geom->width, geom->height, geom->depth);
	free(geom);
}

int main()
{
	pthread_t thr;
	int i;

	CARD32 mask = GCForeground | GCGraphicsExposures;
	CARD32 values[2];
	XCBDRAWABLE rootwin;
	int screen_num;

	c = XCBConnect(0, &screen_num);
	root = XCBAuxGetScreen(c, screen_num);
	get_depth();

	rootwin.window = root->root;
	white = XCBGCONTEXTNew(c);
	black = XCBGCONTEXTNew(c);

	pthread_create(&thr, 0, event_thread, 0);

	values[1] = 0; /* no graphics exposures */

	values[0] = root->white_pixel;
	XCBCreateGC(c, white, rootwin, mask, values);

	values[0] = root->black_pixel;
	XCBCreateGC(c, black, rootwin, mask, values);

	for(i = 1; i < WINS; ++i)
		pthread_create(&thr, 0, run, (void*)i);
	run((void*)0);

	exit(0);
	/*NOTREACHED*/
}

void paint(int idx)
{
	XCBCopyArea(c, windows[idx].p, windows[idx].w, white, 0, 0, 0, 0,
		windows[idx].width, windows[idx].height);
	if(!XCBSync(c, 0))
	{
		perror("XCBSync failed");
		abort();
	}
}

void *run(void *param)
{
	int idx = (int)param;

	int xo, yo;
	double r, theta = 0;

	XCBPOINT line[2];

	windows[idx].w.window = XCBWINDOWNew(c);
	windows[idx].p.pixmap = XCBPIXMAPNew(c);
	windows[idx].width = 300;
	line[0].x = xo = windows[idx].width / 2;
	windows[idx].height = 300;
	line[0].y = yo = windows[idx].height / 2;
	windows[idx].angv = 0.05;

	{
		int ws = windows[idx].width * windows[idx].width;
		int hs = windows[idx].height * windows[idx].height;
		r = sqrt(ws + hs) + 1.0;
	}

	{
		CARD32 mask = XCBCWBackPixel | XCBCWEventMask | XCBCWDontPropagate;
		CARD32 values[3];
		XCBRECTANGLE rect = { 0, 0, windows[idx].width, windows[idx].height };
		values[0] = root->white_pixel;
		values[1] = ButtonReleaseMask | ExposureMask;
		values[2] = ButtonPressMask;

		XCBCreateWindow(c, depth, windows[idx].w.window, root->root,
			/* x */ 0, /* y */ 0,
			windows[idx].width, windows[idx].height,
			/* border */ 0, InputOutput,
			/* visual */ root->root_visual,
			mask, values);

		XCBMapWindow(c, windows[idx].w.window);

		XCBCreatePixmap(c, depth,
			windows[idx].p.pixmap, windows[idx].w,
			windows[idx].width, windows[idx].height);

		XCBPolyFillRectangle(c, windows[idx].p, white, 1, &rect);
	}

	XCBSync(c, 0);

	while(1)
	{
		line[1].x = xo + r * cos(theta);
		line[1].y = yo + r * sin(theta);
		XCBPolyLine(c, CoordModeOrigin, windows[idx].p, black,
			2, line);

		line[1].x = xo + r * cos(theta + LAG);
		line[1].y = yo + r * sin(theta + LAG);
		XCBPolyLine(c, CoordModeOrigin, windows[idx].p, white,
			2, line);

		paint(idx);
		theta += windows[idx].angv;
		while(theta > 2 * PI)
			theta -= 2 * PI;
		while(theta < 0)
			theta += 2 * PI;

		usleep(1000000 / FRAME_RATE);
	}
}

int lookup_window(XCBWINDOW w)
{
	int i;
	for(i = 0; i < WINS; ++i)
		if(windows[i].w.window.xid == w.xid)
			return i;
	return -1;
}

void *event_thread(void *param)
{
	XCBGenericEvent *e;
	int idx;

	while(1)
	{
		e = XCBWaitForEvent(c);
		if(!formatEvent(e))
			return 0;
		if(e->response_type == XCBExpose)
		{
			XCBExposeEvent *ee = (XCBExposeEvent *) e;
			idx = lookup_window(ee->window);
			if(idx == -1)
				fprintf(stderr, "Expose on unknown window!\n");
			else
			{
				XCBCopyArea(c, windows[idx].p, windows[idx].w,
					white, ee->x, ee->y, ee->x, ee->y,
					ee->width, ee->height);
				if(ee->count == 0)
					XCBFlush(c);
			}
		}
		else if(e->response_type == XCBButtonRelease)
		{
			XCBButtonReleaseEvent *bre = (XCBButtonReleaseEvent *) e;
			idx = lookup_window(bre->event);
			if(idx == -1)
				fprintf(stderr, "ButtonRelease on unknown window!\n");
			else
			{
				if(bre->detail.id == Button1)
					windows[idx].angv = -windows[idx].angv;
				else if(bre->detail.id == Button4)
					windows[idx].angv += 0.001;
				else if(bre->detail.id == Button5)
					windows[idx].angv -= 0.001;
			}
		}
		free(e);
	}
}
