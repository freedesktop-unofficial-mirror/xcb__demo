/*--------------------------------------------------------------
 *
 *	xneko
 *
 *			Original Writer:
 *            Masayuki Koba
 *
 *			Programmed by:
 *            Masayuki Koba, 1990
 *
 *          Added Color, RootWindow Capability and Quit Ability:
 *            Dan Checkoway, 7-12-94
 *
 *			Converted to use ANSI C and XCB by:
 *            Ian Osgood, 2006
 *
 *--------------------------------------------------------------*/

#if USING_XLIB
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#else
#include <xcb/xcb.h>
/*#include <xcb/xcb_image.h>*/
#include <xcb/xcb_aux.h>		/* xcb_aux_get_screen_t */
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>		/* STRING atom */
#include <xcb/xcb_keysyms.h>

typedef enum { False, True } Bool;

#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>	/* pause() */
#include <signal.h>
#include <math.h>
#include <sys/time.h>

#include "bitmaps/icon.xbm"
#include "bitmaps/cursor.xbm"
#include "bitmaps/cursor_mask.xbm"

#include "bitmaps/space.xbm"

#include "bitmaps/mati2.xbm"
#include "bitmaps/jare2.xbm"
#include "bitmaps/kaki1.xbm"
#include "bitmaps/kaki2.xbm"
#include "bitmaps/mati3.xbm"
#include "bitmaps/sleep1.xbm"
#include "bitmaps/sleep2.xbm"

#include "bitmaps/awake.xbm"

#include "bitmaps/up1.xbm"
#include "bitmaps/up2.xbm"
#include "bitmaps/down1.xbm"
#include "bitmaps/down2.xbm"
#include "bitmaps/left1.xbm"
#include "bitmaps/left2.xbm"
#include "bitmaps/right1.xbm"
#include "bitmaps/right2.xbm"
#include "bitmaps/upright1.xbm"
#include "bitmaps/upright2.xbm"
#include "bitmaps/upleft1.xbm"
#include "bitmaps/upleft2.xbm"
#include "bitmaps/dwleft1.xbm"
#include "bitmaps/dwleft2.xbm"
#include "bitmaps/dwright1.xbm"
#include "bitmaps/dwright2.xbm"

#include "bitmaps/utogi1.xbm"
#include "bitmaps/utogi2.xbm"
#include "bitmaps/dtogi1.xbm"
#include "bitmaps/dtogi2.xbm"
#include "bitmaps/ltogi1.xbm"
#include "bitmaps/ltogi2.xbm"
#include "bitmaps/rtogi1.xbm"
#include "bitmaps/rtogi2.xbm"

#define	BITMAP_WIDTH		32
#define	BITMAP_HEIGHT		32
#define	WINDOW_WIDTH		320
#define	WINDOW_HEIGHT		256
#define	DEFAULT_BORDER		2
#define	DEFAULT_WIN_X		1
#define	DEFAULT_WIN_Y		1
#define	AVAIL_KEYBUF		255

#define	EVENT_MASK       ( XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_BUTTON_PRESS | \
						   XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY )

#define	EVENT_MASK_ROOT  ( XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_EXPOSURE )

#define	MAX_TICK		9999		/* Odd Only! */
#define	INTERVAL		125000L
#define	NEKO_SPEED		16
#define	IDLE_SPACE		6
#define	NORMAL_STATE	1
#define	DEBUG_LIST		2
#define	DEBUG_MOVE		3

#define	NEKO_STOP		0
#define	NEKO_JARE		1
#define	NEKO_KAKI		2
#define	NEKO_AKUBI		3
#define	NEKO_SLEEP		4
#define	NEKO_AWAKE		5
#define	NEKO_U_MOVE		6
#define	NEKO_D_MOVE		7
#define	NEKO_L_MOVE		8
#define	NEKO_R_MOVE		9
#define	NEKO_UL_MOVE	10
#define	NEKO_UR_MOVE	11
#define	NEKO_DL_MOVE	12
#define	NEKO_DR_MOVE	13
#define	NEKO_U_TOGI		14
#define	NEKO_D_TOGI		15
#define	NEKO_L_TOGI		16
#define	NEKO_R_TOGI		17

#define	NEKO_STOP_TIME		4
#define	NEKO_JARE_TIME		10
#define	NEKO_KAKI_TIME		4
#define	NEKO_AKUBI_TIME		3
#define	NEKO_AWAKE_TIME		3
#define	NEKO_TOGI_TIME		10

#define	PI_PER8			((double)3.1415926535/(double)8)

#define	DIRNAMELEN		255

static int  useRoot;
char        *fgColor, *bgColor;

static char  *ProgramName;

xcb_connection_t	*xc;
xcb_screen_t      *theScreen;		/* instead of macro(theDisplay, int theScreen) */
unsigned long  theFgPixel;
unsigned long  theBgPixel;
xcb_window_t         theWindow;
xcb_cursor_t         theCursor;
xcb_key_symbols_t	*theKeySyms;
xcb_atom_t deleteWindowAtom;

static unsigned int  WindowWidth;
static unsigned int  WindowHeight;
static int           WindowPointX;
static int           WindowPointY;

static unsigned int	BorderWidth = DEFAULT_BORDER;

long	IntervalTime = INTERVAL;

int		EventState;

int		NekoTickCount;
int		NekoStateCount;
int		NekoState;

int		MouseX;
int		MouseY;

int		PrevMouseX = 0;
int		PrevMouseY = 0;

int		NekoX;
int		NekoY;

int		NekoMoveDx;
int		NekoMoveDy;

int		NekoLastX;
int		NekoLastY;
xcb_gcontext_t		NekoLastGC;

double	NekoSpeed = (double)NEKO_SPEED;

double	SinPiPer8Times3;
double	SinPiPer8;

xcb_pixmap_t	SpaceXbm;

xcb_pixmap_t	Mati2Xbm;
xcb_pixmap_t	Jare2Xbm;
xcb_pixmap_t	Kaki1Xbm;
xcb_pixmap_t	Kaki2Xbm;
xcb_pixmap_t	Mati3Xbm;
xcb_pixmap_t	Sleep1Xbm;
xcb_pixmap_t	Sleep2Xbm;

xcb_pixmap_t	AwakeXbm;

xcb_pixmap_t	Up1Xbm;
xcb_pixmap_t	Up2Xbm;
xcb_pixmap_t	Down1Xbm;
xcb_pixmap_t	Down2Xbm;
xcb_pixmap_t	Left1Xbm;
xcb_pixmap_t	Left2Xbm;
xcb_pixmap_t	Right1Xbm;
xcb_pixmap_t	Right2Xbm;
xcb_pixmap_t	UpLeft1Xbm;
xcb_pixmap_t	UpLeft2Xbm;
xcb_pixmap_t	UpRight1Xbm;
xcb_pixmap_t	UpRight2Xbm;
xcb_pixmap_t	DownLeft1Xbm;
xcb_pixmap_t	DownLeft2Xbm;
xcb_pixmap_t	DownRight1Xbm;
xcb_pixmap_t	DownRight2Xbm;

