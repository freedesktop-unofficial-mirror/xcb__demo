/*****************************************************************************/
/*                                  XCB Flame                                */
/*****************************************************************************/
/* Originally By:                                                            */
/*     The Rasterman (Carsten Haitzler)                                      */
/*      Copyright (C) 1996                                                   */
/*****************************************************************************/
/* XcB port:                                                                 */
/*     d'Oursse (Vincent TORRI), 2006                                        */
/*****************************************************************************/
/* This code is Freeware. You may copy it, modify it or do with it as you    */
/* please, but you may not claim copyright on any code wholly or partly      */
/* based on this code. I accept no responisbility for any consequences of    */
/* using this code, be they proper or otherwise.                             */
/*****************************************************************************/
/* Okay, now all the legal mumbo-jumbo is out of the way, I will just say    */
/* this: enjoy this program, do with it as you please and watch out for more */
/* code releases from The Rasterman running under X... the only way to code. */
/*****************************************************************************/

/* standard library */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_image.h>

/* Needed for xcb_set_wm_protocols() */
#include <xcb/xcb_icccm.h>

#define XCB_ALL_PLANES ~0
#include <xcb/xcb_icccm.h>

/* some defines for the flame */
#define HSPREAD 26
#define VSPREAD 78
#define VARIANCE 5
#define VARTREND 2
#define RESIDUAL 68

/* default width and height of the window */
#define BG_W 256
#define BG_H 256

#define IMAX 300

typedef struct
{
  struct
  {
    xcb_connection_t *c;
    xcb_drawable_t    draw;
    xcb_drawable_t    pixmap;
    xcb_colormap_t    cmap;
    uint8_t          depth;
    xcb_gcontext_t    gc;
  }xcb;
  
  /* the palette */
  unsigned int  pal[IMAX];
  unsigned int *im;
  int           ims;
  
  /* the flame arrays */
  int           ws;
  unsigned int *flame;
  unsigned int *flame2;
  
}flame;

static xcb_atom_t get_atom (xcb_connection_t *connection, const char *atomName);
static void title_set (flame *f, const char *title);
static int  ilog2 (unsigned int n);
static void flame_set_palette (flame *f);
static void flame_set_flame_zero (flame *f);
static void flame_set_random_flame_base (flame *f);
static void flame_modify_flame_base (flame *f);
static void flame_process_flame (flame *f);
static void flame_draw_flame (flame *f);

xcb_atom_t
get_atom (xcb_connection_t *connection, const char *atomName)
{
  if (atomName == NULL)
    return XCB_NONE;
  xcb_atom_t atom = XCB_NONE;
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection,
	xcb_intern_atom(connection, 0, strlen(atomName), atomName), NULL);
  if (reply)
    {
      atom = reply->atom;
      free(reply);
    }
  return atom;
}

