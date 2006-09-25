/* Copyright (C) 2006 Josh Triplett. All Rights Reserved.  See the file
 * COPYING in this directory for licensing information. */
#include <xcb/xcb.h>
#include <xcb/xf86dri.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int screen;
    xcb_xf86dri_query_version_cookie_t qvc;
    xcb_xf86dri_query_version_reply_t *qv;
    xcb_xf86dri_query_direct_rendering_capable_cookie_t qdrc;
    xcb_xf86dri_query_direct_rendering_capable_reply_t *qdr;
    xcb_connection_t *c = xcb_connect(NULL, &screen);

    if(!c)
    {
        fprintf(stderr, "Error establishing connection to X server.");
        return 1;
    }

    qvc  = xcb_xf86dri_query_version(c);
    qdrc = xcb_xf86dri_query_direct_rendering_capable(c, screen);

    qv  = xcb_xf86dri_query_version_reply(c, qvc, 0);
    if(!qv)
    {
        fprintf(stderr, "Error querying DRI extension version.\n");
        return 1;
    }
    printf("DRI extension version: %hu.%hu.%u\n",
           qv->dri_major_version, qv->dri_minor_version, qv->dri_minor_patch);
    free(qv);

    qdr = xcb_xf86dri_query_direct_rendering_capable_reply(c, qdrc, 0);
    if(!qdr)
    {
        fprintf(stderr, "Error querying direct rendering capability.\n");
        return 1;
    }
    printf("Direct rendering (screen %d): %s\n",
           screen, qdr->is_capable ? "yes" : "no");
    free(qdr);

    xcb_disconnect(c);

    return 0;
}
