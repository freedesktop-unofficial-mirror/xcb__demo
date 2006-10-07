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

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include "reply_formats.h"

#ifdef VERBOSE
#include <stdio.h>
#endif

void try_events(xcb_connection_t *c);
void wait_events(xcb_connection_t *c);

static xcb_connection_t *c;
static xcb_window_t window;

int main(int argc, char **argv)
{
    uint32_t mask = 0;
    uint32_t values[6];
    xcb_drawable_t d;
    xcb_screen_t *root;
#ifdef TEST_GET_GEOMETRY
    xcb_get_geometry_cookie_t geom[3];
    xcb_get_geometry_reply_t *geomrep[3];
#endif
#ifdef TEST_QUERY_TREE
    xcb_query_tree_cookie_t tree[3];
    xcb_query_tree_reply_t *treerep[3];
#endif
#ifdef TEST_GET_WINDOW_ATTRIBUTES
    xcb_get_window_attributes_cookie_t attr[1];
    xcb_get_window_attributes_reply_t *attrrep[1];
#endif
#ifdef TEST_ICCCM
    xcb_intern_atom_cookie_t atom[2];
    xcb_intern_atom_reply_t *atomrep[2];
#endif
#ifdef TEST_THREADS
    pthread_t event_thread;
#endif
    int screen_num;

    c = xcb_connect(0, &screen_num);
    root = xcb_aux_get_screen(c, screen_num);

#ifdef TEST_THREADS
# ifdef VERBOSE
    printf("main() thread ID: %ld\n", pthread_self());
# endif
    /* don't do this cast. */
    pthread_create(&event_thread, 0, (void *(*)(void *))wait_events, c);
#endif

#if 1
    window = xcb_generate_id(c);
#else
    window = 0; /* should be an invalid ID */
#endif

    mask |= XCB_CW_BACK_PIXEL;
    values[0] = root->white_pixel;

    mask |= XCB_CW_BORDER_PIXEL;
    values[1] = root->black_pixel;

    mask |= XCB_CW_BACKING_STORE;
    values[2] = XCB_BACKING_STORE_ALWAYS;

    mask |= XCB_CW_OVERRIDE_REDIRECT;
    values[3] = 0;

    mask |= XCB_CW_EVENT_MASK;
    values[4] = XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW;

    mask |= XCB_CW_DONT_PROPAGATE;
    values[5] = XCB_EVENT_MASK_BUTTON_PRESS;

    xcb_create_window(c, /* depth */ 0,
        window, root->root,
        /* x */ 20, /* y */ 200, /* width */ 150, /* height */ 150,
        /* border_width */ 10, /* class */ XCB_WINDOW_CLASS_INPUT_OUTPUT,
        /* visual */ root->root_visual, mask, values);
#ifdef TEST_ICCCM
    atom[0] = xcb_intern_atom(c, 0, sizeof("WM_PROTOCOLS")-1, "WM_PROTOCOLS");
    atom[1] = xcb_intern_atom(c, 0, sizeof("WM_DELETE_WINDOW")-1, "WM_DELETE_WINDOW");
    atomrep[1] = xcb_intern_atom_reply(c, atom[1], 0);
    atomrep[0] = xcb_intern_atom_reply(c, atom[0], 0);
    {
        xcb_atom_t XA_WM_NAME = { 39 };
        xcb_atom_t XA_STRING = { 31 };
        xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, XA_WM_NAME, XA_STRING, 8, strlen(argv[0]), argv[0]);
    }
    if(atomrep[0] && atomrep[1])
    {
        xcb_atom_t WM_PROTOCOLS = atomrep[0]->atom;
        xcb_atom_t WM_DELETE_WINDOW = atomrep[1]->atom;
        xcb_atom_t XA_ATOM = { 4 };
        xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, WM_PROTOCOLS, XA_ATOM, 32, 1, &WM_DELETE_WINDOW);
    }
    free(atomrep[0]);
    free(atomrep[1]);
