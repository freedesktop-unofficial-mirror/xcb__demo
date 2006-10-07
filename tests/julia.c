#include <stdlib.h>
#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_image.h>

#include "julia.h"

#define W_W 640
#define W_H 480

/* Parameters of the fractal */

/* double cr = -0.7927; */
/* double ci = 0.1609; */

/* double cr = 0.32; */
/* double ci = 0.043; */

/* double cr = -1.1380; */
/* double ci = -0.2403; */

/* double cr = -0.0986; */
/* double ci = -0.65186; */

/* double cr = -0.1225; */
/* double ci = 0.7449; */

double cr = -0.3380;
double ci = -0.6230;
double origin_x = -1.8;
double origin_y = -1.2;
double width = 3.6;
double height = 2.4;

/* Numbers of colors in the palette */
int cmax = 316;

void
palette_julia (Data *datap)
{
  xcb_alloc_color_reply_t *rep;
  int               i;

  datap->palette = (uint32_t *)malloc (sizeof (uint32_t) * cmax);
  
  for (i = 0 ; i < cmax ; i++)
    {
      if (i < 128)
	rep = xcb_alloc_color_reply (datap->conn,
				  xcb_alloc_color (datap->conn,
						 datap->cmap,
						 i<<9, 0, 0),
				  0);
      else if (i < 255)
	rep = xcb_alloc_color_reply (datap->conn,
				  xcb_alloc_color (datap->conn,
						 datap->cmap,
						 65535, (i-127)<<9, 0),
				  0);
      else
	rep = xcb_alloc_color_reply (datap->conn,
				  xcb_alloc_color (datap->conn,
						 datap->cmap,
						 65535, 65535, (i-255)<<10),
				  0);
      
      if (!rep)
	datap->palette[i] = 0;
      else
	datap->palette[i] = rep->pixel;
      free (rep);
    }
  
}

void
draw_julia (Data *datap)
{
  double    zr, zi, t;
  int       c;
  int       i, j;
  
  datap->image = xcb_image_get (datap->conn, datap->draw,
		       0, 0, W_W, W_H,
		       XCB_ALL_PLANES, datap->format);
  
  for (i = 0 ; i < datap->image->width ; i++)
    for (j = 0 ; j < datap->image->height ; j++)
      {
	zr = origin_x + width * (double)i / (double)datap->image->width;
	zi = origin_y + height * (double)j / (double)datap->image->height;
	c = 0;
	while ((zr*zr + zi*zi < 4.0) &&
	       (c < cmax-1))
	  {
	    t = zr;
	    zr = zr*zr - zi*zi + cr;
	    zi = 2.0*t*zi + ci;
	    c++;
	  }
	xcb_image_put_pixel (datap->image,
			  i,j,
			  datap->palette[c]);
      }

  xcb_image_put (datap->conn, datap->draw, datap->gc, datap->image,
	       0, 0, 0, 0, W_W, W_H);
}

int
main (int argc, char *argv[])
{
  Data             data;
  xcb_screen_t       *screen;
  xcb_drawable_t      win;
  xcb_drawable_t      rect;
  xcb_gcontext_t      bgcolor;
  uint32_t           mask;
  uint32_t           valgc[2];
  uint32_t           valwin[3];
  xcb_rectangle_t     rect_coord = { 0, 0, W_W, W_H};
  xcb_generic_event_t *e;
  int              screen_num;
  
  data.conn = xcb_connect (0, &screen_num);
  screen = xcb_aux_get_screen (data.conn, screen_num);
  data.depth = xcb_aux_get_depth (data.conn, screen);

  win = screen->root;

  data.gc = xcb_generate_id (data.conn);
  mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  valgc[0] = screen->black_pixel;
  valgc[1] = 0; /* no graphics exposures */
  xcb_create_gc (data.conn, data.gc, win, mask, valgc);

  bgcolor = xcb_generate_id (data.conn);
  mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  valgc[0] = screen->white_pixel;
  valgc[1] = 0; /* no graphics exposures */
  xcb_create_gc (data.conn, bgcolor, win, mask, valgc);

  data.draw = xcb_generate_id (data.conn);
  mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_DONT_PROPAGATE;
  valwin[0] = screen->white_pixel;
  valwin[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_EXPOSURE;
  valwin[2] = XCB_EVENT_MASK_BUTTON_PRESS;
  xcb_create_window (data.conn, 0,
		   data.draw,
		   screen->root,
		   0, 0, W_W, W_H,
		   10,
		   XCB_WINDOW_CLASS_INPUT_OUTPUT,
		   screen->root_visual,
		   mask, valwin);
  xcb_map_window (data.conn, data.draw);

  rect = xcb_generate_id (data.conn);
  xcb_create_pixmap (data.conn, data.depth,
		   rect, data.draw,
		   W_W, W_H);
  xcb_poly_fill_rectangle(data.conn, rect, bgcolor, 1, &rect_coord);

  xcb_map_window (data.conn, data.draw);

  data.format = XCB_IMAGE_FORMAT_Z_PIXMAP;

  data.cmap = xcb_generate_id (data.conn);
  xcb_create_colormap (data.conn,
		     XCB_COLORMAP_ALLOC_NONE,
		     data.cmap,
		     data.draw,
		     screen->root_visual);

  palette_julia (&data);

  xcb_flush (data.conn); 

  while ((e = xcb_wait_for_event(data.conn)))
    {
      switch (e->response_type)
	{
	case XCB_EXPOSE:
	  {
	    xcb_copy_area(data.conn, rect, data.draw, bgcolor,
			0, 0, 0, 0, W_W, W_H);
	    draw_julia (&data);
	    xcb_flush (data.conn);
	    break;
	  }
	case XCB_KEY_RELEASE:
	case XCB_BUTTON_RELEASE:
	  {
            if (data.palette)
              free (data.palette);
            if (data.image)
              xcb_image_destroy (data.image);
            free (e);
            xcb_disconnect (data.conn);
            exit (0);
	    break;
	  }
	}
      free (e);
    }

  return 1;
}