xcb_pixmap_t	UpTogi1Xbm;
xcb_pixmap_t	UpTogi2Xbm;
xcb_pixmap_t	DownTogi1Xbm;
xcb_pixmap_t	DownTogi2Xbm;
xcb_pixmap_t	LeftTogi1Xbm;
xcb_pixmap_t	LeftTogi2Xbm;
xcb_pixmap_t	RightTogi1Xbm;
xcb_pixmap_t	RightTogi2Xbm;

xcb_gcontext_t	SpaceGC;

xcb_gcontext_t	Mati2GC;
xcb_gcontext_t	Jare2GC;
xcb_gcontext_t	Kaki1GC;
xcb_gcontext_t	Kaki2GC;
xcb_gcontext_t	Mati3GC;
xcb_gcontext_t	Sleep1GC;
xcb_gcontext_t	Sleep2GC;

xcb_gcontext_t	AwakeGC;

xcb_gcontext_t	Up1GC;
xcb_gcontext_t	Up2GC;
xcb_gcontext_t	Down1GC;
xcb_gcontext_t	Down2GC;
xcb_gcontext_t	Left1GC;
xcb_gcontext_t	Left2GC;
xcb_gcontext_t	Right1GC;
xcb_gcontext_t	Right2GC;
xcb_gcontext_t	UpLeft1GC;
xcb_gcontext_t	UpLeft2GC;
xcb_gcontext_t	UpRight1GC;
xcb_gcontext_t	UpRight2GC;
xcb_gcontext_t	DownLeft1GC;
xcb_gcontext_t	DownLeft2GC;
xcb_gcontext_t	DownRight1GC;
xcb_gcontext_t	DownRight2GC;

xcb_gcontext_t	UpTogi1GC;
xcb_gcontext_t	UpTogi2GC;
xcb_gcontext_t	DownTogi1GC;
xcb_gcontext_t	DownTogi2GC;
xcb_gcontext_t	LeftTogi1GC;
xcb_gcontext_t	LeftTogi2GC;
xcb_gcontext_t	RightTogi1GC;
xcb_gcontext_t	RightTogi2GC;

typedef struct {
  xcb_gcontext_t            *GCCreatePtr;
  xcb_pixmap_t        *BitmapCreatePtr;
  char          *PixelPattern;
  unsigned int  PixelWidth;
  unsigned int  PixelHeight;
} BitmapGCData;

BitmapGCData	BitmapGCDataTable[] = {
  { &SpaceGC, &SpaceXbm, space_bits, space_width, space_height },
  { &Mati2GC, &Mati2Xbm, mati2_bits, mati2_width, mati2_height },
  { &Jare2GC, &Jare2Xbm, jare2_bits, jare2_width, jare2_height },
  { &Kaki1GC, &Kaki1Xbm, kaki1_bits, kaki1_width, kaki1_height },
  { &Kaki2GC, &Kaki2Xbm, kaki2_bits, kaki2_width, kaki2_height },
  { &Mati3GC, &Mati3Xbm, mati3_bits, mati3_width, mati3_height },
  { &Sleep1GC, &Sleep1Xbm, sleep1_bits, sleep1_width, sleep1_height },
  { &Sleep2GC, &Sleep2Xbm, sleep2_bits, sleep2_width, sleep2_height },
  { &AwakeGC, &AwakeXbm, awake_bits, awake_width, awake_height },
  { &Up1GC, &Up1Xbm, up1_bits, up1_width, up1_height },
  { &Up2GC, &Up2Xbm, up2_bits, up2_width, up2_height },
  { &Down1GC, &Down1Xbm, down1_bits, down1_width, down1_height },
  { &Down2GC, &Down2Xbm, down2_bits, down2_width, down2_height },
  { &Left1GC, &Left1Xbm, left1_bits, left1_width, left1_height },
  { &Left2GC, &Left2Xbm, left2_bits, left2_width, left2_height },
  { &Right1GC, &Right1Xbm, right1_bits, right1_width, right1_height },
  { &Right2GC, &Right2Xbm, right2_bits, right2_width, right2_height },
  { &UpLeft1GC, &UpLeft1Xbm, upleft1_bits, upleft1_width, upleft1_height },
  { &UpLeft2GC, &UpLeft2Xbm, upleft2_bits, upleft2_width, upleft2_height },
  { &UpRight1GC,
	  &UpRight1Xbm, upright1_bits, upright1_width, upright1_height },
  { &UpRight2GC,
      &UpRight2Xbm, upright2_bits, upright2_width, upright2_height },
  { &DownLeft1GC, &DownLeft1Xbm, dwleft1_bits, dwleft1_width, dwleft1_height },
  { &DownLeft2GC, &DownLeft2Xbm, dwleft2_bits, dwleft2_width, dwleft2_height },
  { &DownRight1GC,
	  &DownRight1Xbm, dwright1_bits, dwright1_width, dwright1_height },
  { &DownRight2GC,
      &DownRight2Xbm, dwright2_bits, dwright2_width, dwright2_height },
  { &UpTogi1GC, &UpTogi1Xbm, utogi1_bits, utogi1_width, utogi1_height },
  { &UpTogi2GC, &UpTogi2Xbm, utogi2_bits, utogi2_width, utogi2_height },
  { &DownTogi1GC, &DownTogi1Xbm, dtogi1_bits, dtogi1_width, dtogi1_height },
  { &DownTogi2GC, &DownTogi2Xbm, dtogi2_bits, dtogi2_width, dtogi2_height },
  { &LeftTogi1GC, &LeftTogi1Xbm, ltogi1_bits, ltogi1_width, ltogi1_height },
  { &LeftTogi2GC, &LeftTogi2Xbm, ltogi2_bits, ltogi2_width, ltogi2_height },
  { &RightTogi1GC,
      &RightTogi1Xbm, rtogi1_bits, rtogi1_width, rtogi1_height },
  { &RightTogi2GC,
      &RightTogi2Xbm, rtogi2_bits, rtogi2_width, rtogi2_height },
  { NULL, NULL, NULL, 0, 0 }
};

typedef struct {
  xcb_gcontext_t  *TickEvenGCPtr;
  xcb_gcontext_t  *TickOddGCPtr;
} Animation;

Animation  AnimationPattern[] = {
  { &Mati2GC, &Mati2GC },		/* NekoState == NEKO_STOP */
  { &Jare2GC, &Mati2GC },		/* NekoState == NEKO_JARE */
  { &Kaki1GC, &Kaki2GC },		/* NekoState == NEKO_KAKI */
  { &Mati3GC, &Mati3GC },		/* NekoState == NEKO_AKUBI */
  { &Sleep1GC, &Sleep2GC },		/* NekoState == NEKO_SLEEP */
  { &AwakeGC, &AwakeGC },		/* NekoState == NEKO_AWAKE */
  { &Up1GC, &Up2GC }	,		/* NekoState == NEKO_U_MOVE */
  { &Down1GC, &Down2GC },		/* NekoState == NEKO_D_MOVE */
  { &Left1GC, &Left2GC },		/* NekoState == NEKO_L_MOVE */
  { &Right1GC, &Right2GC },		/* NekoState == NEKO_R_MOVE */
  { &UpLeft1GC, &UpLeft2GC },		/* NekoState == NEKO_UL_MOVE */
  { &UpRight1GC, &UpRight2GC },	    /* NekoState == NEKO_UR_MOVE */
  { &DownLeft1GC, &DownLeft2GC },	/* NekoState == NEKO_DL_MOVE */
  { &DownRight1GC, &DownRight2GC },	/* NekoState == NEKO_DR_MOVE */
  { &UpTogi1GC, &UpTogi2GC },		/* NekoState == NEKO_U_TOGI */
  { &DownTogi1GC, &DownTogi2GC },	/* NekoState == NEKO_D_TOGI */
  { &LeftTogi1GC, &LeftTogi2GC },	/* NekoState == NEKO_L_TOGI */
  { &RightTogi1GC, &RightTogi2GC },	/* NekoState == NEKO_R_TOGI */
};


