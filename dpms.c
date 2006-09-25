/*
 * Copyright (C) 2001-2005 Bart Massey and Jamey Sharp.
 * All Rights Reserved.  See the file COPYING in this directory
 * for licensing information.
 */

#include <xcb/xcb.h>
#include <xcb/dpms.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	xcb_connection_t *c = xcb_connect(0, 0);
	xcb_dpms_get_version_cookie_t vc;
	xcb_dpms_get_version_reply_t *ver;
	xcb_dpms_capable_cookie_t cc;
	xcb_dpms_capable_reply_t *cap;
	xcb_dpms_get_timeouts_cookie_t tc;
	xcb_dpms_get_timeouts_reply_t *time;

	xcb_prefetch_extension_data(c, &xcb_dpms_id);

	vc = xcb_dpms_get_version(c, 1, 1);
	cc = xcb_dpms_capable(c);
	tc = xcb_dpms_get_timeouts(c);

	ver = xcb_dpms_get_version_reply(c, vc, 0);
	cap = xcb_dpms_capable_reply(c, cc, 0);
	time = xcb_dpms_get_timeouts_reply(c, tc, 0);

	assert(ver);
	assert(ver->server_major_version == 1);
	assert(ver->server_minor_version == 1);
	free(ver);

	assert(cap);
	assert(cap->capable);
	free(cap);

	assert(time);
	printf("Standby: %d\n" "Suspend: %d\n" "Off: %d\n",
		time->standby_timeout, time->suspend_timeout, time->off_timeout);
	free(time);

	xcb_disconnect(c);

	exit(0);
	/*NOTREACHED*/
}