flame *
flame_init ()
{
  flame       *f;
  xcb_screen_t   *screen;
  xcb_gcontext_t  gc = { 0 };
  int          screen_nbr;
  uint32_t       mask;
  uint32_t       values[2];
  int          size;
  int          flame_width;
  int          flame_height;
  xcb_rectangle_t rect_coord = { 0, 0, BG_W, BG_H};

  f = (flame *)malloc (sizeof (flame));
  if (!f)
    return NULL;

  f->xcb.c = xcb_connect (NULL, &screen_nbr);
  if (!f->xcb.c)
    {
      free (f);
      return NULL;
    }
  screen = xcb_aux_get_screen (f->xcb.c, screen_nbr);

  f->xcb.draw = screen->root;
  f->xcb.gc = xcb_generate_id (f->xcb.c);
  mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  values[0] = screen->black_pixel;
  values[1] = 0; /* no graphics exposures */
  xcb_create_gc (f->xcb.c, f->xcb.gc, f->xcb.draw, mask, values);

  gc = xcb_generate_id (f->xcb.c);
  mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  values[0] = screen->white_pixel;
  values[1] = 0; /* no graphics exposures */
  xcb_create_gc (f->xcb.c, gc, f->xcb.draw, mask, values);

  f->xcb.depth = xcb_aux_get_depth (f->xcb.c, screen);
  mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  values[0] = screen->white_pixel;
  values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS;
  f->xcb.draw = xcb_generate_id (f->xcb.c);
  xcb_create_window (f->xcb.c, f->xcb.depth,
		   f->xcb.draw,
		   screen->root,
		   0, 0, BG_W, BG_H,
		   0,
		   XCB_WINDOW_CLASS_INPUT_OUTPUT,
		   screen->root_visual,
		   mask, values);
  title_set (f, "XCB Flames");
  
  f->xcb.pixmap = xcb_generate_id (f->xcb.c);
  xcb_create_pixmap (f->xcb.c, f->xcb.depth,
		   f->xcb.pixmap, f->xcb.draw,
		   BG_W, BG_H);
  xcb_poly_fill_rectangle(f->xcb.c, f->xcb.pixmap, gc, 1, &rect_coord);

  xcb_map_window (f->xcb.c, f->xcb.draw);
  xcb_flush (f->xcb.c);

  f->xcb.cmap = xcb_generate_id (f->xcb.c);
  xcb_create_colormap (f->xcb.c,
		     XCB_COLORMAP_ALLOC_NONE,
		     f->xcb.cmap,
		     f->xcb.draw,
		     screen->root_visual);

  /* Allocation of the flame arrays */
  flame_width  = BG_W >> 1;
  flame_height = BG_H >> 1;
  f->ws = ilog2 (flame_width);
  size = (1 << f->ws) * flame_height * sizeof (unsigned int);
  f->flame = (unsigned int *)malloc (size);
  if (! f->flame)
    {
      xcb_disconnect (f->xcb.c);
      free (f);
      return NULL;
    }
  f->flame2 = (unsigned int *)malloc (size);
  if (! f->flame2)
    {
      free (f->flame);
      xcb_disconnect (f->xcb.c);
      free (f);
      return NULL;
    }

  /* allocation of the image */
  f->ims = ilog2 (BG_W);

  /* initialization of the palette */
  flame_set_palette (f);

  return f;
}

void
flame_shutdown (flame *f)
{
  if (!f)
    return;

  free (f->flame2);
  free (f->flame);
  xcb_disconnect (f->xcb.c);
  free (f);
}

int
main ()
{
  flame *f;
  xcb_generic_event_t *e;
  xcb_gcontext_t gc = { 0 };

  f = flame_init ();
  if (!f)
    {
      printf ("Can't initialize global data\nExiting...\n");
      return -1;
    }

  flame_set_flame_zero (f);
  flame_set_random_flame_base (f);

  xcb_atom_t deleteWindowAtom = get_atom(f->xcb.c, "WM_DELETE_WINDOW");
  xcb_atom_t wmprotocolsAtom = get_atom(f->xcb.c, "WM_PROTOCOLS");
  /* Listen to X client messages in order to be able to pickup
     the "delete window" message that is generated for example
     when someone clicks the top-right X button within the window
     manager decoration (or when user hits ALT-F4). */
  xcb_set_wm_protocols (f->xcb.c, wmprotocolsAtom, f->xcb.draw, 1, &deleteWindowAtom);

  bool finished = false;
  while (!finished)
    {
      if ((e = xcb_poll_for_event (f->xcb.c)))
	{
	  switch (e->response_type & 0x7f)
	    {
	    case XCB_EXPOSE:
	      xcb_copy_area(f->xcb.c, f->xcb.pixmap, f->xcb.draw, gc,
		          0, 0, 0, 0, BG_W, BG_H);
	      xcb_flush (f->xcb.c);
	      break;
	    case XCB_CLIENT_MESSAGE:
	      if (((xcb_client_message_event_t *)e)->data.data32[0] == deleteWindowAtom)
	        {
	          finished = true;
	        }
	      break;
	    case XCB_BUTTON_PRESS:
	      finished = true;
	      break;
	    }
	  free (e);
        }
      flame_draw_flame (f);
      xcb_flush (f->xcb.c);
    }

  flame_shutdown (f);

  return 0;
}

