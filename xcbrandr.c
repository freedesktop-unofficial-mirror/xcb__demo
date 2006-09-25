/*
 * $XFree86: xc/programs/xrandr/xrandr.c,v 1.11 2002/10/14 18:01:43 keithp Exp $
 *
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
 * Copyright © 2002 Hewlett Pacard Company, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard or HP not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard and HP makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD and HP DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Blame Jim Gettys for any bugs; he wrote most of the client side code,
 * and part of the server code for randr.
 *
 * Ported to XCB by Jeremy Kolb 2/8/2005
 */

#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/randr.h>
#include <xcb/render.h>	/* we share subpixel information */
#include <string.h>
#include <stdlib.h>

#define CurrentTime 0L /* special time apparently*/

static char *program_name;

static char *direction[5] = {
  "normal", 
  "left", 
  "inverted", 
  "right",
  "\n"};

/* subpixel order */
static char *order[6] = {
  "unknown",
  "horizontal rgb",
  "horizontal bgr",
  "vertical rgb",
  "vertical bgr",
  "no subpixels"};


static void
usage(void)
{
  fprintf(stderr, "usage: %s [options]\n", program_name);
  fprintf(stderr, "  where options are:\n");
  fprintf(stderr, "  -display <display> or -d <display>\n");
  fprintf(stderr, "  -help\n");
  fprintf(stderr, "  -o <normal,inverted,left,right,0,1,2,3>\n");
  fprintf(stderr, "            or --orientation <normal,inverted,left,right,0,1,2,3>\n");
  fprintf(stderr, "  -q        or --query\n");
  fprintf(stderr, "  -s <size>/<width>x<height> or --size <size>/<width>x<height>\n");
  fprintf(stderr, "  -r <rate> or --rate <rate>\n");
  fprintf(stderr, "  -v        or --version\n");
  fprintf(stderr, "  -x        (reflect in x)\n");
  fprintf(stderr, "  -y        (reflect in y)\n");
  fprintf(stderr, "  --screen <screen>\n");
  fprintf(stderr, "  --verbose\n");
  
  exit(1);
  /*NOTREACHED*/
}

/*
 * Same idea as xc/lib/Xrandr.c (XRRConfigRates).
 * Returns the rates for a given screen.
 * Would be nice to put in another library or something.
 */
short*
ConfigRates(xcb_randr_get_screen_info_reply_t *config, int sizeID, int *nrates)
{
    int i = 0;
    short *ents;
    xcb_randr_refresh_rates_iterator_t ri = xcb_randr_get_screen_info_rates_iterator(config);
    
    while (i++ < sizeID) {
	xcb_randr_refresh_rates_next(&ri);
    }
    
    ents = (short *)xcb_randr_refresh_rates_rates(ri.data);
    *nrates = xcb_randr_refresh_rates_rates_length(ri.data);
    
    if (!nrates) {
	*nrates = 0;
	return 0;
    }
    
    return ents;
}

