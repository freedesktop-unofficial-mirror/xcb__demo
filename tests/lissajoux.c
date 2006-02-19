#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/XCB/xcb.h>
#include <X11/XCB/shm.h>
#include <X11/XCB/xcb_aux.h>
#include <X11/XCB/xcb_image.h>

#include "lissajoux.h"

#define W_W 100
#define W_H 100

double time_start;
int    loop_count;
double t_previous;
double t;
int    do_shm = 0;

XCBShmSegmentInfo shminfo;

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
      i = XCBImageSHMGet (datap->conn, datap->draw,
			  datap->image, shminfo,
			  0, 0,
			  AllPlanes);
      assert(i);
    }
  else
    {
      datap->image = XCBImageGet (datap->conn, datap->draw,
                                  0, 0, W_W, W_H,
                                  AllPlanes, datap->format);
      assert(datap->image);
    }
  
  pi = 3.1415926535897;
  period = 2.0 * pi;
  a1 = 2.0;
  a2 = 3.0;
  p1 = 4.0*t_previous*pi*0.05;
  p2 = 0.0;

  nbr = 1000;
  for (i = 0 ; i < nbr ; i++)
      {
	x = cos (a1*i*period/nbr + p1);
	y = sin (a2*i*period/nbr + p2);
	XCBImagePutPixel (datap->image,
			  (int)((double)(W_W-5)*(x+1)/2.0),
			  (int)((double)(W_H-5)*(y+1)/2.0), 65535);
      }

  p1 = 4.0*t*pi*0.05;
  p2 = 0.0;

  for (i = 0 ; i < nbr ; i++)
      {
	x = cos (a1*i*period/nbr + p1);
	y = sin (a2*i*period/nbr + p2);
	XCBImagePutPixel (datap->image,
			  (int)((double)(W_W-5)*(x+1)/2.0),
			  (int)((double)(W_H-5)*(y+1)/2.0), 0);
      }

  if (do_shm)
    XCBImageSHMPut (datap->conn, datap->draw, datap->gc,
		    datap->image, shminfo,
		    0, 0, 0, 0, W_W, W_H, 0);
  else
    {
      XCBImagePut (datap->conn, datap->draw, datap->gc, datap->image,
                   0, 0, 0, 0, W_W, W_H);
      XCBImageDestroy (datap->image);
    }
}

void
step (Data *datap)
{
  loop_count++;
  t = get_time () - time_start;

  if (t <= 20.0)
    {
      draw_lissajoux (datap);
    }
  else
    {
      printf("FRAME COUNT..: %i frames\n", loop_count);
      printf("TIME.........: %3.3f seconds\n", t);
      printf("AVERAGE FPS..: %3.3f fps\n", (double)loop_count / t);
      XCBDisconnect (datap->conn);
      exit(0);
    }
}

/* Return 0 if shm is not availaible, 1 otherwise */
void
shm_test (Data *datap)
{
  XCBShmQueryVersionRep *rep;

  rep = XCBShmQueryVersionReply (datap->conn,
				 XCBShmQueryVersion (datap->conn),
				 NULL);
  if (rep)
    {
      CARD8 format;
      int shmctl_status;
      
      if (rep->shared_pixmaps && 
	  (rep->major_version > 1 || rep->minor_version > 0))
	format = rep->pixmap_format;
      else
	format = 0;
      datap->image = XCBImageSHMCreate (datap->conn, datap->depth,
				      format, NULL, W_W, W_H);
      assert(datap->image);

      shminfo.shmid = shmget (IPC_PRIVATE,
			      datap->image->bytes_per_line*datap->image->height,
			      IPC_CREAT | 0777);
      assert(shminfo.shmid != -1);
      shminfo.shmaddr = shmat (shminfo.shmid, 0, 0);
      assert(shminfo.shmaddr);
      datap->image->data = shminfo.shmaddr;

      shminfo.shmseg = XCBShmSEGNew (datap->conn);
      XCBShmAttach (datap->conn, shminfo.shmseg,
		    shminfo.shmid, 0);
      shmctl_status = shmctl(shminfo.shmid, IPC_RMID, 0);
      assert(shmctl_status != -1);
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
  XCBSCREEN       *screen;
  XCBDRAWABLE      win;
  XCBDRAWABLE      rect;
  XCBGCONTEXT      bgcolor;
  CARD32           mask;
  CARD32           valgc[2];
  CARD32           valwin[3];
  XCBRECTANGLE     rect_coord = { 0, 0, W_W, W_H};
  XCBGenericEvent *e;
  int              try_shm;
  int              screen_num;
  
  try_shm = 0;

  /* Arguments test */
  if (argc < 2)
    {
      printf ("Usage: lissajoux try_shm\n");
      printf ("         try_shm == 0: shm not used\n");
      printf ("         try_shm != 0: shm is used (if availaible)\n");
      exit (0);
    }
  if (argc >= 2)
    try_shm = atoi (argv[1]);
  if (try_shm != 0)
    try_shm = 1;

  data.conn = XCBConnect (0, &screen_num);
  screen = XCBAuxGetScreen(data.conn, screen_num);
  data.depth = XCBAuxGetDepth (data.conn, screen);

  win.window = screen->root;

  data.gc = XCBGCONTEXTNew (data.conn);
  mask = GCForeground | GCGraphicsExposures;
  valgc[0] = screen->black_pixel;
  valgc[1] = 0; /* no graphics exposures */
  XCBCreateGC (data.conn, data.gc, win, mask, valgc);

  bgcolor = XCBGCONTEXTNew (data.conn);
  mask = GCForeground | GCGraphicsExposures;
  valgc[0] = screen->white_pixel;
  valgc[1] = 0; /* no graphics exposures */
  XCBCreateGC (data.conn, bgcolor, win, mask, valgc);

  data.draw.window = XCBWINDOWNew (data.conn);
  mask = XCBCWBackPixel | XCBCWEventMask | XCBCWDontPropagate;
  valwin[0] = screen->white_pixel;
  valwin[1] = KeyPressMask | ButtonReleaseMask | ExposureMask;
  valwin[2] = ButtonPressMask;
  XCBCreateWindow (data.conn, 0,
		   data.draw.window,
		   screen->root,
		   0, 0, W_W, W_H,
		   10,
		   InputOutput,
		   screen->root_visual,
		   mask, valwin);
  XCBMapWindow (data.conn, data.draw.window);

  rect.pixmap = XCBPIXMAPNew (data.conn);
  XCBCreatePixmap (data.conn, data.depth,
		   rect.pixmap, data.draw,
		   W_W, W_H);
  XCBPolyFillRectangle(data.conn, rect, bgcolor, 1, &rect_coord);

  data.format = ZPixmap;
  XCBSync (data.conn, 0); 

  if (try_shm)
    shm_test (&data);

  time_start = get_time ();
  t_previous = 0.0;
  while (1)
    {
      e = XCBPollForEvent(data.conn, NULL);
      if (e)
	{
	  switch (e->response_type)
	    {
	    case XCBExpose:
	      XCBCopyArea(data.conn, rect, data.draw, bgcolor,
		          0, 0, 0, 0, W_W, W_H);
	      XCBSync (data.conn, 0);
	      break;
	    }
	  free (e);
        }
      step (&data);
      XCBFlush (data.conn);
      t_previous = t;
    }
  /*NOTREACHED*/
}
