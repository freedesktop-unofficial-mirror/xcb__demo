/*
 * Copyright (C) 2001-2002 Bart Massey and Jamey Sharp.
 * All Rights Reserved.  See the file COPYING in this directory
 * for licensing information.
 */

#define TEST_GET_WINDOW_ATTRIBUTES
#define TEST_GET_GEOMETRY
#define TEST_QUERY_TREE
#undef TEST_THREADS
#define VERBOSE
#undef SUPERVERBOSE
#define TEST_ICCCM

#ifdef TEST_THREADS
#include <pthread.h>
#endif

#ifdef TEST_ICCCM
#include <string.h>
#endif

#include <stdlib.h>

#include <X11/XCB/xcb.h>
#include <X11/XCB/xcb_aux.h>
#include "reply_formats.h"

#ifdef VERBOSE
#include <stdio.h>
#endif

void try_events(XCBConnection *c);
void wait_events(XCBConnection *c);

static XCBConnection *c;
static XCBWINDOW window;

int main(int argc, char **argv)
{
    CARD32 mask = 0;
    CARD32 values[6];
    XCBDRAWABLE d;
    XCBSCREEN *root;
#ifdef TEST_GET_GEOMETRY
    XCBGetGeometryCookie geom[3];
    XCBGetGeometryRep *geomrep[3];
#endif
#ifdef TEST_QUERY_TREE
    XCBQueryTreeCookie tree[3];
    XCBQueryTreeRep *treerep[3];
#endif
#ifdef TEST_GET_WINDOW_ATTRIBUTES
    XCBGetWindowAttributesCookie attr[1];
    XCBGetWindowAttributesRep *attrrep[1];
#endif
#ifdef TEST_ICCCM
    XCBInternAtomCookie atom[2];
    XCBInternAtomRep *atomrep[2];
#endif
#ifdef TEST_THREADS
    pthread_t event_thread;
#endif
    int screen_num;

    c = XCBConnect(0, &screen_num);
    root = XCBAuxGetScreen(c, screen_num);

#ifdef TEST_THREADS
# ifdef VERBOSE
    printf("main() thread ID: %ld\n", pthread_self());
# endif
    /* don't do this cast. */
    pthread_create(&event_thread, 0, (void *(*)(void *))wait_events, c);
#endif

#if 1
    window = XCBWINDOWNew(c);
#else
    window = 0; /* should be an invalid ID */
#endif

    mask |= XCBCWBackPixel;
    values[0] = root->white_pixel;

    mask |= XCBCWBorderPixel;
    values[1] = root->black_pixel;

    mask |= XCBCWBackingStore;
    values[2] = Always;

    mask |= XCBCWOverrideRedirect;
    values[3] = 0;

    mask |= XCBCWEventMask;
    values[4] = ButtonReleaseMask | ExposureMask | StructureNotifyMask
        | EnterWindowMask | LeaveWindowMask;

    mask |= XCBCWDontPropagate;
    values[5] = ButtonPressMask;

    XCBCreateWindow(c, /* depth */ 0,
        window, root->root,
        /* x */ 20, /* y */ 200, /* width */ 150, /* height */ 150,
        /* border_width */ 10, /* class */ InputOutput,
        /* visual */ root->root_visual, mask, values);
#ifdef TEST_ICCCM
    atom[0] = XCBInternAtom(c, 0, sizeof("WM_PROTOCOLS")-1, "WM_PROTOCOLS");
    atom[1] = XCBInternAtom(c, 0, sizeof("WM_DELETE_WINDOW")-1, "WM_DELETE_WINDOW");
    atomrep[1] = XCBInternAtomReply(c, atom[1], 0);
    atomrep[0] = XCBInternAtomReply(c, atom[0], 0);
    {
        XCBATOM XA_WM_NAME = { 39 };
        XCBATOM XA_STRING = { 31 };
        XCBChangeProperty(c, PropModeReplace, window, XA_WM_NAME, XA_STRING, 8, strlen(argv[0]), argv[0]);
    }
    if(atomrep[0] && atomrep[1])
    {
        XCBATOM WM_PROTOCOLS = atomrep[0]->atom;
        XCBATOM WM_DELETE_WINDOW = atomrep[1]->atom;
        XCBATOM XA_ATOM = { 4 };
        XCBChangeProperty(c, PropModeReplace, window, WM_PROTOCOLS, XA_ATOM, 32, 1, &WM_DELETE_WINDOW);
    }
    free(atomrep[0]);
    free(atomrep[1]);
#endif
    try_events(c);

    XCBMapWindow(c, window);
    XCBFlush(c);

    /* Send off a collection of requests */
#ifdef TEST_GET_WINDOW_ATTRIBUTES
    attr[0] = XCBGetWindowAttributes(c, window);
#endif
#ifdef TEST_GET_GEOMETRY
    d.window = root->root;
    geom[0] = XCBGetGeometry(c, d);
    d.window = window;
    geom[1] = XCBGetGeometry(c, d);
#endif
#ifdef TEST_QUERY_TREE
# ifdef SUPERVERBOSE /* this produces a lot of output :) */
    tree[0] = XCBQueryTree(c, root->root);
# endif
    tree[1] = XCBQueryTree(c, window);
#endif

    /* Start reading replies and possibly events */
#ifdef TEST_GET_GEOMETRY
    geomrep[0] = XCBGetGeometryReply(c, geom[0], 0);
    formatGetGeometryReply(root->root, geomrep[0]);
    free(geomrep[0]);
#endif

#ifdef TEST_QUERY_TREE
# ifdef SUPERVERBOSE /* this produces a lot of output :) */
    treerep[0] = XCBQueryTreeReply(c, tree[0], 0);
    formatQueryTreeReply(root->root, treerep[0]);
    free(treerep[0]);
# endif
#endif

#ifdef TEST_GET_GEOMETRY
    geomrep[1] = XCBGetGeometryReply(c, geom[1], 0);
    formatGetGeometryReply(window, geomrep[1]);
    free(geomrep[1]);
#endif

    try_events(c);

    /* Mix in some more requests */
#ifdef TEST_QUERY_TREE
    treerep[1] = XCBQueryTreeReply(c, tree[1], 0);
    formatQueryTreeReply(window, treerep[1]);

    if(treerep[1] && treerep[1]->parent.xid && treerep[1]->parent.xid != root->root.xid)
    {
        tree[2] = XCBQueryTree(c, treerep[1]->parent);

# ifdef TEST_GET_GEOMETRY
        d.window = treerep[1]->parent;
        geom[2] = XCBGetGeometry(c, d);
        geomrep[2] = XCBGetGeometryReply(c, geom[2], 0);
        formatGetGeometryReply(treerep[1]->parent, geomrep[2]);
        free(geomrep[2]);
# endif

        treerep[2] = XCBQueryTreeReply(c, tree[2], 0);
        formatQueryTreeReply(treerep[1]->parent, treerep[2]);
        free(treerep[2]);
    }

    free(treerep[1]);
#endif

    try_events(c);

    /* Get the last reply of the first batch */
#if 1 /* if 0, leaves a reply in the reply queue */
#ifdef TEST_GET_WINDOW_ATTRIBUTES
    attrrep[0] = XCBGetWindowAttributesReply(c, attr[0], 0);
    formatGetWindowAttributesReply(window, attrrep[0]);
    free(attrrep[0]);
#endif
#endif

#ifdef TEST_THREADS
    pthread_join(event_thread, 0);
#else
    wait_events(c);
#endif

    exit(0);
    /*NOTREACHED*/
}

int show_event(XCBGenericEvent *e)
{
    int ret = 1;
    if(!formatEvent(e))
        return 0;

    if(e->response_type == XCBButtonRelease)
        ret = 0; /* They clicked, therefore, we're done. */
    free(e);
    return ret;
}

void try_events(XCBConnection *c)
{
    XCBGenericEvent *e;
    while((e = XCBPollForEvent(c, 0)) && show_event(e))
        /* empty statement */ ;
}

void wait_events(XCBConnection *c)
{
    XCBGenericEvent *e;
#ifdef TEST_THREADS
# ifdef VERBOSE
    printf("wait_events() thread ID: %ld\n", pthread_self());
# endif
#endif
    while((e = XCBWaitForEvent(c)) && show_event(e))
        /* empty statement */ ;
}