static void title_set (flame *f, const char *title)
{
  xcb_intern_atom_reply_t *rep;
  xcb_atom_t           encoding;
  char             *atom_name;

  /* encoding */
  atom_name = "UTF8_STRING";
  rep = xcb_intern_atom_reply (f->xcb.c,
                            xcb_intern_atom (f->xcb.c,
                                           0,
                                           strlen (atom_name),
                                           atom_name),
                            NULL);
  encoding = rep->atom;
  free (rep);

  /* ICCCM */
/*   SetWMName (f->xcb.c, f->xcb.draw.window, encoding, strlen (title), title); */

  /* NETWM */
  atom_name = "_NET_WM_NAME";
  rep = xcb_intern_atom_reply (f->xcb.c,
                            xcb_intern_atom (f->xcb.c,
                                           0,
                                           strlen (atom_name),
                                           atom_name),
                            NULL);
  xcb_change_property(f->xcb.c, XCB_PROP_MODE_REPLACE,
                    f->xcb.draw,
                    rep->atom, encoding, 8, strlen (title), title);
  free (rep);
}

static void
flame_draw_flame (flame *f)
{
  xcb_image_t     *image;
  unsigned int *ptr;
  int           x;
  int           y;
  int           xx;
  int           yy;
  int           cl;
  int           cl1;
  int           cl2;
  int           cl3;
  int           cl4;

  image = xcb_image_get (f->xcb.c, f->xcb.draw,
		       0, 0, BG_W, BG_H,
		       XCB_ALL_PLANES, XCB_IMAGE_FORMAT_Z_PIXMAP);
  /* If the top-level window is minimized (iconic) then the xcb_image_get()
     will return NULL. In this case, we'll skip both updating and drawing
     the flame, and we will also do a small sleep so that the program doesn't
     hog as much CPU while minimized. 
   
     Another (non-polling == cleaner) way to not hog the CPU while minimized 
     would be to pass the XCB_EVENT_MASK_STRUCTURE_NOTIFY flag to
     xcb_create_window(). This will give you XCB_UNMAP_NOTIFY and
     XCB_MAP_NOTIFY events whenever the window is "minimized" (made iconic)
     and "unminimized" (made normal again). This information would then be
     used to make the main loop use the xcb_wait_for_event() instead of
     xcb_poll_for_event() while the window is minimized (iconic).     */
  if (image == NULL)
    {
      usleep (100000);
      return;
    }
  f->im = (unsigned int *)image->data;

  /* modify the base of the flame */
  flame_modify_flame_base (f);
  /* process the flame array, propagating the flames up the array */
  flame_process_flame (f);

  for (y = 0 ; y < ((BG_H >> 1) - 1) ; y++)
    {
      for (x = 0 ; x < ((BG_W >> 1) - 1) ; x++)
	{
	  xx = x << 1;
	  yy = y << 1;

	  ptr = f->flame2 + (y << f->ws) + x;
	  cl1 = cl = (int)*ptr;
	  ptr = f->flame2 + (y << f->ws) + x + 1;
	  cl2 = (int)*ptr;
	  ptr = f->flame2 + ((y + 1) << f->ws) + x + 1;
	  cl3 = (int)*ptr;
	  ptr = f->flame2 + ((y + 1) << f->ws) + x;
	  cl4 = (int)*ptr;

          
          xcb_image_put_pixel (image,
                            xx, yy,
                            f->pal[cl]);
          xcb_image_put_pixel (image,
                            xx + 1, yy,
                            f->pal[((cl1+cl2) >> 1)]);
          xcb_image_put_pixel (image,
                            xx, yy + 1,
                            f->pal[((cl1 + cl3) >> 1)]);
          xcb_image_put_pixel (image,
                            xx + 1, yy + 1,
                            f->pal[((cl1 + cl4) >> 1)]);
	}
    }
  xcb_image_put (f->xcb.c, f->xcb.draw, f->xcb.gc, image,
	       0, 0, 0);
  xcb_image_destroy (image);
}

