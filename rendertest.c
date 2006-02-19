
#include <X11/XCB/xcb.h>
#include <X11/XCB/xcb_aux.h>
#include <X11/XCB/render.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * FUNCTION PROTOTYPES
 */
void print_version_info(XCBRenderQueryVersionRep *reply);
int print_formats_info(XCBRenderQueryPictFormatsRep *reply);
int draw_window(XCBConnection *conn, XCBRenderQueryPictFormatsRep *reply);
XCBRenderPICTFORMAT get_pictformat_from_visual(XCBRenderQueryPictFormatsRep *reply, XCBVISUALID visual);
XCBRenderPICTFORMINFO *get_pictforminfo(XCBRenderQueryPictFormatsRep *reply, XCBRenderPICTFORMINFO *query);

XCBConnection   *c;
XCBRenderPICTFORMAT pf;

XCBRenderFIXED make_fixed(INT16 i, INT16 f)
{
    return (i << 16) | (f & 0xffff);
}

void print_version_info(XCBRenderQueryVersionRep *reply)
{
    
    fprintf(stdout, "Render Version: %ld.%ld\n", reply->major_version, 
            reply->minor_version);
}

int print_formats_info(XCBRenderQueryPictFormatsRep *reply)
{
    XCBRenderPICTFORMINFO *first_forminfo;
    int num_formats;
    int num_screens;
    int num_depths;
    int num_visuals;
    XCBRenderPICTFORMINFOIter forminfo_iter;
    XCBRenderPICTSCREENIter     screen_iter;
    
    forminfo_iter = XCBRenderQueryPictFormatsFormatsIter(reply);
    screen_iter =  XCBRenderQueryPictFormatsScreensIter(reply);

    fprintf(stdout, "Number of PictFormInfo iterations: %d\n", forminfo_iter.rem);

    num_formats = reply->num_formats;
    first_forminfo = forminfo_iter.data;
    pf = first_forminfo->id;
    while(forminfo_iter.rem)
    {
        XCBRenderPICTFORMINFO *forminfo = (XCBRenderPICTFORMINFO *)forminfo_iter.data;

        fprintf(stdout, "PICTFORMINFO #%d\n", 1 + num_formats - forminfo_iter.rem);
        fprintf(stdout, "    PICTFORMAT ID:          %ld\n", forminfo->id.xid);
        fprintf(stdout, "    PICTFORMAT Type:        %d\n", forminfo->type);
        fprintf(stdout, "    PICTFORMAT Depth:       %d\n", forminfo->depth);
        fprintf(stdout, "        Direct RedShift:    %d\n", forminfo->direct.red_shift);
        fprintf(stdout, "        Direct RedMask:     %d\n", forminfo->direct.red_mask);
        fprintf(stdout, "        Direct BlueShift:   %d\n", forminfo->direct.blue_shift);
        fprintf(stdout, "        Direct BlueMask:    %d\n", forminfo->direct.blue_mask);
        fprintf(stdout, "        Direct GreenShift:  %d\n", forminfo->direct.green_shift);
        fprintf(stdout, "        Direct GreenMask:   %d\n", forminfo->direct.green_mask);
        fprintf(stdout, "        Direct AlphaShift:  %d\n", forminfo->direct.alpha_shift);
        fprintf(stdout, "        Direct AlphaMask:   %d\n", forminfo->direct.alpha_mask);
        fprintf(stdout, "\n");
        XCBRenderPICTFORMINFONext(&forminfo_iter);
    }

    num_screens = reply->num_screens;
    while(screen_iter.rem)
    {
        XCBRenderPICTDEPTHIter depth_iter;
        XCBRenderPICTSCREEN *cscreen = screen_iter.data;
        
        fprintf(stdout, "Screen #%d\n", 1 + num_screens - screen_iter.rem);
        fprintf(stdout, "    Depths for this screen:    %ld\n", cscreen->num_depths);
        fprintf(stdout, "    Fallback PICTFORMAT:       %ld\n", cscreen->fallback.xid);
        depth_iter = XCBRenderPICTSCREENDepthsIter(cscreen);

        num_depths = cscreen->num_depths;
        while(depth_iter.rem)
        {
            XCBRenderPICTVISUALIter    visual_iter;
            XCBRenderPICTDEPTH *cdepth = depth_iter.data;

            fprintf(stdout, "    Depth #%d\n", 1 + num_depths - depth_iter.rem);
            fprintf(stdout, "        Visuals for this depth:    %d\n", cdepth->num_visuals);
            fprintf(stdout, "        Depth:                     %d\n", cdepth->depth);
            visual_iter = XCBRenderPICTDEPTHVisualsIter(cdepth);

            num_visuals = cdepth->num_visuals;
            while(visual_iter.rem)
            {
                XCBRenderPICTVISUAL *cvisual = visual_iter.data;
                
                fprintf(stdout, "        Visual #%d\n", 1 + num_visuals - visual_iter.rem);
                fprintf(stdout, "            VISUALID:      %ld\n", cvisual->visual.id);
                fprintf(stdout, "            PICTFORMAT:    %ld\n", cvisual->format.xid);
                XCBRenderPICTVISUALNext(&visual_iter);
            }
            XCBRenderPICTDEPTHNext(&depth_iter);
        }
        XCBRenderPICTSCREENNext(&screen_iter);
    }
    return 0;
}