#endif
    try_events(c);

    xcb_map_window(c, window);
    xcb_flush(c);

    /* Send off a collection of requests */
#ifdef TEST_GET_WINDOW_ATTRIBUTES
    attr[0] = xcb_get_window_attributes(c, window);
#endif
#ifdef TEST_GET_GEOMETRY
    geom[0] = xcb_get_geometry(c, root->root);
    geom[1] = xcb_get_geometry(c, window);
#endif
#ifdef TEST_QUERY_TREE
# ifdef SUPERVERBOSE /* this produces a lot of output :) */
    tree[0] = xcb_query_tree(c, root->root);
# endif
    tree[1] = xcb_query_tree(c, window);
#endif

    /* Start reading replies and possibly events */
#ifdef TEST_GET_GEOMETRY
    geomrep[0] = xcb_get_geometry_reply(c, geom[0], 0);
    formatGetGeometryReply(root->root, geomrep[0]);
    free(geomrep[0]);
#endif

#ifdef TEST_QUERY_TREE
# ifdef SUPERVERBOSE /* this produces a lot of output :) */
    treerep[0] = xcb_query_tree_reply(c, tree[0], 0);
    formatQueryTreeReply(root->root, treerep[0]);
    free(treerep[0]);
# endif
#endif

#ifdef TEST_GET_GEOMETRY
    geomrep[1] = xcb_get_geometry_reply(c, geom[1], 0);
    formatGetGeometryReply(window, geomrep[1]);
    free(geomrep[1]);
#endif

    try_events(c);

    /* Mix in some more requests */
#ifdef TEST_QUERY_TREE
    treerep[1] = xcb_query_tree_reply(c, tree[1], 0);
    formatQueryTreeReply(window, treerep[1]);

    if(treerep[1] && treerep[1]->parent && treerep[1]->parent != root->root)
    {
        tree[2] = xcb_query_tree(c, treerep[1]->parent);

# ifdef TEST_GET_GEOMETRY
        geom[2] = xcb_get_geometry(c, treerep[1]->parent);
        geomrep[2] = xcb_get_geometry_reply(c, geom[2], 0);
        formatGetGeometryReply(treerep[1]->parent, geomrep[2]);
        free(geomrep[2]);
# endif

        treerep[2] = xcb_query_tree_reply(c, tree[2], 0);
        formatQueryTreeReply(treerep[1]->parent, treerep[2]);
        free(treerep[2]);
    }

    free(treerep[1]);
#endif

    try_events(c);

    /* Get the last reply of the first batch */
#if 1 /* if 0, leaves a reply in the reply queue */
#ifdef TEST_GET_WINDOW_ATTRIBUTES
    attrrep[0] = xcb_get_window_attributes_reply(c, attr[0], 0);
    formatGetWindowAttributesReply(window, attrrep[0]);
    free(attrrep[0]);
#endif
#endif

#ifdef TEST_THREADS
    pthread_join(event_thread, 0);
#else
    wait_events(c);
#endif
    xcb_disconnect(c);
    exit(0);
    /*NOTREACHED*/
}

int show_event(xcb_generic_event_t *e)
{
    int ret = 1;
    if(!formatEvent(e))
        return 0;

    if(e->response_type == XCB_BUTTON_RELEASE)
        ret = 0; /* They clicked, therefore, we're done. */
    free(e);
    return ret;
}

void try_events(xcb_connection_t *c)
{
    xcb_generic_event_t *e;
    while((e = xcb_poll_for_event(c)) && show_event(e))
        /* empty statement */ ;
}

void wait_events(xcb_connection_t *c)
{
    xcb_generic_event_t *e;
#ifdef TEST_THREADS
# ifdef VERBOSE
    printf("wait_events() thread ID: %ld\n", pthread_self());
# endif
#endif
    while((e = xcb_wait_for_event(c)) && show_event(e))
        /* empty statement */ ;
}
