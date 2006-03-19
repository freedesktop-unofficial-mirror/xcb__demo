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
#define X_H   /* make sure we aren't using symbols from X.h */
#include <X11/XCB/xcb.h>
/*#include <X11/XCB/xcb_image.h>*/
#include <X11/XCB/xcb_aux.h>		/* XCBAuxGetScreen */
#include <X11/XCB/xcb_icccm.h>
#include <X11/XCB/xcb_atom.h>		/* STRING atom */

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

#define	EVENT_MASK       ( XCBEventMaskKeyPress | XCBEventMaskButtonPress | \
						   XCBEventMaskExposure | XCBEventMaskStructureNotify )

#define	EVENT_MASK_ROOT  ( XCBEventMaskKeyPress | XCBEventMaskExposure )

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

/*Display        *theDisplay;*/
XCBConnection	*xc;
XCBSCREEN      *theScreen;		/* instead of macro(theDisplay, int theScreen) */
unsigned int   theDepth;
unsigned long  theBlackPixel;
unsigned long  theWhitePixel;
XCBWINDOW         theWindow;
XCBCURSOR         theCursor;

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
XCBGCONTEXT		NekoLastGC;

double	NekoSpeed = (double)NEKO_SPEED;

double	SinPiPer8Times3;
double	SinPiPer8;

XCBPIXMAP	SpaceXbm;

XCBPIXMAP	Mati2Xbm;
XCBPIXMAP	Jare2Xbm;
XCBPIXMAP	Kaki1Xbm;
XCBPIXMAP	Kaki2Xbm;
XCBPIXMAP	Mati3Xbm;
XCBPIXMAP	Sleep1Xbm;
XCBPIXMAP	Sleep2Xbm;

XCBPIXMAP	AwakeXbm;

XCBPIXMAP	Up1Xbm;
XCBPIXMAP	Up2Xbm;
XCBPIXMAP	Down1Xbm;
XCBPIXMAP	Down2Xbm;
XCBPIXMAP	Left1Xbm;
XCBPIXMAP	Left2Xbm;
XCBPIXMAP	Right1Xbm;
XCBPIXMAP	Right2Xbm;
XCBPIXMAP	UpLeft1Xbm;
XCBPIXMAP	UpLeft2Xbm;
XCBPIXMAP	UpRight1Xbm;
XCBPIXMAP	UpRight2Xbm;
XCBPIXMAP	DownLeft1Xbm;
XCBPIXMAP	DownLeft2Xbm;
XCBPIXMAP	DownRight1Xbm;
XCBPIXMAP	DownRight2Xbm;

XCBPIXMAP	UpTogi1Xbm;
XCBPIXMAP	UpTogi2Xbm;
XCBPIXMAP	DownTogi1Xbm;
XCBPIXMAP	DownTogi2Xbm;
XCBPIXMAP	LeftTogi1Xbm;
XCBPIXMAP	LeftTogi2Xbm;
XCBPIXMAP	RightTogi1Xbm;
XCBPIXMAP	RightTogi2Xbm;

XCBGCONTEXT	SpaceGC;

XCBGCONTEXT	Mati2GC;
XCBGCONTEXT	Jare2GC;
XCBGCONTEXT	Kaki1GC;
XCBGCONTEXT	Kaki2GC;
XCBGCONTEXT	Mati3GC;
XCBGCONTEXT	Sleep1GC;
XCBGCONTEXT	Sleep2GC;

XCBGCONTEXT	AwakeGC;

XCBGCONTEXT	Up1GC;
XCBGCONTEXT	Up2GC;
XCBGCONTEXT	Down1GC;
XCBGCONTEXT	Down2GC;
XCBGCONTEXT	Left1GC;
XCBGCONTEXT	Left2GC;
XCBGCONTEXT	Right1GC;
XCBGCONTEXT	Right2GC;
XCBGCONTEXT	UpLeft1GC;
XCBGCONTEXT	UpLeft2GC;
XCBGCONTEXT	UpRight1GC;
XCBGCONTEXT	UpRight2GC;
XCBGCONTEXT	DownLeft1GC;
XCBGCONTEXT	DownLeft2GC;
XCBGCONTEXT	DownRight1GC;
XCBGCONTEXT	DownRight2GC;