int draw_window(XCBConnection *conn, XCBRenderQueryPictFormatsRep *reply)
{
    XCBWINDOW          window;
    XCBDRAWABLE        window_drawable, tmp, root_drawable;
    XCBPIXMAP          surfaces[4], alpha_surface;
    XCBRenderPICTFORMAT      alpha_mask_format, window_format, surface_format, no_format = {0};
    XCBRenderPICTURE         window_pict, pict_surfaces[4], alpha_pict, 
                        no_picture = {0}, root_picture;
    XCBRenderPICTFORMINFO    *forminfo_ptr, *alpha_forminfo_ptr, query;
    CARD32          value_mask, value_list[4];
    XCBRECTANGLE       pict_rect[1], window_rect;
    XCBRenderCOLOR           pict_color[4], back_color, alpha_color;
    XCBSCREEN          *root;
    XCBRenderTRAP            traps[4];
    XCBRenderTRIANGLE        triangles[4];
    XCBRenderPOINTFIX        tristrips[9];
    XCBRenderPOINTFIX        trifans[9];
    int index;

    root = XCBConnSetupSuccessRepRootsIter(XCBGetSetup(c)).data;
    root_drawable.window = root->root;
   
    /* Setting query so that it will search for an 8 bit alpha surface. */
    query.id.xid = 0;
    query.type = XCBRenderPictTypeDirect;
    query.depth = 8;
    query.direct.red_mask = 0;
    query.direct.green_mask = 0;
    query.direct.blue_mask = 0;
    query.direct.alpha_mask = 255;

    /* Get the XCBRenderPICTFORMAT associated with the window. */
    window_format = get_pictformat_from_visual(reply, root->root_visual);

    /* Get the XCBRenderPICTFORMAT we will use for the alpha mask */
    alpha_forminfo_ptr = get_pictforminfo(reply, &query);
    alpha_mask_format.xid = alpha_forminfo_ptr->id.xid;
    
    /* resetting certain parts of query to search for the surface format */
    query.depth = 32;
    query.direct.alpha_mask = 0;
  
    /* Get the surface forminfo and XCBRenderPICTFORMAT */
    forminfo_ptr = get_pictforminfo(reply, &query);
    surface_format.xid = forminfo_ptr->id.xid;
    
    /* assign XIDs to all of the drawables and pictures */
    for(index = 0; index < 4; index++)
    {
        surfaces[index] = XCBPIXMAPNew(conn);
        pict_surfaces[index] = XCBRenderPICTURENew(conn);
    }
    alpha_surface = XCBPIXMAPNew(conn);
    alpha_pict = XCBRenderPICTURENew(conn);
    window = XCBWINDOWNew(conn);
    window_pict = XCBRenderPICTURENew(conn);
    window_drawable.window = window;
    root_picture = XCBRenderPICTURENew(conn);
    
    /* Here we will create the pixmaps that we will use */
    for(index = 0; index < 4; index++)
    {
        surfaces[index] = XCBPIXMAPNew(conn);
        XCBCreatePixmap(conn, 32, surfaces[index], root_drawable, 600, 600);
    }
    alpha_surface = XCBPIXMAPNew(conn);
    XCBCreatePixmap(conn, 8, alpha_surface, root_drawable, 600, 600);
    
    /* initialize the value list */
    value_mask = XCBCWEventMask;
    value_list[0] = XCBExpose;
    
    /* Create the window */
    XCBCreateWindow(conn, /* XCBConnection */
            0,  /* depth, 0 means it will copy it from the parent */
            window, root_drawable.window, /* window and parent */
            0, 0,   /* x and y */
            600, 600,   /* width and height */
            0,  /* border width */
            InputOutput,    /* class */
            root->root_visual,   /* XCBVISUALID */
            value_mask, value_list); /* LISTofVALUES */
    
    /* 
     * Create the pictures 
     */
    value_mask = 1<<0; /* repeat (still needs to be added to xcb_render.m4) */
    value_list[0] = 1;

    XCBRenderCreatePicture(conn, root_picture, root_drawable, window_format,
            value_mask, value_list);
    XCBRenderCreatePicture(conn, window_pict, window_drawable, window_format,
            value_mask, value_list);
    tmp.pixmap = alpha_surface;
    XCBRenderCreatePicture(conn, alpha_pict, tmp, alpha_mask_format,
            value_mask, value_list);
    for(index = 0; index < 4; index++)
    {
        tmp.pixmap = surfaces[index];
        XCBRenderCreatePicture(conn, pict_surfaces[index], tmp, surface_format,
                value_mask, value_list);
    }

    /* 
     * initialize the rectangles
     */
    window_rect.x = 0;
    window_rect.y = 0;
    window_rect.width = 600;
    window_rect.height = 600;

    pict_rect[0].x = 0;
    pict_rect[0].y = 0;
    pict_rect[0].width = 600;
    pict_rect[0].height = 600;
   
    /* 
     * initialize the colors
     */
    back_color.red = 0xffff;
    back_color.green = 0xffff;
    back_color.blue = 0xffff;
    back_color.alpha = 0xffff;
   
    pict_color[0].red = 0x5fff;
    pict_color[0].green = 0x0000;
    pict_color[0].blue = 0x0000;
    pict_color[0].alpha = 0x5fff;
    
    pict_color[1].red = 0x0000;
    pict_color[1].green = 0x5fff;
    pict_color[1].blue = 0x0000;
    pict_color[1].alpha = 0x5fff;

    pict_color[2].red = 0x0000;
    pict_color[2].green = 0x0000;
    pict_color[2].blue = 0x5fff;
    pict_color[2].alpha = 0x5fff;

    pict_color[3].red = 0x0000;
    pict_color[3].green = 0x0000;
    pict_color[3].blue = 0x5fff;
    pict_color[3].alpha = 0x5fff;

    alpha_color.red = 0x0000;
    alpha_color.green = 0x0000;
    alpha_color.blue = 0x0000;
    alpha_color.alpha = 0xffff;

    /* Create the trapeziod dimensions */
    traps[0].top = make_fixed(300, 32000);
    traps[0].bottom = make_fixed(416, 0);
    traps[0].left.p1.y = make_fixed(250, 0);
    traps[0].left.p1.x = make_fixed(300, 0);
    traps[0].left.p2.y = make_fixed(500, 0);
    traps[0].left.p2.x = make_fixed(100, 0);
    traps[0].right.p1.y = make_fixed(250, 0);
    traps[0].right.p1.x = make_fixed(300, 0);
    traps[0].right.p2.y = make_fixed(505, 6000);
    traps[0].right.p2.x = make_fixed(456, 512);

    /* Create the triangle dimensions */
    triangles[0].p1.x = make_fixed(100, 40000);
    triangles[0].p1.y = make_fixed(100, 0);
    triangles[0].p2.x = make_fixed(400, 0);
    triangles[0].p2.y = make_fixed(150, 30000);
    triangles[0].p3.x = make_fixed(30, 0);
    triangles[0].p3.y = make_fixed(456, 0);

    /* Create the tristrip dimensions */
    tristrips[0].x = make_fixed(400, 0);
    tristrips[0].y = make_fixed(50, 0);
    tristrips[1].x = make_fixed(436, 0);
    tristrips[1].y = make_fixed(50, 0);
    tristrips[2].x = make_fixed(398, 0);
    tristrips[2].y = make_fixed(127, 0);
    tristrips[3].x = make_fixed(450, 0);
    tristrips[3].y = make_fixed(120, 0);
    tristrips[4].x = make_fixed(450, 0);
    tristrips[4].y = make_fixed(180, 0);
    tristrips[5].x = make_fixed(503, 0);
    tristrips[5].y = make_fixed(124, 0);
    tristrips[6].x = make_fixed(500, 0);
    tristrips[6].y = make_fixed(217, 0);
    tristrips[7].x = make_fixed(542, 0);
    tristrips[7].y = make_fixed(237, 0);
    tristrips[8].x = make_fixed(501, 0);
    tristrips[8].y = make_fixed(250, 0);

    /* Create the trifan dimensions */
    trifans[0].x = make_fixed(424, 0);
    trifans[0].y = make_fixed(415, 0);
    trifans[1].x = make_fixed(375, 0);
    trifans[1].y = make_fixed(355, 0);
    trifans[2].x = make_fixed(403, 0);
    trifans[2].y = make_fixed(350, 0);
    trifans[3].x = make_fixed(430, 0);
    trifans[3].y = make_fixed(380, 0);
    trifans[4].x = make_fixed(481, 0);
    trifans[4].y = make_fixed(400, 0);
    trifans[5].x = make_fixed(475, 0);
    trifans[5].y = make_fixed(437, 0);
    trifans[6].x = make_fixed(430, 0);
    trifans[6].y = make_fixed(444, 0);
    trifans[7].x = make_fixed(400, 0);
    trifans[7].y = make_fixed(430, 0);

    /* 
     * Map the window
     */
    XCBMapWindow(conn, window);
    
    /*
     * Play around with Render
     */

    XCBRenderFillRectangles(conn, XCBRenderPictOpSrc, alpha_pict, alpha_color, 1, pict_rect);
    XCBRenderFillRectangles(conn, XCBRenderPictOpSrc, pict_surfaces[0], pict_color[0], 1, pict_rect);
    XCBRenderFillRectangles(conn, XCBRenderPictOpSrc, pict_surfaces[1], pict_color[1], 1, pict_rect);
    XCBRenderFillRectangles(conn, XCBRenderPictOpSrc, pict_surfaces[2], pict_color[2], 1, pict_rect);
    XCBRenderFillRectangles(conn, XCBRenderPictOpSrc, pict_surfaces[3], pict_color[3], 1, pict_rect);

    XCBFlush(conn);
    sleep(1);

    XCBRenderFillRectangles(conn, XCBRenderPictOpOver, window_pict, back_color, 1, &window_rect);

    XCBFlush(conn);
    sleep(1);


    /* Composite the first pict_surface onto the window picture */
    XCBRenderComposite(conn, XCBRenderPictOpOver, pict_surfaces[0], no_picture /* alpha_pict */, window_pict,
            0, 0, 0, 0, 200, 200,
            400, 400);
    XCBFlush(conn);
    sleep(1);
/*
    XCBRenderComposite(conn, XCBRenderPictOpOver, pict_surfaces[0], alpha_pict, window_pict,
            0, 0, 0, 0, 0, 0,
            200, 200);
    XCBFlush(conn);
    sleep(1);
*/
    XCBRenderComposite(conn, XCBRenderPictOpOver, pict_surfaces[1], no_picture /* alpha_pict */, window_pict,
            0, 0, 0, 0, 0, 0,
            400, 400);
    XCBFlush(conn);
    sleep(1);
    
    XCBRenderComposite(conn, XCBRenderPictOpOver, pict_surfaces[2], no_picture /* alpha_pict */, window_pict,
            0, 0, 0, 0, 200, 0,
            400, 400);
    XCBFlush(conn);
    sleep(1);
    
    XCBRenderComposite(conn, XCBRenderPictOpOver, pict_surfaces[3],  no_picture /* alpha_pict */, window_pict,
            0, 0, 0, 0, 0, 200,
            400, 400);
    XCBFlush(conn);
    sleep(1);

    XCBRenderTrapezoids(conn, XCBRenderPictOpOver, pict_surfaces[0], window_pict, alpha_mask_format, 0, 0, 1, &traps[0]);
    XCBFlush(conn);
    sleep(1);

    XCBRenderTriangles(conn, XCBRenderPictOpOver, pict_surfaces[1], window_pict, no_format, 0, 0, 1, &triangles[0]);
    XCBFlush(conn);
    sleep(1);
    
    XCBRenderTriStrip(conn, XCBRenderPictOpOver, pict_surfaces[2], window_pict, no_format, 0, 0, 9, &tristrips[0]);
    XCBFlush(conn);
    sleep(1);
    
    XCBRenderTriFan(conn, XCBRenderPictOpOver, pict_surfaces[3], window_pict, no_format, 0, 0, 8, &trifans[0]);
    XCBFlush(conn);
    sleep(2);
    
    
    /* Free up all of the resources we used */
    for(index = 0; index < 4; index++)
    {
        XCBFreePixmap(conn, surfaces[index]);
        XCBRenderFreePicture(conn, pict_surfaces[index]);
    }
    XCBFreePixmap(conn, alpha_surface);
    XCBRenderFreePicture(conn, alpha_pict);
    XCBRenderFreePicture(conn, window_pict);
    XCBRenderFreePicture(conn, root_picture);
   
    /* sync up and leave the function */
    XCBSync(conn, 0);
    return 0;
}