/* PutImage.c: format/unit/order conversion should move out to a library */
static unsigned char const _reverse_byte[0x100] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

/* convert 1Ll <--> 4Mm */
static void
SwapBits(
    register unsigned char *src,
    register unsigned char *dest,
    long srclen, long srcinc, long destinc,
    unsigned int height)
{
    register long h, n;
    register const unsigned char *rev = _reverse_byte;

    srcinc -= srclen;
    destinc -= srclen;
    for (h = height; --h >= 0; src += srcinc, dest += destinc)
        for (n = srclen; --n >= 0; )
            *dest++ = rev[*src++];
}

/* assumes pad is a power of 2 */
#define ROUNDUP(nbytes, pad) (((nbytes) + ((pad) - 1)) & ~(long)((pad) - 1))

/* CrPFBData.c and CrBFData.c (very similar) */
/*  if depth==1, behaves like CreateBitmapFromData */
xcb_pixmap_t CreatePixmapFromBitmapData( xcb_connection_t *c,
	xcb_window_t window, char *data, uint16_t w, uint16_t h,
	uint32_t fg, uint32_t bg, uint32_t depth)
{
  xcb_pixmap_t bitmap = xcb_generate_id( c );

  xcb_create_pixmap( c, depth, bitmap, window, w, h );
  
  xcb_gcontext_t gc = xcb_generate_id( c );
  
  uint32_t mask = (depth==1 ? 0 : XCB_GC_FOREGROUND | XCB_GC_BACKGROUND);
  uint32_t values[] = { fg, bg };

  xcb_create_gc( c, gc, bitmap, mask, values );
  
  /* XImage attributes: bpp=1, xoffset=0,
       byte_order=bit_order=LSB, unit=8, pad=8,   bpl=(w+7/8) */

  /*  must swap and pad the data if bit/byte_order isn't LSB (Mac) */
  
  /* Mac X Server: byte_order=bit_order=MSB, unit=32, padding=32 */
  long bpl = (w+7)/8;
  long pad = xcb_get_setup(c)->bitmap_format_scanline_pad;
  long bpd = ROUNDUP(w, pad)>>3;
  long bufLen = bpd * h;
  uint8_t buf[1024];
  if (xcb_get_setup(c)->bitmap_format_scanline_unit == 32 &&
      xcb_get_setup(c)->bitmap_format_bit_order == XCB_IMAGE_ORDER_MSB_FIRST &&
      xcb_get_setup(c)->image_byte_order == XCB_IMAGE_ORDER_MSB_FIRST)
  {
    SwapBits((unsigned char *)data, buf, bpl, bpl, bpd, h);
  }
  else if (bpl != bpd)
  {
    int i;
    uint8_t *src = (uint8_t *)data, *dest = buf;
    for (i=0; i<h; i++, dest += bpd, src += bpl)
      memcpy(dest, src, bpl);
  }
  else
    memcpy(buf, data, bufLen);

  /* note: CBfD uses XYPixmap, but CPfBD uses XYBitmap
           there shouldn't be a difference when depth==1,
           but the neko images are corrupt if using XYPixmap */
  uint8_t format = (depth==1 ? XCB_IMAGE_FORMAT_XY_PIXMAP : XCB_IMAGE_FORMAT_XY_BITMAP);
  
  /* PutImage.c: left_pad = (image->xoffset + req->xoffset) & (dpy->bitmap_unit-1)
       screen->bitmap_format_scanline_unit
       left_pad = (0 + 0) & (32 - 1) = 0 */

  xcb_put_image( c, format, bitmap, gc,
  	w, h, 0, 0,
  	0, 1,		/* left_pad, depth */
  	bufLen, buf);

#if DEBUG
  xcb_generic_error_t *error = NULL;
  xcb_sync( c, &error );
  if (error) {
    printf("error code %d", (int)error->error_code);
    free(error);
  }
#endif

  xcb_free_gc( c, gc );
  
  /* later: xcb_free_pixmap( c, bitmap ); */
  return bitmap;
}

xcb_pixmap_t CreateBitmapFromData(xcb_connection_t *c, xcb_window_t window,
	char *data, uint16_t w, uint16_t h)
{
	uint32_t depth = 1;
	return CreatePixmapFromBitmapData(c, window, data, w, h, 0, 0, depth);
}

void  InitBitmapAndGCs(void) {
  BitmapGCData	*BitmapGCDataTablePtr;
  uint32_t theGCValues[5];

  theGCValues[0] = XCB_GX_COPY;

  theGCValues[1] = theFgPixel;
  theGCValues[2] = theBgPixel;

  theGCValues[3] = XCB_FILL_STYLE_TILED;
  
  /* TODO: latency: make all the bitmaps, then all the contexts? */

  for ( BitmapGCDataTablePtr = BitmapGCDataTable;
        BitmapGCDataTablePtr->GCCreatePtr != NULL;
        BitmapGCDataTablePtr++ ) {

	*(BitmapGCDataTablePtr->BitmapCreatePtr)
	  = CreatePixmapFromBitmapData( xc, theScreen->root,
		BitmapGCDataTablePtr->PixelPattern,
		BitmapGCDataTablePtr->PixelWidth,
		BitmapGCDataTablePtr->PixelHeight,
		theFgPixel, theBgPixel, theScreen->root_depth);

	theGCValues[4] = *(BitmapGCDataTablePtr->BitmapCreatePtr); /* tile */
	
	*(BitmapGCDataTablePtr->GCCreatePtr) = xcb_generate_id( xc );
	xcb_create_gc( xc, *(BitmapGCDataTablePtr->GCCreatePtr), theWindow,
		  XCB_GC_FUNCTION | XCB_GC_FOREGROUND | XCB_GC_BACKGROUND |
		  XCB_GC_FILL_STYLE | XCB_GC_TILE,
		  theGCValues );
  }
  
  /* later: xcb_free_pixmap( c, bitmap ); */
  /* later: xcb_free_gc( c, gc ); */
}

xcb_atom_t
GetAtom(xcb_connection_t *c, const char *atomName)
{
	xcb_atom_t atom = { XCB_NONE };
	xcb_intern_atom_reply_t *r = xcb_intern_atom_reply(c,
		xcb_intern_atom(c, 0, strlen(atomName), atomName), NULL);
	if (r) {
		atom = r->atom;
		free(r);
	}
	return atom;
}