int
main (int argc, char **argv)
{
  xcb_connection_t  *c;
  xcb_randr_screen_size_t *sizes;
  xcb_randr_get_screen_info_reply_t *sc;
  int		nsize;
  int		nrate;
  short		*rates;
  xcb_screen_t	*root;
  int		status = XCB_RANDR_SET_CONFIG_FAILED;
  int		rot = -1;
  int		verbose = 0, query = 0;
  short		rotation, current_rotation, rotations;
  xcb_generic_event_t *event;
  xcb_randr_screen_change_notify_event_t *sce;
  char          *display_name = NULL;
  int 		i, j;
  int		current_size;
  short		current_rate;
  short		rate = -1;
  short		size = -1;
  int		dirind = 0;
  static int	setit = 0;
  int		screen = -1;
  int		version = 0;
  int		event_base, error_base;
  short		reflection = 0;
  int		width = 0, height = 0;
  int		have_pixel_size = 0;
  xcb_generic_error_t *err;
  uint16_t mask = (uint16_t) XCB_EVENT_MASK_STRUCTURE_NOTIFY;
  uint32_t values[1];
  xcb_randr_get_screen_info_cookie_t scookie;
  int major_version, minor_version;
  xcb_randr_query_version_reply_t *rr_version;

  program_name = argv[0];
  if (argc == 1) query = 1;
  for (i = 1; i < argc; i++) {
    if (!strcmp ("-display", argv[i]) || !strcmp ("-d", argv[i])) {
      if (++i>=argc) usage ();
      display_name = argv[i];
      continue;
    }
    if (!strcmp("-help", argv[i])) {
      usage();
      continue;
    }
    if (!strcmp ("--verbose", argv[i])) {
      verbose = 1;
      continue;
    }

    if (!strcmp ("-s", argv[i]) || !strcmp ("--size", argv[i])) {
      if (++i>=argc) usage ();
      if (sscanf (argv[i], "%dx%d", &width, &height) == 2)
	have_pixel_size = 1;
      else {
	size = atoi (argv[i]);
	if (size < 0) usage();
      }
      setit = 1;
      continue;
    }

    if (!strcmp ("-r", argv[i]) || !strcmp ("--rate", argv[i])) {
      if (++i>=argc) usage ();
      rate = atoi (argv[i]);
      if (rate < 0) usage();
      setit = 1;
      continue;
    }

    if (!strcmp ("-v", argv[i]) || !strcmp ("--version", argv[i])) {
      version = 1;
      continue;
    }

    if (!strcmp ("-x", argv[i])) {
      reflection |= XCB_RANDR_ROTATION_REFLECT_X;
      setit = 1;
      continue;
    }
    if (!strcmp ("-y", argv[i])) {
      reflection |= XCB_RANDR_ROTATION_REFLECT_Y;
      setit = 1;
      continue;
    }
    if (!strcmp ("--screen", argv[i])) {
      if (++i>=argc) usage ();
      screen = atoi (argv[i]);
      if (screen < 0) usage();
      continue;
    }
    if (!strcmp ("-q", argv[i]) || !strcmp ("--query", argv[i])) {
      query = 1;
      continue;
    }
    if (!strcmp ("-o", argv[i]) || !strcmp ("--orientation", argv[i])) {
      char *endptr;
      if (++i>=argc) usage ();
      dirind = strtol(argv[i], &endptr, 0);
      if (*endptr != '\0') {
	for (dirind = 0; dirind < 4; dirind++) {
	  if (strcmp (direction[dirind], argv[i]) == 0) break;
	}
	if ((dirind < 0) || (dirind > 3))  usage();
      }
      rot = dirind;
      setit = 1;
      continue;
    }
    
    usage();
  }
 
  if (verbose) query = 1;

  if (!display_name)
      display_name = getenv("DISPLAY");
  if (!display_name) {
      fprintf (stderr, "No display available\n");
      exit (1);
  }
  c = xcb_connect(display_name, &screen);
  if (!c) {
      fprintf (stderr, "Can't open display %s\n", display_name);
      exit (1);
  }
  root = xcb_aux_get_screen(c, screen);
  rr_version = xcb_randr_query_version_reply(c, xcb_randr_query_version(c, 1, 1), 0);
  if (!rr_version) {
      fprintf(stderr, "Can't get VersionReply.\n");
      exit (1);
  }
  major_version = rr_version->major_version;
  minor_version = rr_version->minor_version;

  scookie = xcb_randr_get_screen_info(c, root->root);
  sc = xcb_randr_get_screen_info_reply(c, scookie, 0);
  if (!sc) {
	fprintf(stderr, "Can't get ScreenInfo.\n");
	exit (1);
  }

  current_rotation = sc->rotation;
  current_size = sc->sizeID;
  
  nsize = sc->nSizes;
  sizes = xcb_randr_get_screen_info_sizes(sc);

  if (have_pixel_size) {
    for (size = 0; size < nsize; size++)
    {
      if (sizes[size].width == width && sizes[size].height == height)
	break;
    }
  }
  else if (size < 0)
    size = current_size;

  if (size >= nsize) usage();

  if (rot < 0)
  {
    for (rot = 0; rot < 4; rot++)
	if (1 << rot == (current_rotation & 0xf))
	    break;
  }

  current_rate = sc->rate;
  
  if (rate < 0)
  {
    if (size == current_size)
	rate = current_rate;
    else
	rate = 0;
  }

  if (version) {
    printf("Server reports RandR version %d.%d\n", 
	   major_version, minor_version);
  }
  
  if (query) {
    printf(" SZ:    Pixels          Physical       Refresh\n");
    for (i = 0; i < nsize; i++) {
      printf ("%c%-2d %5d x %-5d  (%4dmm x%4dmm )",
	   i == current_size ? '*' : ' ',
	   i, sizes[i].width, sizes[i].height,
	   sizes[i].mwidth, sizes[i].mheight);
      rates = ConfigRates (sc, i, &nrate);

      if (nrate) printf ("  ");
      for (j = 0; j < nrate; j++)
	printf ("%c%-4d",
		i == current_size && rates[j] == current_rate ? '*' : ' ',
		rates[j]);
      printf ("\n");
    }
  }

#if 0
  rotations = XRRConfigRotations(sc, &current_rotation);
#else
  rotations = sc->rotation;
#endif

  rotation = 1 << rot ;
  if (query) {
    for (i = 0; i < 4; i ++) {
      if ((current_rotation >> i) & 1) 
	printf("Current rotation - %s\n", direction[i]);
    }

    printf("Current reflection - ");
    if (current_rotation & (XCB_RANDR_ROTATION_REFLECT_X|XCB_RANDR_ROTATION_REFLECT_Y))
    {
	if (current_rotation & XCB_RANDR_ROTATION_REFLECT_X) printf ("X Axis ");
	if (current_rotation & XCB_RANDR_ROTATION_REFLECT_Y) printf ("Y Axis");
    }
    else
	printf ("none");
    printf ("\n");
    

    printf ("Rotations possible - ");
    for (i = 0; i < 4; i ++) {
      if ((rotations >> i) & 1)  printf("%s ", direction[i]);
    }
    printf ("\n");

    printf ("Reflections possible - ");
    if (rotations & (XCB_RANDR_ROTATION_REFLECT_X|XCB_RANDR_ROTATION_REFLECT_Y))
    {
        if (rotations & XCB_RANDR_ROTATION_REFLECT_X) printf ("X Axis ");
	if (rotations & XCB_RANDR_ROTATION_REFLECT_Y) printf ("Y Axis");
    }
    else
	printf ("none");
    printf ("\n");
  }

  if (verbose) { 
    printf("Setting size to %d, rotation to %s\n",  size, direction[rot]);

    printf ("Setting reflection on ");
    if (reflection)
    {
	if (reflection & XCB_RANDR_ROTATION_REFLECT_X) printf ("X Axis ");
	if (reflection & XCB_RANDR_ROTATION_REFLECT_Y) printf ("Y Axis");
    }
    else
	printf ("neither axis");
    printf ("\n");

    if (reflection & XCB_RANDR_ROTATION_REFLECT_X) printf("Setting reflection on X axis\n");

    if (reflection & XCB_RANDR_ROTATION_REFLECT_Y) printf("Setting reflection on Y axis\n");
  }

  /* we should test configureNotify on the root window */
  values[0] = 1;
  xcb_configure_window(c, root->root, mask, values);

  if (setit) xcb_randr_select_input (c, root->root, XCB_RANDR_SM_SCREEN_CHANGE_NOTIFY);

  if (setit) {
    xcb_randr_set_screen_config_cookie_t sscc;
    xcb_randr_set_screen_config_reply_t *config;
    sscc = xcb_randr_set_screen_config(c, root->root, CurrentTime, sc->config_timestamp, size,
	    (short) (rotation | reflection), rate);
    config = xcb_randr_set_screen_config_reply(c, sscc, &err);
    if (!config) {
	fprintf(stderr, "Can't set the screen. Error Code: %i Status:%i\n",
		err->error_code, status);
	exit(1);
    }
    status = config->status;
  }
    
  const xcb_query_extension_reply_t *qrre_rep;
  qrre_rep = xcb_get_extension_data(c, &xcb_randr_id);
  event_base = qrre_rep->first_event;
  error_base = qrre_rep->first_error;
  
  if (verbose && setit) {
    if (status == XCB_RANDR_SET_CONFIG_SUCCESS)
      {
	while (1) {
	int spo;
	event = xcb_wait_for_event(c);
	
	printf ("Event received, type = %d\n", event->response_type);
#if 0
	/*
	 * update Xlib's knowledge of the event
	 * Not sure what the equiv of this is or if we need it.
	 */
	XRRUpdateConfiguration (&event);
#endif
	
	switch (event->response_type - event_base) {
	case XCB_RANDR_SCREEN_CHANGE_NOTIFY:
	  sce = (xcb_randr_screen_change_notify_event_t *) event;

	  printf("Got a screen change notify event!\n");
	  printf(" window = %d\n root = %d\n size_index = %d\n rotation %d\n", 
	       (int) sce->request_window.xid, (int) sce->root.xid, 
	       sce->sizeID,  sce->rotation);
	  printf(" timestamp = %d, config_timestamp = %d\n",
	       sce->timestamp, sce->config_timestamp);
	  printf(" Rotation = %x\n", sce->rotation);
	  printf(" %d X %d pixels, %d X %d mm\n",
		 sce->width, sce->height, sce->mwidth, sce->mheight);
	  
	  printf("Display width   %d, height   %d\n",
		 root->width_in_pixels, root->height_in_pixels);
	  printf("Display widthmm %d, heightmm %d\n", 
		 root->width_in_millimeters, root->height_in_millimeters);
	  
	  spo = sce->subpixel_order;
	  if ((spo < 0) || (spo > 5))
	    printf ("Unknown subpixel order, value = %d\n", spo);
	  else printf ("new Subpixel rendering model is %s\n", order[spo]);
	  break;
	default:
	  if (event->response_type == XCB_CONFIGURE_NOTIFY)
	    printf("Received ConfigureNotify Event!\n");
          else
	    printf("unknown event received, type = %d!\n", event->response_type);
	}
	}
      }
  }
#if 0
  XRRFreeScreenConfigInfo(sc);
#endif
  free(sc);
  free(rr_version);
  xcb_disconnect(c);
  return(0);
}