/**********************************************************
 * This function searches through the reply for a 
 * PictVisual who's XCBVISUALID is the same as the one
 * specified in query. The function will then return the
 * XCBRenderPICTFORMAT from that PictVIsual structure. 
 * This is useful for getting the XCBRenderPICTFORMAT that is
 * the same visual type as the root window.
 **********************************************************/
XCBRenderPICTFORMAT get_pictformat_from_visual(XCBRenderQueryPictFormatsRep *reply, XCBVISUALID query)
{
    XCBRenderPICTSCREENIter screen_iter;
    XCBRenderPICTSCREEN    *cscreen;
    XCBRenderPICTDEPTHIter  depth_iter;
    XCBRenderPICTDEPTH     *cdepth;
    XCBRenderPICTVISUALIter visual_iter; 
    XCBRenderPICTVISUAL    *cvisual;
    XCBRenderPICTFORMAT  return_value;
    
    screen_iter = XCBRenderQueryPictFormatsScreensIter(reply);

    while(screen_iter.rem)
    {
        cscreen = screen_iter.data;
        
        depth_iter = XCBRenderPICTSCREENDepthsIter(cscreen);
        while(depth_iter.rem)
        {
            cdepth = depth_iter.data;

            visual_iter = XCBRenderPICTDEPTHVisualsIter(cdepth);
            while(visual_iter.rem)
            {
                cvisual = visual_iter.data;

                if(cvisual->visual.id == query.id)
                {
                    return cvisual->format;
                }
                XCBRenderPICTVISUALNext(&visual_iter);
            }
            XCBRenderPICTDEPTHNext(&depth_iter);
        }
        XCBRenderPICTSCREENNext(&screen_iter);
    }
    return_value.xid = 0;
    return return_value;
}