void
InitScreen( char *DisplayName, char *theGeometry, char *theTitle, Bool iconicState )
{
  xcb_pixmap_t		theCursorSource;
  xcb_pixmap_t		theCursorMask;
  unsigned int   theDepth;
  xcb_colormap_t		theColormap;
  int screen;
  
  if ( ( xc = xcb_connect( DisplayName, &screen ) ) == NULL ) {
	fprintf( stderr, "%s: Can't open connection", ProgramName );
	if ( DisplayName != NULL )
	  fprintf( stderr, " %s.\n", DisplayName );
	else
	  fprintf( stderr, ".\n" );
	exit( 1 );
  }
  
  theScreen = xcb_aux_get_screen(xc, screen);
  if (theScreen == NULL) {
	fprintf( stderr, "%s: Can't get default screen", ProgramName );
	exit( 1 );
  }
  
  theDepth    = theScreen->root_depth;  /* DefaultDepth */
  theColormap = theScreen->default_colormap;

  WindowPointX = DEFAULT_WIN_X;
  WindowPointY = DEFAULT_WIN_Y;
  WindowWidth  = WINDOW_WIDTH;
  WindowHeight = WINDOW_HEIGHT;

#ifdef TODO  
  int			GeometryStatus;
  GeometryStatus = XParseGeometry( theGeometry,
								  &WindowPointX, &WindowPointY,
								  &WindowWidth, &WindowHeight );
#endif
  
  theCursorSource = CreateBitmapFromData( xc,
										  theScreen->root,
										  cursor_bits,
										  cursor_width,
										  cursor_height );
  
  theCursorMask = CreateBitmapFromData( xc,
										theScreen->root,
										cursor_mask_bits,
										cursor_mask_width,
										cursor_mask_height );


  if ( bgColor == NULL) bgColor = "white";
  if ( fgColor == NULL) fgColor = "black";
  
  xcb_alloc_named_color_cookie_t bgCookie = xcb_alloc_named_color ( xc,
		theColormap,  strlen(bgColor), bgColor );

  xcb_alloc_named_color_cookie_t fgCookie = xcb_alloc_named_color ( xc,
		theColormap,  strlen(fgColor), fgColor );

  /* mouse cursor is always black and white */
  xcb_alloc_named_color_cookie_t blackCookie = xcb_alloc_named_color ( xc,
		theColormap,  strlen("black"), "black" );
  xcb_alloc_named_color_cookie_t whiteCookie = xcb_alloc_named_color ( xc,
		theColormap,  strlen("white"), "white" );
		
  xcb_alloc_named_color_reply_t *bgRep = xcb_alloc_named_color_reply( xc, bgCookie, 0 );
  if (!bgRep) {
	fprintf( stderr,
			"%s: Can't allocate the background color %s.\n", ProgramName, bgColor );
	exit( 1 );
  }
  theBgPixel = bgRep->pixel;

  xcb_alloc_named_color_reply_t *fgRep = xcb_alloc_named_color_reply( xc, fgCookie, 0 );
  if (!fgRep) {
	fprintf( stderr,
			"%s: Can't allocate the foreground color %s.\n", ProgramName, fgColor );
	exit( 1 );
  }
  theFgPixel = fgRep->pixel;

  xcb_alloc_named_color_reply_t *blackRep = xcb_alloc_named_color_reply( xc, blackCookie, 0 );
  if (!blackRep) {
	fprintf( stderr,
			"%s: Can't allocate the black color.\n", ProgramName );
	exit( 1 );
  }
  xcb_alloc_named_color_reply_t *whiteRep = xcb_alloc_named_color_reply( xc, whiteCookie, 0 );
  if (!whiteRep) {
	fprintf( stderr,
			"%s: Can't allocate the white color.\n", ProgramName );
	exit( 1 );
  }
  
  theCursor = xcb_generate_id( xc );
  xcb_create_cursor ( xc, theCursor, theCursorSource, theCursorMask,
  	blackRep->visual_red, blackRep->visual_green, blackRep->visual_blue,
  	whiteRep->visual_red, whiteRep->visual_green, whiteRep->visual_blue,
  	cursor_x_hot, cursor_y_hot );

  free(bgRep);
  free(fgRep);
  free(blackRep);
  free(whiteRep);

  if ( useRoot ) {
    uint32_t rootAttributes[] = { theBgPixel, EVENT_MASK_ROOT, theCursor };
	theWindow = theScreen->root;
	xcb_change_window_attributes(xc, theWindow,
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_CURSOR, rootAttributes );
	
	/* XClearWindow: clear area with all dimensions 0 */
	xcb_clear_area( xc, False, theWindow, 0, 0, 0, 0 );
	
	xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply( xc,
	  xcb_get_geometry( xc, theWindow ), NULL);
	if (geometry) {
	  /* only width & height are used by the program */
	  WindowWidth  = geometry->width;
	  WindowHeight = geometry->height;
	  free(geometry);
	}
	
	/* TODO: grab key Alt-Q to quit gracefully? */
  }
  else {
	xcb_pixmap_t                theIconPixmap;

	uint32_t theWindowAttributes[] = {
		theBgPixel,    /* background */
		theFgPixel,    /* border */
		False,         /* override_redirect */
		EVENT_MASK,
		theCursor };

	unsigned long theWindowMask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL |
	  XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK | XCB_CW_CURSOR;
	
	theWindow = xcb_generate_id( xc );
	xcb_create_window( xc,
		theDepth,
		theWindow,
		theScreen->root,
		WindowPointX, WindowPointY,
		WindowWidth, WindowHeight,
		BorderWidth,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		theScreen->root_visual, /* CopyFromParent */
		theWindowMask, theWindowAttributes );

	/* new: obey the window-delete protocol, look for XCB_CLIENT_MESSAGE */
	deleteWindowAtom = GetAtom(xc, "WM_DELETE_WINDOW");
	xcb_set_wm_protocols( xc, theWindow, 1, &deleteWindowAtom );

	theIconPixmap = CreateBitmapFromData( xc, theWindow,
										  icon_bits, icon_width, icon_height );

	xcb_wm_hints_t *theWMHints = xcb_alloc_wm_hints();

	xcb_wm_hints_set_icon_pixmap( theWMHints, theIconPixmap );

	if ( iconicState )
	  xcb_wm_hints_set_iconic( theWMHints );
	else
	  xcb_wm_hints_set_normal( theWMHints );
	
	xcb_set_wm_hints( xc, theWindow, theWMHints);
	
	free( theWMHints );

	/* why hide the structure? */
	xcb_size_hints_t *theSizeHints = xcb_alloc_size_hints();

	/* need enum for second param (user specified) */
	xcb_size_hints_set_position(theSizeHints, 0, WindowPointX, WindowPointY);
	xcb_size_hints_set_size(theSizeHints, 0, WindowWidth, WindowHeight);

	xcb_set_wm_normal_hints(xc, theWindow, theSizeHints);

	xcb_free_size_hints(theSizeHints);

	xcb_set_wm_name( xc, theWindow, STRING, strlen(theTitle), theTitle );
	xcb_set_wm_icon_name( xc, theWindow, STRING, strlen(theTitle), theTitle );

	xcb_map_window( xc, theWindow );

  }
  
  InitBitmapAndGCs();

  xcb_flush(xc);

  /* latency: ask for keysyms now, and receive them later */
  theKeySyms = xcb_key_symbols_alloc( xc );

  /* later: xcb_key_symbols_free( keysyms ); */
  /* later: xcb_refresh_keyboard_mapping ( keysyms, mappingEvent ); */
}


