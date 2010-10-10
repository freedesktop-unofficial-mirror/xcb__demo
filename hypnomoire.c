/*
 * Copyright (C) 2001-2002 Bart Massey and Jamey Sharp.
 * All Rights Reserved.  See the file COPYING in this directory
 * for licensing information.
 */

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
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

static xcb_connection_t *c;
static xcb_screen_t *root;
static xcb_gcontext_t white, black;
static int depth;

#define WINS 8
static struct {
	xcb_drawable_t w;
	xcb_drawable_t p;
	uint16_t width;
	uint16_t height;
	float angv;
} windows[WINS];

void *run(void *param);
void *event_thread(void *param);

static void get_depth()
{
	xcb_drawable_t drawable = { root->root };
	xcb_get_geometry_reply_t *geom;
	geom = xcb_get_geometry_reply(c, xcb_get_geometry(c, drawable), 0);
	if(!geom)
	{
		perror("GetGeometry(root) failed");
		abort();
	}

	depth = geom->depth;
	fprintf(stderr, "Root 0x%x: %dx%dx%d\n",
	        root->root, geom->width, geom->height, geom->depth);
	free(geom);
}

int main()
{
	pthread_t thr;
	int i;

	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	uint32_t values[2];
	xcb_drawable_t rootwin;
	int screen_num;

	c = xcb_connect(0, &screen_num);
	root = xcb_aux_get_screen(c, screen_num);
	get_depth();

	rootwin = root->root;
	white = xcb_generate_id(c);
	black = xcb_generate_id(c);

	pthread_create(&thr, 0, event_thread, 0);

	values[1] = 0; /* no graphics exposures */

	values[0] = root->white_pixel;
	xcb_create_gc(c, white, rootwin, mask, values);

	values[0] = root->black_pixel;
	xcb_create_gc(c, black, rootwin, mask, values);

	for(i = 1; i < WINS; ++i)
		pthread_create(&thr, 0, run, (void*)i);
	run((void*)0);

	exit(0);
	/*NOTREACHED*/
}

void *run(void *param)
{
	int idx = (int)param;

	int xo, yo;
	double r, theta = 0;

	xcb_point_t line[2];

	windows[idx].w = xcb_generate_id(c);
	windows[idx].p = xcb_generate_id(c);
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
		uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_DONT_PROPAGATE;
		uint32_t values[3];
		xcb_rectangle_t rect = { 0, 0, windows[idx].width, windows[idx].height };
		values[0] = root->white_pixel;
		values[1] = XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_EXPOSURE;
		values[2] = XCB_EVENT_MASK_BUTTON_PRESS;

		xcb_create_window(c, depth, windows[idx].w, root->root,
			/* x */ 0, /* y */ 0,
			windows[idx].width, windows[idx].height,
			/* border */ 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			/* visual */ root->root_visual,
			mask, values);

		xcb_map_window(c, windows[idx].w);

		xcb_create_pixmap(c, depth,
			windows[idx].p, windows[idx].w,
			windows[idx].width, windows[idx].height);

		xcb_poly_fill_rectangle(c, windows[idx].p, white, 1, &rect);
	}

	xcb_flush(c);

	while(1)
	{
		line[1].x = xo + r * cos(theta);
		line[1].y = yo + r * sin(theta);
		xcb_poly_line(c, XCB_COORD_MODE_ORIGIN, windows[idx].p, black,
			2, line);

		line[1].x = xo + r * cos(theta + LAG);
		line[1].y = yo + r * sin(theta + LAG);
		xcb_poly_line(c, XCB_COORD_MODE_ORIGIN, windows[idx].p, white,
			2, line);

		xcb_copy_area(c, windows[idx].p, windows[idx].w, white, 0, 0, 0, 0,
			windows[idx].width, windows[idx].height);
		if(xcb_flush(c) <= 0)
			break;

		theta += windows[idx].angv;
		while(theta > 2 * PI)
			theta -= 2 * PI;
		while(theta < 0)
			theta += 2 * PI;

		usleep(1000000 / FRAME_RATE);
	}

        return 0;
}

int lookup_window(xcb_window_t w)
{
	int i;
	for(i = 0; i < WINS; ++i)
		if(windows[i].w == w)
			return i;
	return -1;
}

void *event_thread(void *param)
{
	xcb_generic_event_t *e;
	int idx;

	while(1)
	{
		e = xcb_wait_for_event(c);
		if(!formatEvent(e))
			return 0;
		if(e->response_type == XCB_EXPOSE)
		{
			xcb_expose_event_t *ee = (xcb_expose_event_t *) e;
			idx = lookup_window(ee->window);
			if(idx == -1)
				fprintf(stderr, "Expose on unknown window!\n");
			else
			{
				xcb_copy_area(c, windows[idx].p, windows[idx].w,
					white, ee->x, ee->y, ee->x, ee->y,
					ee->width, ee->height);
				if(ee->count == 0)
					xcb_flush(c);
			}
		}
		else if(e->response_type == XCB_BUTTON_RELEASE)
		{
			xcb_button_release_event_t *bre = (xcb_button_release_event_t *) e;
			idx = lookup_window(bre->event);
			if(idx == -1)
				fprintf(stderr, "ButtonRelease on unknown window!\n");
			else
			{
				if(bre->detail == XCB_BUTTON_INDEX_1)
					windows[idx].angv = -windows[idx].angv;
				else if(bre->detail == XCB_BUTTON_INDEX_4)
					windows[idx].angv += 0.001;
				else if(bre->detail == XCB_BUTTON_INDEX_5)
					windows[idx].angv -= 0.001;
			}
		}
		free(e);
	}
}