XCBGCONTEXT	UpTogi1GC;
XCBGCONTEXT	UpTogi2GC;
XCBGCONTEXT	DownTogi1GC;
XCBGCONTEXT	DownTogi2GC;
XCBGCONTEXT	LeftTogi1GC;
XCBGCONTEXT	LeftTogi2GC;
XCBGCONTEXT	RightTogi1GC;
XCBGCONTEXT	RightTogi2GC;

typedef struct {
  XCBGCONTEXT            *GCCreatePtr;
  XCBPIXMAP        *BitmapCreatePtr;
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
  XCBGCONTEXT  *TickEvenGCPtr;
  XCBGCONTEXT  *TickOddGCPtr;
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
XCBPIXMAP CreatePixmapFromBitmapData( XCBConnection *c,
	XCBWINDOW window, char *data, CARD16 w, CARD16 h,
	CARD32 fg, CARD32 bg, CARD32 depth)
{
  XCBDRAWABLE drawable;
  XCBPIXMAP bitmap = XCBPIXMAPNew( c );

  drawable.window = window;
  XCBCreatePixmap( c, depth, bitmap, drawable, w, h );
  
  XCBGCONTEXT gc = XCBGCONTEXTNew( c );
  
  CARD32 mask = (depth==1 ? 0 : XCBGCForeground | XCBGCBackground);
  CARD32 values[] = { fg, bg };

  drawable.pixmap = bitmap;
  XCBCreateGC( c, gc, drawable, mask, values );
  
  /* XImage attributes: bpp=1, xoffset=0,
       byte_order=bit_order=LSB, unit=8, pad=8,   bpl=(w+7/8) */

  /*  must swap and pad the data if bit/byte_order isn't LSB (Mac) */
  
  /* Mac X Server: byte_order=bit_order=MSB, unit=32, padding=32 */
  long bufLen = (w+7)/8*h;
  BYTE buf[1024];
  if (XCBGetSetup(c)->bitmap_format_scanline_unit == 32 &&
      XCBGetSetup(c)->bitmap_format_bit_order == XCBImageOrderMSBFirst &&
      XCBGetSetup(c)->image_byte_order == XCBImageOrderMSBFirst)
  {
    long bpl = (w+7)/8;
    long pad = XCBGetSetup(c)->bitmap_format_scanline_pad;
    long bpd = ROUNDUP(w, pad)>>3;
    SwapBits((unsigned char *)data, (unsigned char *)buf, bpl, bpl, bpd, h);
    bufLen = bpd * h;
  }
  else
    memcpy(buf, data, bufLen);

  /* note: CBfD uses XYPixmap, but CPfBD uses XYBitmap
           there shouldn't be a difference when depth==1,
           but the neko images are corrupt if using XYPixmap */
  CARD8 format = (depth==1 ? XCBImageFormatXYPixmap : XCBImageFormatXYBitmap);
  
  /* PutImage.c: left_pad = (image->xoffset + req->xoffset) & (dpy->bitmap_unit-1)
       screen->bitmap_format_scanline_unit
       left_pad = (0 + 0) & (32 - 1) = 0 */

  XCBPutImage( c, format, drawable, gc,
  	w, h, 0, 0,
  	0, 1,		/* left_pad, depth */
  	bufLen, buf);

#if DEBUG
  XCBGenericError *error = NULL;
  XCBSync( c, &error );
  if (error) {
    printf("error code %d", (int)error->error_code);
    free(error);
  }
#endif

  XCBFreeGC( c, gc );
  
  /* later: XCBFreePixmap( c, bitmap ); */
  return bitmap;
}

XCBPIXMAP CreateBitmapFromData(XCBConnection *c, XCBWINDOW window,
	char *data, CARD16 w, CARD16 h)
{
	CARD32 depth = 1;
	return CreatePixmapFromBitmapData(c, window, data, w, h, 0, 0, depth);
}

void  InitBitmapAndGCs(void) {
  BitmapGCData	*BitmapGCDataTablePtr;
  CARD32 theGCValues[5];
  XCBDRAWABLE drawable;   drawable.window = theWindow;

  theGCValues[0] = XCBGXcopy;

  theGCValues[1] = theBlackPixel;
  theGCValues[2] = theWhitePixel;

  theGCValues[3] = XCBFillStyleTiled;
  
  /* TODO: latency: make all the bitmaps, then all the contexts? */

  for ( BitmapGCDataTablePtr = BitmapGCDataTable;
        BitmapGCDataTablePtr->GCCreatePtr != NULL;
        BitmapGCDataTablePtr++ ) {

	*(BitmapGCDataTablePtr->BitmapCreatePtr)
	  = CreatePixmapFromBitmapData( xc, theScreen->root,
		BitmapGCDataTablePtr->PixelPattern,
		BitmapGCDataTablePtr->PixelWidth,
		BitmapGCDataTablePtr->PixelHeight,
		theBlackPixel, theWhitePixel, theScreen->root_depth);

	theGCValues[4] = BitmapGCDataTablePtr->BitmapCreatePtr->xid; /* tile */
	
	*(BitmapGCDataTablePtr->GCCreatePtr) = XCBGCONTEXTNew( xc );
	XCBCreateGC( xc, *(BitmapGCDataTablePtr->GCCreatePtr), drawable,
		  XCBGCFunction | XCBGCForeground | XCBGCBackground |
		  XCBGCFillStyle | XCBGCTile,
		  theGCValues );
  }
  
  /* later: XCBFreePixmap( c, bitmap ); */
  /* later: XCBFreeGC( c, gc ); */
  XCBFlush( xc );
}

void
InitScreen( char *DisplayName, char *theGeometry, char *theTitle, Bool iconicState )
{
  XCBPIXMAP		theCursorSource;
  XCBPIXMAP		theCursorMask;
  XCBCOLORMAP		theColormap;
  int screen;
  
  if ( ( xc = XCBConnect( DisplayName, &screen ) ) == NULL ) {
	fprintf( stderr, "%s: Can't open connection", ProgramName );
	if ( DisplayName != NULL )
	  fprintf( stderr, " %s.\n", DisplayName );
	else
	  fprintf( stderr, ".\n" );
	exit( 1 );
  }
  
  theScreen = XCBAuxGetScreen(xc, screen);
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
  
  XCBAllocNamedColorCookie bgCookie = XCBAllocNamedColor ( xc,
		theColormap,  strlen(bgColor), bgColor );

  XCBAllocNamedColorCookie fgCookie = XCBAllocNamedColor ( xc,
		theColormap,  strlen(fgColor), fgColor );
		
  XCBAllocNamedColorRep *bgRep = XCBAllocNamedColorReply( xc, bgCookie, 0 );
  if (!bgRep) {
	fprintf( stderr,
			"%s: Can't allocate the background color %s.\n", ProgramName, bgColor );
	exit( 1 );
  }
  theWhitePixel = bgRep->pixel;

  XCBAllocNamedColorRep *fgRep = XCBAllocNamedColorReply( xc, fgCookie, 0 );
  if (!fgRep) {
	fprintf( stderr,
			"%s: Can't allocate the foreground color %s.\n", ProgramName, fgColor );
	exit( 1 );
  }
  theBlackPixel = fgRep->pixel;
  
  theCursor = XCBCURSORNew( xc );
  XCBCreateCursor ( xc, theCursor, theCursorSource, theCursorMask,
  	fgRep->visual_red, fgRep->visual_green, fgRep->visual_blue,
  	bgRep->visual_red, bgRep->visual_green, bgRep->visual_blue,
  	cursor_x_hot, cursor_y_hot );

  free(bgRep);
  free(fgRep);

  if ( useRoot ) {
    CARD32 rootAttributes[] = { EVENT_MASK_ROOT, theCursor.xid };
	theWindow = theScreen->root;
	XCBChangeWindowAttributes(xc, theWindow,
		XCBCWEventMask | XCBCWCursor, rootAttributes );
  }
  else {
	XCBPIXMAP                theIconPixmap;

	CARD32 theWindowAttributes[] = {
		theWhitePixel, /* background */
		theBlackPixel, /* border */
		False,         /* override_redirect */
		EVENT_MASK,
		theCursor.xid };

	unsigned long theWindowMask = XCBCWBackPixel | XCBCWBorderPixel |
	  XCBCWOverrideRedirect | XCBCWEventMask | XCBCWCursor;
	
	theWindow = XCBWINDOWNew( xc );
	XCBCreateWindow( xc,
		theDepth,
		theWindow,
		theScreen->root,
		WindowPointX, WindowPointY,
		WindowWidth, WindowHeight,
		BorderWidth,
		XCBWindowClassInputOutput,
		theScreen->root_visual, /* CopyFromParent */
		theWindowMask, theWindowAttributes );

	theIconPixmap = CreateBitmapFromData( xc, theWindow,
										  icon_bits, icon_width, icon_height );
	
#ifdef TODO_ICCCM
    /* Um... there is no function to send the hints...
	WMHints              theWMHints;
	WMHintsSetIconPixmap( &theHints, theIconPixmap );

	if ( iconicState )
	  WMHintsSetIconic( &theHints );
	else
	  WMHintsSetNormal( &theHints );
    */
	theWMHints.icon_pixmap = theIconPixmap;
	
	if ( iconicState )
	  theWMHints.initial_state = IconicState;
	else
	  theWMHints.initial_state = NormalState;
	
	theWMHints.flags = IconPixmapHint | StateHint;
	
	XSetWMHints( theDisplay, theWindow, &theWMHints );
#endif

#ifdef TODO_ICCCM
	/*
	SizeHints *hints = AllocSizeHints();
	SizeHintsSetPosition(hints, WindowPointX, WindowPointY);
	SizeHintsSetSize(hints, WindowWidth, WindowHeight);
	SetWMNormalHints(xc, theWindow, hints);
	FreeSizeHints(hints);
	*/
	XSizeHints            theSizeHints;

	theSizeHints.flags = PPosition | PSize;
	theSizeHints.x = WindowPointX;
	theSizeHints.y = WindowPointY;
	theSizeHints.width = WindowWidth;
	theSizeHints.height = WindowHeight;
	
	XSetNormalHints( theDisplay, theWindow, &theSizeHints );
#endif

	/* Um, why do I have to specify the encoding in this API? */
	SetWMName( xc, theWindow, STRING, strlen(theTitle), theTitle );
	SetWMIconName( xc, theWindow, STRING, strlen(theTitle), theTitle );

	XCBMapWindow( xc, theWindow );

    /* moved to the CreateWindow attribute list */
	/* XSelectInput( theDisplay, theWindow, EVENT_MASK ); */

  }
  
#ifdef TODO
  /* is this really necessary? */
  XSetWindowBackground( theDisplay, theWindow, theWhitePixel );
  XClearWindow( theDisplay, theWindow );
  XFlush( theDisplay );

  XCBWINDOW		theRoot;
  XGetGeometry( theDisplay, theWindow, &theRoot,
			   &WindowPointX, &WindowPointY, &WindowWidth, &WindowHeight,
			   &BorderWidth, &theDepth );
#endif

  InitBitmapAndGCs();

  XCBFlush(xc);
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
DrawNeko( int x, int y, XCBGCONTEXT DrawGC )
{
  XCBDRAWABLE drawable;  drawable.window = theWindow;
  XCBRECTANGLE rect = { NekoLastX, NekoLastY, BITMAP_WIDTH, BITMAP_HEIGHT };

  if ( (x != NekoLastX || y != NekoLastY) && (EventState != DEBUG_LIST) )
  {
	XCBPolyFillRectangle( xc, drawable, SpaceGC, 1, &rect );
	rect.x = x; rect.y = y;

  }

  CARD32 originMask = XCBGCTileStippleOriginX | XCBGCTileStippleOriginY;
  CARD32 origin[2] = { x, y };
  XCBChangeGC( xc, DrawGC, originMask, origin );
  /* XSetTSOrigin( theDisplay, DrawGC, x, y ); */

  XCBPolyFillRectangle( xc, drawable, DrawGC, 1, &rect );

  XCBFlush( xc );

  NekoLastX = x;
  NekoLastY = y;
  
  NekoLastGC = DrawGC;
}


void  RedrawNeko(void) {
  XCBDRAWABLE drawable;  drawable.window = theWindow;
  XCBRECTANGLE rect = { NekoLastX, NekoLastY, BITMAP_WIDTH, BITMAP_HEIGHT };

  XCBPolyFillRectangle( xc, drawable, NekoLastGC, 1, &rect );

  XCBFlush( xc );
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

  XCBQueryPointerRep *reply = XCBQueryPointerReply( xc,
	XCBQueryPointer( xc, theWindow ), NULL);
	
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
ProcessKeyPress( XCBKeyPressEvent *theKeyEvent )
{
#if TODO    
  Bool ReturnState = True;

  int			Length;
  char		theKeyBuffer[ AVAIL_KEYBUF + 1 ];
    int			theKeyBufferMaxLen = AVAIL_KEYBUF;
  KeySym		theKeySym;
  XComposeStatus	theComposeStatus;

  Length = XLookupString( theKeyEvent,
						 theKeyBuffer, theKeyBufferMaxLen,
						 &theKeySym, &theComposeStatus );

  if ( Length > 0 ) {
	switch ( theKeyBuffer[ 0 ] ) {
	case 'q':
	case 'Q':
	  if ( theKeyEvent->state & XCBModMask1 ) {	/* META (Alt) %-!< */
		ReturnState = False;
	  }
	  break;
	default:
	  break;
	}
  }
#else
  /* quit on any key */
  Bool ReturnState = False;
#endif


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

Bool  ProcessEvent(void) {
  XCBGenericEvent *theEvent;
  XCBConfigureNotifyEvent *theConfigureNotification;
  XCBExposeEvent *theExposure;
  XCBButtonPressEvent *theButtonPress;
  Bool	ContinueState = True;
  int error = 0;
  
  switch ( EventState ) {
  case NORMAL_STATE:
    while ( NULL != (theEvent = XCBPollForEvent( xc, &error )) ) {  /*while ( XCheckMaskEvent( theDisplay, EVENT_MASK, &theEvent ) ) {*/
	  switch ( theEvent->response_type ) {
	  case XCBConfigureNotify:
	    theConfigureNotification = (XCBConfigureNotifyEvent *)theEvent;
		WindowWidth = theConfigureNotification->width;
		WindowHeight = theConfigureNotification->height;
		WindowPointX = theConfigureNotification->x;
		WindowPointY = theConfigureNotification->y;
		BorderWidth = theConfigureNotification->border_width;
		NekoAdjust();
		break;
	  case XCBExpose:
	    theExposure = (XCBExposeEvent *)theEvent;
		if ( theExposure->count == 0 )
		  RedrawNeko();
		break;
	  case XCBMapNotify:
		RedrawNeko();
		break;
	  case XCBKeyPress:
		ContinueState = ProcessKeyPress( (XCBKeyPressEvent *)theEvent );
		if ( !ContinueState ) {
		  free(theEvent);
		  return( ContinueState );
		}
		break;
	  case XCBButtonPress:
	    theButtonPress = (XCBButtonPressEvent *)theEvent;
		if ( theButtonPress->detail.id == 3 ) {	/* xbutton.button */
		  free(theEvent);
		  return( False );
		}
		break;
	  default:
		/* Unknown Event */
		break;
	  }
	  free(theEvent);
	  if (error != 0)
	  	return False;
	}
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

void UndefineCursor( XCBConnection *c, XCBWINDOW w)
{
	CARD32 none[] = { XCBNone };
	XCBChangeWindowAttributes( c, w, XCBCWCursor, none );
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
  XCBDisconnect( xc );
  exit( 0 );
}
