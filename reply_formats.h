/*
 * Copyright (C) 2001-2002 Bart Massey and Jamey Sharp.
 * All Rights Reserved.  See the file COPYING in this directory
 * for licensing information.
 */

#ifndef REPLY_FORMATS_H
#define REPLY_FORMATS_H

#include <X11/XCB/xcb.h>

int formatGetWindowAttributesReply(xcb_window_t wid, xcb_get_window_attributes_reply_t *reply);
int formatGetGeometryReply(xcb_window_t wid, xcb_get_geometry_reply_t *reply);
int formatQueryTreeReply(xcb_window_t wid, xcb_query_tree_reply_t *reply);
int formatEvent(xcb_generic_event_t *e);

#endif /* REPLY_FORMATS_H */
