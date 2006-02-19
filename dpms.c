/*
 * Copyright (C) 2001-2005 Bart Massey and Jamey Sharp.
 * All Rights Reserved.  See the file COPYING in this directory
 * for licensing information.
 */

#include <X11/XCB/xcb.h>
#include <X11/XCB/dpms.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	XCBConnection *c = XCBConnect(0, 0);
	XCBDPMSGetVersionCookie vc;
	XCBDPMSGetVersionRep *ver;
	XCBDPMSCapableCookie cc;
	XCBDPMSCapableRep *cap;
	XCBDPMSGetTimeoutsCookie tc;
	XCBDPMSGetTimeoutsRep *time;

	XCBPrefetchExtensionData(c, &XCBDPMSId);

	vc = XCBDPMSGetVersion(c, 1, 1);
	cc = XCBDPMSCapable(c);
	tc = XCBDPMSGetTimeouts(c);

	ver = XCBDPMSGetVersionReply(c, vc, 0);
	cap = XCBDPMSCapableReply(c, cc, 0);
	time = XCBDPMSGetTimeoutsReply(c, tc, 0);

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

	XCBDisconnect(c);

	exit(0);
	/*NOTREACHED*/
}