void  Interval(void) {
  pause();
}


void  TickCount(void) {
  if ( ++NekoTickCount >= MAX_TICK )
	NekoTickCount = 0;
  
  if ( NekoTickCount % 2 == 0 )
	if ( NekoStateCount < MAX_TICK )
	  NekoStateCount++;
}


void
SetNekoState( int SetValue )
{
  NekoTickCount = 0;
  NekoStateCount = 0;
  
  NekoState = SetValue;
  
#ifdef	DEBUG
  switch ( NekoState ) {
  case NEKO_STOP:
  case NEKO_JARE:
  case NEKO_KAKI:
  case NEKO_AKUBI:
  case NEKO_SLEEP:
  case NEKO_U_TOGI:
  case NEKO_D_TOGI:
  case NEKO_L_TOGI:
  case NEKO_R_TOGI:
	NekoMoveDx = NekoMoveDy = 0;
	break;
  default:
	break;
  }
#endif
}

/* FillRct.c */
/*   Xlib does merging of requests, but the Flush and frequent DrawGC changes
     defeat this mechanism */

void
DrawNeko( int x, int y, xcb_gcontext_t DrawGC )
{
  xcb_rectangle_t rect = { NekoLastX, NekoLastY, BITMAP_WIDTH, BITMAP_HEIGHT };

  if ( (x != NekoLastX || y != NekoLastY) && (EventState != DEBUG_LIST) )
  {
	xcb_poly_fill_rectangle( xc, theWindow, SpaceGC, 1, &rect );
	rect.x = x; rect.y = y;

  }

  uint32_t originMask = XCB_GC_TILE_STIPPLE_ORIGIN_X | XCB_GC_TILE_STIPPLE_ORIGIN_Y;
  uint32_t origin[2] = { x, y };
  xcb_change_gc( xc, DrawGC, originMask, origin );
  /* XSetTSOrigin( theDisplay, DrawGC, x, y ); */

  xcb_poly_fill_rectangle( xc, theWindow, DrawGC, 1, &rect );

  xcb_flush( xc );

  NekoLastX = x;
  NekoLastY = y;
  
  NekoLastGC = DrawGC;
}


void  RedrawNeko(void) {
  xcb_rectangle_t rect = { NekoLastX, NekoLastY, BITMAP_WIDTH, BITMAP_HEIGHT };

  xcb_poly_fill_rectangle( xc, theWindow, NekoLastGC, 1, &rect );

  xcb_flush( xc );
}



void  NekoDirection(void) {
  int			NewState;
  double		LargeX, LargeY;
  double		Length;
  double		SinTheta;
  
  if ( NekoMoveDx == 0 && NekoMoveDy == 0 ) {
	NewState = NEKO_STOP;
  } else {
	LargeX = (double)NekoMoveDx;
	LargeY = (double)(-NekoMoveDy);
	Length = sqrt( LargeX * LargeX + LargeY * LargeY );
	SinTheta = LargeY / Length;
	
	if ( NekoMoveDx > 0 ) {
	  if ( SinTheta > SinPiPer8Times3 ) {
		NewState = NEKO_U_MOVE;
	  } else if ( ( SinTheta <= SinPiPer8Times3 )
				 && ( SinTheta > SinPiPer8 ) ) {
		NewState = NEKO_UR_MOVE;
	  } else if ( ( SinTheta <= SinPiPer8 )
				 && ( SinTheta > -( SinPiPer8 ) ) ) {
		NewState = NEKO_R_MOVE;
	  } else if ( ( SinTheta <= -( SinPiPer8 ) )
				 && ( SinTheta > -( SinPiPer8Times3 ) ) ) {
		NewState = NEKO_DR_MOVE;
	  } else {
		NewState = NEKO_D_MOVE;
	  }
	} else {
	  if ( SinTheta > SinPiPer8Times3 ) {
		NewState = NEKO_U_MOVE;
	  } else if ( ( SinTheta <= SinPiPer8Times3 )
				 && ( SinTheta > SinPiPer8 ) ) {
		NewState = NEKO_UL_MOVE;
	  } else if ( ( SinTheta <= SinPiPer8 )
				 && ( SinTheta > -( SinPiPer8 ) ) ) {
		NewState = NEKO_L_MOVE;
	  } else if ( ( SinTheta <= -( SinPiPer8 ) )
				 && ( SinTheta > -( SinPiPer8Times3 ) ) ) {
		NewState = NEKO_DL_MOVE;
	  } else {
		NewState = NEKO_D_MOVE;
	  }
	}
  }
  
  if ( NekoState != NewState ) {
	SetNekoState( NewState );
  }
}


Bool  IsWindowOver(void) {
  Bool	ReturnValue = False;
  
  if ( NekoY <= 0 ) {
	NekoY = 0;
	ReturnValue = True;
  } else if ( NekoY >= WindowHeight - BITMAP_HEIGHT ) {
	NekoY = WindowHeight - BITMAP_HEIGHT;
	ReturnValue = True;
  }
  if ( NekoX <= 0 ) {
	NekoX = 0;
	ReturnValue = True;
  } else if ( NekoX >= WindowWidth - BITMAP_WIDTH ) {
	NekoX = WindowWidth - BITMAP_WIDTH;
	ReturnValue = True;
  }
  
  return( ReturnValue );
}


Bool  IsNekoDontMove(void) {
  return( NekoX == NekoLastX  &&  NekoY == NekoLastY );
}


Bool  IsNekoMoveStart(void) {
#ifndef	DEBUG
  if ( (PrevMouseX >= MouseX - IDLE_SPACE
		&& PrevMouseX <= MouseX + IDLE_SPACE) &&
	  (PrevMouseY >= MouseY - IDLE_SPACE 
	   && PrevMouseY <= MouseY + IDLE_SPACE) )
	return( False );
  else
	return( True );
#else
  if ( NekoMoveDx == 0 && NekoMoveDy == 0 )
	return( False );
  else
	return( True );
#endif
}

#ifndef	DEBUG
void  CalcDxDy(void) {
  int			RelativeX, RelativeY;
  double		LargeX, LargeY;
  double		DoubleLength, Length;
  
  /* TODO: replace query with pointer motion notification? */

  xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply( xc,
	xcb_query_pointer( xc, theWindow ), NULL);
	
  RelativeX = reply->win_x;
  RelativeY = reply->win_y;
  
  free(reply);

  PrevMouseX = MouseX;
  PrevMouseY = MouseY;
  
  MouseX = RelativeX;
  MouseY = RelativeY;
  
  LargeX = (double)( MouseX - NekoX - BITMAP_WIDTH / 2 );
  LargeY = (double)( MouseY - NekoY - BITMAP_HEIGHT );
  
  DoubleLength = LargeX * LargeX + LargeY * LargeY;
  
  if ( DoubleLength != (double)0 ) {
	Length = sqrt( DoubleLength );
	if ( Length <= NekoSpeed ) {
	  NekoMoveDx = (int)LargeX;
	  NekoMoveDy = (int)LargeY;
	} else {
	  NekoMoveDx = (int)( ( NekoSpeed * LargeX ) / Length );
	  NekoMoveDy = (int)( ( NekoSpeed * LargeY ) / Length );
	}
  } else {
	NekoMoveDx = NekoMoveDy = 0;
  }
}
#endif