XCBRenderPICTFORMINFO *get_pictforminfo(XCBRenderQueryPictFormatsRep *reply, XCBRenderPICTFORMINFO *query)
{
    XCBRenderPICTFORMINFOIter forminfo_iter;
    
    forminfo_iter = XCBRenderQueryPictFormatsFormatsIter(reply);

    while(forminfo_iter.rem)
    {
        XCBRenderPICTFORMINFO *cformat;
        cformat  = forminfo_iter.data;
        XCBRenderPICTFORMINFONext(&forminfo_iter);

        if( (query->id.xid != 0) && (query->id.xid != cformat->id.xid) )
        {
            continue;
        }

        if(query->type != cformat->type)
        {
            continue;
        }
        
        if( (query->depth != 0) && (query->depth != cformat->depth) )
        {
            continue;
        }
        
        if( (query->direct.red_mask  != 0)&& (query->direct.red_mask != cformat->direct.red_mask))
        {
            continue;
        }
        
        if( (query->direct.green_mask != 0) && (query->direct.green_mask != cformat->direct.green_mask))
        {
            continue;
        }
        
        if( (query->direct.blue_mask != 0) && (query->direct.blue_mask != cformat->direct.blue_mask))
        {
            continue;
        }
        
        if( (query->direct.alpha_mask != 0) && (query->direct.alpha_mask != cformat->direct.alpha_mask))
        {
            continue;
        }
        
        /* This point will only be reached if the pict format   *
         * matches what the user specified                      */
        return cformat; 
    }
    
    return NULL;
}

