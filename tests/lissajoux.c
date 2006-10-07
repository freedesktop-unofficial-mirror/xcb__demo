#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_image.h>

#include "lissajoux.h"

#define W_W 100
#define W_H 100

double time_start;
int    loop_count;
double t_previous;
double t;
int    do_shm = 0;

double tab_cos[3600];
double tab_sin[3600];

xcb_shm_segment_info_t shminfo;

double
get_time(void)
{
  struct timeval timev;
  
  gettimeofday(&timev, NULL);

  return (double)timev.tv_sec + (((double)timev.tv_usec) / 1000000);
}

void
draw_lissajoux (Data *datap)
{
  int       i, nbr;
  double    a1, a2, p1, p2;
  double    pi, period;
  double    x, y;
  
  if (do_shm)
    { 
      i = xcb_image_shm_get (datap->conn, datap->draw,
			  datap->image, shminfo,
			  0, 0,
			  XCB_ALL_PLANES);
      assert(i);
    }
  else
    {
      datap->image = xcb_image_get (datap->conn, datap->draw,
                                  0, 0, W_W, W_H,
                                  XCB_ALL_PLANES, datap->format);
      assert(datap->image);
    }
  
  pi = 3.1415926535897;
  period = 2.0 * pi;
  a1 = 2.0;
  a2 = 3.0;
  p1 = 4.0*t_previous*pi*0.05;
  p2 = 0.0;

  nbr = 10000;
  for (i = 0 ; i < nbr ; i++)
      {
	x = tab_cos[(int)(a1*i + p1*nbr) % 3600];
	y = tab_sin[(int)(a2*i + p2*nbr) % 3600];
	xcb_image_put_pixel (datap->image,
			  (int)((double)(W_W-5)*(x+1)/2.0),
			  (int)((double)(W_H-5)*(y+1)/2.0), 65535);
      }

  p1 = 4.0*t*pi*0.05;
  p2 = 0.0;

  for (i = 0 ; i < nbr ; i++)
      {
	x = tab_cos[(int)(a1*i + p1*nbr) % 3600];
	y = tab_sin[(int)(a2*i + p2*nbr) % 3600];
	xcb_image_put_pixel (datap->image,
			  (int)((double)(W_W-5)*(x+1)/2.0),
			  (int)((double)(W_H-5)*(y+1)/2.0), 0);
      }

  if (do_shm)
    xcb_image_shm_put (datap->conn, datap->draw, datap->gc,
		    datap->image, shminfo,
		    0, 0, 0, 0, W_W, W_H, 0);
  else
    {
      xcb_image_put (datap->conn, datap->draw, datap->gc, datap->image,
                   0, 0, 0, 0, W_W, W_H);
      xcb_image_destroy (datap->image);
    }
}

void
step (Data *datap)
{
  loop_count++;
  t = get_time () - time_start;

  if (t <= 2.0)
    {
      draw_lissajoux (datap);
    }
  else
    {
      printf("FRAME COUNT..: %i frames\n", loop_count);
      printf("TIME.........: %3.3f seconds\n", t);
      printf("AVERAGE FPS..: %3.3f fps\n", (double)loop_count / t);
      if (do_shm)
        xcb_image_shm_destroy (datap->image);
      xcb_disconnect (datap->conn);
      exit(0);
    }
}

/* Return 0 if shm is not available, 1 otherwise */
void
shm_test (Data *datap)
{
  xcb_shm_query_version_reply_t *rep;

  rep = xcb_shm_query_version_reply (datap->conn,
				 xcb_shm_query_version (datap->conn),
				 NULL);
  if (rep)
    {
      uint8_t format;
      int shmctl_status;
      
      if (rep->shared_pixmaps && 
	  (rep->major_version > 1 || rep->minor_version > 0))
	format = rep->pixmap_format;
      else
	format = 0;
      datap->image = xcb_image_shm_create (datap->conn, datap->depth,
                                        format, NULL, W_W, W_H);
      assert(datap->image);

      shminfo.shmid = shmget (IPC_PRIVATE,
			      datap->image->bytes_per_line*datap->image->height,
			      IPC_CREAT | 0777);
      assert(shminfo.shmid != -1);
      shminfo.shmaddr = shmat (shminfo.shmid, 0, 0);
      assert(shminfo.shmaddr);
      datap->image->data = shminfo.shmaddr;

      shminfo.shmseg = xcb_generate_id (datap->conn);
      xcb_shm_attach (datap->conn, shminfo.shmseg,
		    shminfo.shmid, 0);
      shmctl_status = shmctl(shminfo.shmid, IPC_RMID, 0);
      assert(shmctl_status != -1);
      free (rep);
    }

  if (datap->image)
    {
      printf ("Use of shm.\n");
      do_shm = 1;
    }
  else
    {
      printf ("Can't use shm. Use standard functions.\n");
      shmdt (shminfo.shmaddr);		       
      shmctl (shminfo.shmid, IPC_RMID, 0);
      datap->image = NULL;
    }
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
  int              try_shm;
  int              screen_num;
  int              i;
  
  try_shm = 0;

  /* Arguments test */
  if (argc < 2)
    {
      printf ("Usage: lissajoux try_shm\n");
      printf ("         try_shm == 0: shm not used\n");
      printf ("         try_shm != 0: shm is used (if available)\n");
      exit (0);
    }
  if (argc >= 2)
    try_shm = atoi (argv[1]);
  if (try_shm != 0)
    try_shm = 1;

  data.conn = xcb_connect (0, &screen_num);
  screen = xcb_aux_get_screen(data.conn, screen_num);
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
  valwin[1] = XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_EXPOSURE;
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

  data.format = XCB_IMAGE_FORMAT_Z_PIXMAP;
  xcb_flush (data.conn); 

  if (try_shm)
    shm_test (&data);

  for (i = 0; i < 3600; i++) {
    tab_cos[i] = cos (2.0 * 3.1415926535897 * (double)i / 3600.0);
    tab_sin[i] = sin (2.0 * 3.1415926535897 * (double)i / 3600.0);
  }

  time_start = get_time ();
  t_previous = 0.0;
  while (1)
    {
      e = xcb_poll_for_event(data.conn);
      if (e)
	{
	  switch (e->response_type)
	    {
	    case XCB_EXPOSE:
	      xcb_copy_area(data.conn, rect, data.draw, bgcolor,
		          0, 0, 0, 0, W_W, W_H);
	      break;
	    }
	  free (e);
        }
      step (&data);
      xcb_flush (data.conn);
      t_previous = t;
    }
  /*NOTREACHED*/
}