/* set the flame palette */
static void
flame_set_palette (flame *f)
{
  xcb_alloc_color_cookie_t cookies[IMAX];
  xcb_alloc_color_reply_t *rep;
  int               i;
  int               r;
  int               g;
  int               b;

  for (i = 0 ; i < IMAX ; i++)
    {
      r = i * 3;
      g = (i - 80) * 3;
      b = (i - 160) * 3;

      if (r < 0)   r = 0;
      if (r > 255) r = 255;
      if (g < 0)   g = 0;
      if (g > 255) g = 255;
      if (b < 0)   b = 0;
      if (b > 255) b = 255;

      cookies[i] = xcb_alloc_color (f->xcb.c, f->xcb.cmap,
                                  r << 8, g << 8, b << 8);
    }

  for (i = 0 ; i < IMAX ; i++)
    {
      rep = xcb_alloc_color_reply (f->xcb.c, cookies[i], NULL);
      f->pal[i] = rep->pixel;
      free (rep);
    }
}

/* set the flame array to zero */
static void
flame_set_flame_zero (flame *f)
{
  int           x;
  int           y;
  unsigned int *ptr;

  for (y = 0 ; y < (BG_H >> 1) ; y++)
    {
      for (x = 0 ; x < (BG_W >> 1) ; x++)
	{
	  ptr = f->flame + (y << f->ws) + x;
	  *ptr = 0;
	}
    }
}

static void
flame_set_random_flame_base (flame *f)
{
  int           x;
  int           y;
  unsigned int *ptr;
  
  /* initialize a random number seed from the time, so we get random */
  /* numbers each time */
  srand (time(NULL));
  y = (BG_H >> 1) - 1;
  for (x = 0 ; x < (BG_W >> 1) ; x++)
    {
      ptr = f->flame + (y << f->ws) + x;
      *ptr = rand ()%IMAX;
    }
}

/* modify the base of the flame with random values */
static void
flame_modify_flame_base (flame *f)
{
  int           x;
  int           y;
  unsigned int *ptr;
  int           val;
  
  y = (BG_H >> 1) - 1;
  for (x = 0 ; x < (BG_W >> 1) ; x++)
    {
      ptr = f->flame + (y << f->ws) + x;
      *ptr += ((rand ()%VARIANCE) - VARTREND);
      val = *ptr;
      if (val > IMAX) *ptr = 0;
      if (val < 0)    *ptr = 0;
    }
}

/* process entire flame array */
static void
flame_process_flame (flame *f)
{
  int           x;
  int           y;
  unsigned int *ptr;
  unsigned int *p;
  unsigned int  val;
  unsigned int  tmp;
  
  for (y = ((BG_H >> 1) - 1) ; y >= 2 ; y--)
    {
      for (x = 1 ; x < ((BG_W >> 1) - 1) ; x++)
	{
	  ptr = f->flame + (y << f->ws) + x;
	  val = (unsigned int)*ptr;
	  if (val > IMAX)
	    *ptr = (unsigned int)IMAX;
	  val = (unsigned int)*ptr;
	  if (val > 0)
	    {
	      tmp = (val * VSPREAD) >> 8;
	      p   = ptr - (2 << f->ws);
	      *p  = *p + (tmp >> 1);
	      p   = ptr - (1 << f->ws);
	      *p  = *p + tmp;
	      tmp = (val * HSPREAD) >> 8;
	      p   = ptr - (1 << f->ws) - 1;
	      *p  = *p + tmp;
	      p   = ptr - (1 << f->ws) + 1;
	      *p  = *p + tmp;
	      p   = ptr - 1;
	      *p  = *p + (tmp >>1 );
	      p   = ptr + 1;
	      *p  = *p + (tmp >> 1);
	      p   = f->flame2 + (y << f->ws) + x;
	      *p  = val;
	      if (y < ((BG_H >> 1) - 1))
		*ptr = (val * RESIDUAL) >> 8;
	    }
	}
    }
}

static int
ilog2 (unsigned int n)
{
  int p = -1;

  assert(n > 0);
  while (n > 0) {
    p++;
    n >>= 1;
  }

  return p;
}