void  NekoThinkDraw(void) {
#ifndef	DEBUG
  CalcDxDy();
#endif

  if ( NekoState != NEKO_SLEEP ) {
	DrawNeko( NekoX, NekoY,
			 NekoTickCount % 2 == 0 ?
			 *(AnimationPattern[ NekoState ].TickEvenGCPtr) :
			 *(AnimationPattern[ NekoState ].TickOddGCPtr) );
  } else {
	DrawNeko( NekoX, NekoY,
			 NekoTickCount % 8 <= 3 ?
			 *(AnimationPattern[ NekoState ].TickEvenGCPtr) :
			 *(AnimationPattern[ NekoState ].TickOddGCPtr) );
  }
  
  TickCount();
  
  switch ( NekoState ) {
  case NEKO_STOP:
	if ( IsNekoMoveStart() ) {
	  SetNekoState( NEKO_AWAKE );
	  break;
	}
	if ( NekoStateCount < NEKO_STOP_TIME ) {
	  break;
	}
	if ( NekoMoveDx < 0 && NekoX <= 0 ) {
	  SetNekoState( NEKO_L_TOGI );
	} else if ( NekoMoveDx > 0 && NekoX >= WindowWidth - BITMAP_WIDTH ) {
	  SetNekoState( NEKO_R_TOGI );
	} else if ( NekoMoveDy < 0 && NekoY <= 0 ) {
	  SetNekoState( NEKO_U_TOGI );
	} else if ( NekoMoveDy > 0 && NekoY >= WindowHeight - BITMAP_HEIGHT ) {
	  SetNekoState( NEKO_D_TOGI );
	} else {
	  SetNekoState( NEKO_JARE );
	}
	break;
  case NEKO_JARE:
	if ( IsNekoMoveStart() ) {
	  SetNekoState( NEKO_AWAKE );
	  break;
	}
	if ( NekoStateCount < NEKO_JARE_TIME ) {
	  break;
	}
	SetNekoState( NEKO_KAKI );
	break;
  case NEKO_KAKI:
	if ( IsNekoMoveStart() ) {
	  SetNekoState( NEKO_AWAKE );
	  break;
	}
	if ( NekoStateCount < NEKO_KAKI_TIME ) {
	  break;
	}
	SetNekoState( NEKO_AKUBI );
	break;
  case NEKO_AKUBI:
	if ( IsNekoMoveStart() ) {
	  SetNekoState( NEKO_AWAKE );
	  break;
	}
	if ( NekoStateCount < NEKO_AKUBI_TIME ) {
	  break;
	}
	SetNekoState( NEKO_SLEEP );
	break;
  case NEKO_SLEEP:
	if ( IsNekoMoveStart() ) {
	  SetNekoState( NEKO_AWAKE );
	  break;
	}
	break;
  case NEKO_AWAKE:
	if ( NekoStateCount < NEKO_AWAKE_TIME ) {
	  break;
	}
	NekoDirection();
	break;
  case NEKO_U_MOVE:
  case NEKO_D_MOVE:
  case NEKO_L_MOVE:
  case NEKO_R_MOVE:
  case NEKO_UL_MOVE:
  case NEKO_UR_MOVE:
  case NEKO_DL_MOVE:
  case NEKO_DR_MOVE:
	NekoX += NekoMoveDx;
	NekoY += NekoMoveDy;
	NekoDirection();
	if ( IsWindowOver() ) {
	  if ( IsNekoDontMove() ) {
		SetNekoState( NEKO_STOP );
	  }
	}
	break;
  case NEKO_U_TOGI:
  case NEKO_D_TOGI:
  case NEKO_L_TOGI:
  case NEKO_R_TOGI:
	if ( IsNekoMoveStart() ) {
	  SetNekoState( NEKO_AWAKE );
	  break;
	}
	if ( NekoStateCount < NEKO_TOGI_TIME ) {
	  break;
	}
	SetNekoState( NEKO_KAKI );
	break;
  default:
	/* Internal Error */
	SetNekoState( NEKO_STOP );
	break;
  }
  
  Interval();
}


#ifdef	DEBUG
void  DisplayCharacters() {
  int		Index;
  int		x, y;
  
  for ( Index = 0, x = 0, y = 0;
	   BitmapGCDataTable[ Index ].GCCreatePtr != NULL; Index++ ) {
	
	DrawNeko( x, y, *(BitmapGCDataTable[ Index ].GCCreatePtr) );
	XFlush( theDisplay );
	
	x += BITMAP_WIDTH;
	
	if ( x > WindowWidth - BITMAP_WIDTH ) {
	  x = 0;
	  y += BITMAP_HEIGHT;
	  if ( y > WindowHeight - BITMAP_HEIGHT) {
		break;
	  }
	}
  }
}

#endif	/* DEBUG */

Bool
ProcessKeyPress( xcb_key_press_event_t *theKeyEvent )
{
  Bool ReturnState = True;

  /* quit on Meta-Q (Alt-Q) */
  xcb_keysym_t theKeySym;

  /* last param is "int col". What? add enumeration to xcb_keysyms.h */
  theKeySym = xcb_key_press_lookup_keysym( theKeySyms, theKeyEvent, 1 );

  /* KeySym XK_Q == 'Q' */
  if (theKeySym == 'Q' && (theKeyEvent->state & XCB_MOD_MASK_1))
    ReturnState = False;

#ifdef	DEBUG
  if ( EventState == DEBUG_MOVE ) {
	switch ( theKeySym ) {
	case XK_KP_1:
	  NekoMoveDx = -(int)( NekoSpeed / sqrt( (double)2 ) );
	  NekoMoveDy = -NekoMoveDx;
	  break;
	case XK_KP_2:
	  NekoMoveDx = 0;
	  NekoMoveDy = (int)NekoSpeed;
	  break;
	case XK_KP_3:
	  NekoMoveDx = (int)( NekoSpeed / sqrt( (double)2 ) );
	  NekoMoveDy = NekoMoveDx;
	  break;
	case XK_KP_4:
	  NekoMoveDx = -(int)NekoSpeed;
	  NekoMoveDy = 0;
	  break;
	case XK_KP_5:
	  NekoMoveDx = 0;
	  NekoMoveDy = 0;
	  break;
	case XK_KP_6:
	  NekoMoveDx = (int)NekoSpeed;
	  NekoMoveDy = 0;
	  break;
	case XK_KP_7:
	  NekoMoveDx = -(int)( NekoSpeed / sqrt( (double)2 ) );
	  NekoMoveDy = NekoMoveDx;
	  break;
	case XK_KP_8:
	  NekoMoveDx = 0;
	  NekoMoveDy = -(int)NekoSpeed;
	  break;
	case XK_KP_9:
	  NekoMoveDx = (int)( NekoSpeed / sqrt( (double)2 ) );
	  NekoMoveDy = -NekoMoveDx;
	  break;
	}
  }
#endif
  
  return( ReturnState );
}


