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
#include <time.h>
#include <assert.h>
#include <string.h>

#include <X11/X.h>
#include <X11/XCB/xcb.h>
#include <X11/XCB/shm.h>
#include <X11/XCB/xcb_aux.h>
#include <X11/XCB/xcb_image.h>
#include <X11/XCB/xcb_icccm.h>

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
    XCBConnection *c;
    XCBDRAWABLE    draw;
    XCBDRAWABLE    pixmap;
    XCBCOLORMAP    cmap;
    CARD8          depth;
    XCBGCONTEXT    gc;
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

static void title_set (flame *f, const char *title);
static int  ilog2 (unsigned int n);
static void flame_set_palette (flame *f);
static void flame_set_flame_zero (flame *f);
static void flame_set_random_flame_base (flame *f);
static void flame_modify_flame_base (flame *f);
static void flame_process_flame (flame *f);
static void flame_draw_flame (flame *f);

flame *
flame_init ()
{
  flame       *f;
  XCBSCREEN   *screen;
  XCBGCONTEXT  gc = { 0 };
  int          screen_nbr;
  CARD32       mask;
  CARD32       values[2];
  int          size;
  int          flame_width;
  int          flame_height;
  XCBRECTANGLE rect_coord = { 0, 0, BG_W, BG_H};

  f = (flame *)malloc (sizeof (flame));
  if (!f)
    return NULL;

  f->xcb.c = XCBConnect (NULL, &screen_nbr);
  if (!f->xcb.c)
    {
      free (f);
      return NULL;
    }
  screen = XCBAuxGetScreen (f->xcb.c, screen_nbr);

  f->xcb.draw.window = screen->root;
  f->xcb.gc = XCBGCONTEXTNew (f->xcb.c);
  mask = GCForeground | GCGraphicsExposures;
  values[0] = screen->black_pixel;
  values[1] = 0; /* no graphics exposures */
  XCBCreateGC (f->xcb.c, f->xcb.gc, f->xcb.draw, mask, values);

  gc = XCBGCONTEXTNew (f->xcb.c);
  mask = GCForeground | GCGraphicsExposures;
  values[0] = screen->white_pixel;
  values[1] = 0; /* no graphics exposures */
  XCBCreateGC (f->xcb.c, gc, f->xcb.draw, mask, values);

  f->xcb.depth = XCBAuxGetDepth (f->xcb.c, screen);
  mask = XCBCWBackPixel | XCBCWEventMask;
  values[0] = screen->white_pixel;
  values[1] = ExposureMask | ButtonPressMask;
  f->xcb.draw.window = XCBWINDOWNew (f->xcb.c);
  XCBCreateWindow (f->xcb.c, f->xcb.depth,
		   f->xcb.draw.window,
		   screen->root,
		   0, 0, BG_W, BG_H,
		   0,
		   InputOutput,
		   screen->root_visual,
		   mask, values);
  title_set (f, "XCB Flames");
  
  f->xcb.pixmap.pixmap = XCBPIXMAPNew (f->xcb.c);
  XCBCreatePixmap (f->xcb.c, f->xcb.depth,
		   f->xcb.pixmap.pixmap, f->xcb.draw,
		   BG_W, BG_H);
  XCBPolyFillRectangle(f->xcb.c, f->xcb.pixmap, gc, 1, &rect_coord);

  XCBMapWindow (f->xcb.c, f->xcb.draw.window);
  XCBSync (f->xcb.c, 0);

  f->xcb.cmap = XCBCOLORMAPNew (f->xcb.c);
  XCBCreateColormap (f->xcb.c,
		     AllocNone,
		     f->xcb.cmap,
		     f->xcb.draw.window,
		     screen->root_visual);

  /* Allocation of the flame arrays */
  flame_width  = BG_W >> 1;
  flame_height = BG_H >> 1;
  f->ws = ilog2 (flame_width);
  size = (1 << f->ws) * flame_height * sizeof (unsigned int);
  f->flame = (unsigned int *)malloc (size);
  if (! f->flame)
    {
      XCBDisconnect (f->xcb.c);
      free (f);
      return NULL;
    }
  f->flame2 = (unsigned int *)malloc (size);
  if (! f->flame2)
    {
      free (f->flame);
      XCBDisconnect (f->xcb.c);
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
  XCBDisconnect (f->xcb.c);
  free (f);
}

int
main ()
{
  flame *f;
  XCBGenericEvent *e;
  XCBGCONTEXT gc = { 0 };

  f = flame_init ();
  if (!f)
    {
      printf ("Can't initialize global data\nExiting...\n");
      return -1;
    }

  flame_set_flame_zero (f);
  flame_set_random_flame_base (f);

  while (1)
    {
      if ((e = XCBPollForEvent (f->xcb.c, NULL)))
	{
	  switch (e->response_type)
	    {
	    case XCBExpose:
	      XCBCopyArea(f->xcb.c, f->xcb.pixmap, f->xcb.draw, gc,
		          0, 0, 0, 0, BG_W, BG_H);
	      XCBSync (f->xcb.c, 0);
	      break;
            case XCBButtonPress:
              printf ("Exiting...\n");
              free (e);
              goto sortie;
	    }
	  free (e);
        }
      flame_draw_flame (f);
      XCBSync (f->xcb.c, 0);
    }

 sortie:
  flame_shutdown (f);

  return 0;
}

static void title_set (flame *f, const char *title)
{
  XCBInternAtomRep *rep;
  XCBATOM           encoding;
  char             *atom_name;

  /* encoding */
  atom_name = "UTF8_STRING";
  rep = XCBInternAtomReply (f->xcb.c,
                            XCBInternAtom (f->xcb.c,
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
  rep = XCBInternAtomReply (f->xcb.c,
                            XCBInternAtom (f->xcb.c,
                                           0,
                                           strlen (atom_name),
                                           atom_name),
                            NULL);
  XCBChangeProperty(f->xcb.c, PropModeReplace,
                    f->xcb.draw.window,
                    rep->atom, encoding, 8, strlen (title), title);
  free (rep);
}

static void
flame_draw_flame (flame *f)
{
  XCBImage     *image;
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

  /* modify the base of the flame */
  flame_modify_flame_base (f);
  /* process the flame array, propagating the flames up the array */
  flame_process_flame (f);

  image = XCBImageGet (f->xcb.c, f->xcb.draw,
		       0, 0, BG_W, BG_H,
		       ((unsigned long)~0L), ZPixmap);
  f->im = (unsigned int *)image->data;

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

          
          XCBImagePutPixel (image,
                            xx, yy,
                            f->pal[cl]);
          XCBImagePutPixel (image,
                            xx + 1, yy,
                            f->pal[((cl1+cl2) >> 1)]);
          XCBImagePutPixel (image,
                            xx, yy + 1,
                            f->pal[((cl1 + cl3) >> 1)]);
          XCBImagePutPixel (image,
                            xx + 1, yy + 1,
                            f->pal[((cl1 + cl4) >> 1)]);
	}
    }
  XCBImagePut (f->xcb.c, f->xcb.draw, f->xcb.gc, image,
	       0, 0, 0, 0, BG_W, BG_H);
  XCBImageDestroy (image);
}

/* set the flame palette */
static void
flame_set_palette (flame *f)
{
  XCBAllocColorCookie cookies[IMAX];
  XCBAllocColorRep *rep;
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

      cookies[i] = XCBAllocColor (f->xcb.c, f->xcb.cmap,
                                  r << 8, g << 8, b << 8);
    }

  for (i = 0 ; i < IMAX ; i++)
    {
      rep = XCBAllocColorReply (f->xcb.c, cookies[i], NULL);
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