int main(int argc, char *argv[])
{
    XCBRenderQueryVersionCookie version_cookie;
    XCBRenderQueryVersionRep    *version_reply;
    XCBRenderQueryPictFormatsCookie formats_cookie;
    XCBRenderQueryPictFormatsRep *formats_reply;
    XCBRenderPICTFORMAT  rootformat;
    XCBSCREEN *root;
    int screen_num;
    
    XCBRenderPICTFORMINFO  forminfo_query, *forminfo_result;
    
    c = XCBConnect(0, &screen_num);
    root = XCBAuxGetScreen(c, screen_num);
    
    version_cookie = XCBRenderQueryVersion(c, (CARD32)0, (CARD32)3);
    version_reply = XCBRenderQueryVersionReply(c, version_cookie, 0);

    print_version_info(version_reply);
    
    formats_cookie = XCBRenderQueryPictFormats(c);
    formats_reply = XCBRenderQueryPictFormatsReply(c, formats_cookie, 0);

    draw_window(c, formats_reply);
    
    print_formats_info(formats_reply);
   
    forminfo_query.id.xid = 0;
    forminfo_query.type = XCBRenderPictTypeDirect;
    forminfo_query.depth = 8;
    forminfo_query.direct.red_mask = 0;
    forminfo_query.direct.green_mask = 0;
    forminfo_query.direct.blue_mask = 0;
    forminfo_query.direct.alpha_mask = 255;
    
    forminfo_result = get_pictforminfo(formats_reply, &forminfo_query);
    fprintf(stdout, "\n***** found PICTFORMAT:  %ld *****\n",
            forminfo_result->id.xid);
    rootformat = get_pictformat_from_visual(formats_reply, root->root_visual);
    fprintf(stdout, "\n***** found root PICTFORMAT:   %ld *****\n", rootformat.xid);
   
#if 0
    draw_window(c, formats_reply);
#endif
    
    /* It's very important to free the replys. We don't want memory leaks. */
    free(version_reply);
    free(formats_reply);

    XCBDisconnect(c);

    exit(0);
}