void  NekoAdjust(void) {
  if ( NekoX < 0 )
	NekoX = 0;
  else if ( NekoX > WindowWidth - BITMAP_WIDTH )
	NekoX = WindowWidth - BITMAP_WIDTH;

  if ( NekoY < 0 )
	NekoY = 0;
  else if ( NekoY > WindowHeight - BITMAP_HEIGHT )
	NekoY = WindowHeight - BITMAP_HEIGHT;
}

int IsDeleteMessage(xcb_client_message_event_t *msg)
{
	return msg->data.data32[0] == deleteWindowAtom;
}

Bool  ProcessEvent(void) {
  xcb_generic_event_t *theEvent;
  xcb_configure_notify_event_t *theConfigureNotification;
  xcb_expose_event_t *theExposure;
  xcb_button_press_event_t *theButtonPress;
  Bool	ContinueState = True;
  
  switch ( EventState ) {
  case NORMAL_STATE:
    while ( ContinueState &&
            NULL != (theEvent = xcb_poll_for_event( xc )) ) {  /*while ( XCheckMaskEvent( theDisplay, EVENT_MASK, &theEvent ) ) {*/
	  switch ( theEvent->response_type & 0x7f ) {
	  case XCB_CONFIGURE_NOTIFY:
	    theConfigureNotification = (xcb_configure_notify_event_t *)theEvent;
		WindowWidth = theConfigureNotification->width;
		WindowHeight = theConfigureNotification->height;
		WindowPointX = theConfigureNotification->x;
		WindowPointY = theConfigureNotification->y;
		BorderWidth = theConfigureNotification->border_width;
		NekoAdjust();
		break;
	  case XCB_EXPOSE:
	    theExposure = (xcb_expose_event_t *)theEvent;
		if ( theExposure->count == 0 )
		  RedrawNeko();
		break;
	  case XCB_MAP_NOTIFY:
		RedrawNeko();
		break;
	  case XCB_KEY_PRESS:
		ContinueState = ProcessKeyPress( (xcb_key_press_event_t *)theEvent );
		break;
	  case XCB_BUTTON_PRESS:
	    theButtonPress = (xcb_button_press_event_t *)theEvent;
		ContinueState = ( theButtonPress->detail != 3 );	/* xbutton.button */
		break;
	  /* new: handle ClientMessage */
	  case XCB_CLIENT_MESSAGE:
	    ContinueState = !IsDeleteMessage((xcb_client_message_event_t *)theEvent);
	    break;
	  default:
		/* Unknown Event */
		/*printf("event type:%x\n", (int)theEvent->response_type);*/
		break;
	  }
	  free(theEvent);
	} /* end while */
	break;
#ifdef	DEBUG
  case DEBUG_LIST:
	XNextEvent( theDisplay, &theEvent );
	switch ( theEvent.type ) {
	case ConfigureNotify:
	  WindowWidth = theEvent.xconfigure.width;
	  WindowHeight = theEvent.xconfigure.height;
	  WindowPointX = theEvent.xconfigure.x;
	  WindowPointY = theEvent.xconfigure.y;
	  BorderWidth = theEvent.xconfigure.border_width;
	  break;
	case Expose:
	  if ( theEvent.xexpose.count == 0 )
		DisplayCharacters();
	  break;
	case MapNotify:
	  DisplayCharacters();
	  break;
	case KeyPress:
	  ContinueState = ProcessKeyPress( &theEvent );
	  break;
	case ButtonPress:
	  if ( theEvent.xbutton.button == 3 )
		return( False );
	  break;
	default:
	  /* Unknown Event */
	  break;
	}
	break;
  case DEBUG_MOVE:
	while ( XCheckMaskEvent( theDisplay, EVENT_MASK, &theEvent ) ) {
	  switch ( theEvent.type ) {
	  case ConfigureNotify:
		WindowWidth = theEvent.xconfigure.width;
		WindowHeight = theEvent.xconfigure.height;
		WindowPointX = theEvent.xconfigure.x;
		WindowPointY = theEvent.xconfigure.y;
		BorderWidth = theEvent.xconfigure.border_width;
		NekoAdjust();
		break;
	  case Expose:
		if ( theEvent.xexpose.count == 0 )
		  RedrawNeko();
		break;
	  case MapNotify:
		RedrawNeko();
		break;
	  case KeyPress:
		ContinueState = ProcessKeyPress( &theEvent );
		if ( !ContinueState ) {
		  return( ContinueState );
		}
		break;
	  case ButtonPress:
		if ( theEvent.xbutton.button == 3 )
		  return( False );
		break;
	  default:
		/* Unknown Event */
		break;
	  }
	}
	break;
#endif
  default:
	/* Internal Error */
	break;
  }
  
  return( ContinueState );
}


void  ProcessNeko(void) {
  struct itimerval	Value;
  
  EventState = NORMAL_STATE;

  NekoX = ( WindowWidth - BITMAP_WIDTH / 2 ) / 2;
  NekoY = ( WindowHeight - BITMAP_HEIGHT / 2 ) / 2;
  
  NekoLastX = NekoX;
  NekoLastY = NekoY;
  
  SetNekoState( NEKO_STOP );
  
  timerclear( &Value.it_interval );
  timerclear( &Value.it_value );
  
  Value.it_interval.tv_usec = IntervalTime;
  Value.it_value.tv_usec = IntervalTime;
  
  setitimer( ITIMER_REAL, &Value, 0 );
  
  do {
	NekoThinkDraw();
  } while ( ProcessEvent() );
}

#ifdef	DEBUG

void  NekoList() {
  EventState = DEBUG_LIST;
  
  fprintf( stderr, "\n" );
  fprintf( stderr, "G-0lMw$rI=<($7$^$9!#(Quit !D Alt-Q)\n" );
  fprintf( stderr, "\n" );
  
  XSelectInput( theDisplay, theWindow, EVENT_MASK );
  
  while ( ProcessEvent() );
}


