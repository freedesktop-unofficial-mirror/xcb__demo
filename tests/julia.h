#ifndef __TEST_H__
#define __TEST_H__

typedef struct Data_ Data;

struct Data_
{
  XCBConnection *conn;
  CARD8          depth;
  XCBDRAWABLE    draw;
  XCBGCONTEXT    gc;
  XCBCOLORMAP    cmap;
  CARD8          format;

  XCBImage      *image;
  CARD32        *palette;
};

#endif /* __TEST_H__ */