void  NekoMoveTest() {
  struct itimerval	Value;
  
  EventState = DEBUG_MOVE;
  
  NekoX = ( WindowWidth - BITMAP_WIDTH / 2 ) / 2;
  NekoY = ( WindowHeight - BITMAP_HEIGHT / 2 ) / 2;
  
  NekoLastX = NekoX;
  NekoLastY = NekoY;
  
  SetNekoState( NEKO_STOP );
  
  timerclear( &Value.it_interval );
  timerclear( &Value.it_value );
  
  Value.it_interval.tv_usec = IntervalTime;
  Value.it_value.tv_usec = IntervalTime;
  
  setitimer( ITIMER_REAL, &Value, 0 );
  
  fprintf( stderr, "\n" );
  fprintf( stderr, "G-$N0\F0%F%9%H$r9T$$$^$9!#(Quit !D Alt-Q)\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "\t%-!<%Q%C%I>e$N%F%s%-!<$GG-$r0\F0$5$;$F2<$5$$!#\n" );
  fprintf( stderr, "\t(M-8z$J%-!<$O#1!A#9$G$9!#)\n" );
  fprintf( stderr, "\n" );
  
  do {
	NekoThinkDraw();
  } while ( ProcessEvent() );
}


void  ProcessDebugMenu() {
  int		UserSelectNo = 0;
  char	UserAnswer[ BUFSIZ ];
  
  fprintf( stderr, "\n" );
  fprintf( stderr, "!Zxneko %G%P%C%0%a%K%e!<![\n" );
  
  while ( !( UserSelectNo >= 1 && UserSelectNo <= 2 ) ) {
	fprintf( stderr, "\n" );
	fprintf( stderr, "\t1)!!G-%-%c%i%/%?!<0lMwI=<(\n" );
	fprintf( stderr, "\t2)!!G-0\F0%F%9%H\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "Select: " );

	fgets( UserAnswer, sizeof( UserAnswer ), stdin );
	
	UserSelectNo = atoi( UserAnswer );
	
	if ( !( UserSelectNo >= 1 && UserSelectNo <= 2 ) ) {
	  fprintf( stderr, "\n" );
	  fprintf( stderr, "@5$7$$HV9f$rA*Br$7$F2<$5$$!#\n" );
	}
  }
  
  switch ( UserSelectNo ) {
  case 1:
	NekoList();
	break;
  case 2:
	NekoMoveTest();
	break;
  default:
	/* Internal Error */
	break;
  }
  
  fprintf( stderr, "%F%9%H=*N;!#\n" );
  fprintf( stderr, "\n" );
}

#endif	/* DEBUG */

void  NullFunction(int ignored)
{
  /* signal( SIGALRM, NullFunction ); */
}


void  Usage(void) {
  fprintf( stderr,
		  "Usage: %s [-display <display>] [-geometry <geometry>] \\\n",
		  ProgramName );
  fprintf( stderr, "\t[-bg <background>] [-fg <foreground>] \\\n" );
  fprintf( stderr, "\t[-title <title>] [-name <title>] [-iconic] \\\n" );
  fprintf( stderr, "\t[-speed <speed>] [-time <time>] [-root] [-help]\n" );
}


Bool
GetArguments( int argc, char *argv[],
	char *theDisplayName, char *theGeometry, char **theTitle,
	double *NekoSpeed, long *IntervalTime )
{
  int		ArgCounter;
  Bool	iconicState;
  
  theDisplayName[ 0 ] = '\0';
  theGeometry[ 0 ] = '\0';
  
  iconicState = False;
  
  for ( ArgCounter = 0; ArgCounter < argc; ArgCounter++ ) {
	if ( strncmp( argv[ ArgCounter ], "-h", 2 ) == 0 ) {
	  Usage();
	  exit( 0 );
	} else if ( strcmp( argv[ ArgCounter ], "-display" ) == 0 ) {
	  ArgCounter++;
	  if ( ArgCounter < argc ) {
		strcpy( theDisplayName, argv[ ArgCounter ] );
	  } else {
		fprintf( stderr, "%s: -display option error.\n", ProgramName );
		exit( 1 );
	  }
	} else if ( strncmp( argv[ ArgCounter ], "-geom", 5 ) == 0 ) {
	  ArgCounter++;
	  if ( ArgCounter < argc ) {
		strcpy( theGeometry, argv[ ArgCounter ] );
	  } else {
		fprintf( stderr,
				"%s: -geometry option error.\n", ProgramName );
		exit( 1 );
	  }
	} else if ( ( strcmp( argv[ ArgCounter ], "-title" ) == 0 )
			   || ( strcmp( argv[ ArgCounter ], "-name" ) == 0 ) ) {
	  ArgCounter++;
	  if ( ArgCounter < argc ) {
	    *theTitle = argv[ ArgCounter ];
	  } else {
		fprintf( stderr, "%s: -title option error.\n", ProgramName );
		exit( 1 );
	  }
	} else if ( strcmp( argv[ ArgCounter ], "-iconic" ) == 0 ) {
	  iconicState = True;
	} else if ( strcmp( argv[ ArgCounter ], "-speed" ) == 0 ) {
	  ArgCounter++;
	  if ( ArgCounter < argc ) {
		*NekoSpeed = atof( argv[ ArgCounter ] );
	  } else {
		fprintf( stderr, "%s: -speed option error.\n", ProgramName );
		exit( 1 );
	  }
	} else if ( strcmp( argv[ ArgCounter ], "-fg" ) == 0 ) {
	  ArgCounter++;
	  if ( ArgCounter < argc ) {
		fgColor = argv[ArgCounter];
	  } else {
		fprintf( stderr, "%s: -fg option error.\n", ProgramName );
		exit( 1 );
	  }
	} else if ( strcmp( argv[ ArgCounter ], "-bg" ) == 0 ) {
	  ArgCounter++;
	  if ( ArgCounter < argc ) {
		bgColor = argv[ArgCounter];
	  } else {
		fprintf( stderr, "%s: -bg option error.\n", ProgramName );
		exit( 1 );
	  }
	} else if ( strcmp( argv[ ArgCounter ], "-time" ) == 0 ) {
	  ArgCounter++;
	  if ( ArgCounter < argc ) {
		*IntervalTime = atol( argv[ ArgCounter ] );
	  } else {
		fprintf( stderr, "%s: -time option error.\n", ProgramName );
		exit( 1 );
	  }
	} else if ( strcmp( argv[ ArgCounter ], "-root" ) == 0 ) {
	  useRoot = 1;
	} else {
	  fprintf( stderr,
			  "%s: Unknown option \"%s\".\n", ProgramName,
			  argv[ ArgCounter ] );
	  Usage();
	  exit( 1 );
	}
  }
  
  return( iconicState );
}

void UndefineCursor( xcb_connection_t *c, xcb_window_t w)
{
	uint32_t none[] = { XCB_NONE };
	xcb_change_window_attributes( c, w, XCB_CW_CURSOR, none );
}

int
main( int argc, char *argv[] )
{
  Bool	iconicState;
  char	theDisplayName[ DIRNAMELEN ];
  char	theGeometry[ DIRNAMELEN ];
  char	*theTitle = "";
  
  ProgramName = argv[ 0 ];
  
  argc--;
  argv++;
  
  useRoot = 0;
  fgColor = bgColor = (char *) NULL;
  
  iconicState = GetArguments( argc, argv, theDisplayName, theGeometry,
							 &theTitle, &NekoSpeed, &IntervalTime );
  
  if (theTitle[0] == 0) theTitle = ProgramName;
  InitScreen( theDisplayName, theGeometry, theTitle, iconicState );
  
  signal( SIGALRM, NullFunction );
  
  SinPiPer8Times3 = sin( PI_PER8 * (double)3 );
  SinPiPer8 = sin( PI_PER8 );

#ifndef	DEBUG
  ProcessNeko();
#else
  ProcessDebugMenu();
#endif

  UndefineCursor( xc, theWindow );
  xcb_disconnect( xc );
  exit( 0 );
}
