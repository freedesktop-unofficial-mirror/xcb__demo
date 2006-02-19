#ifndef __TEST_H__
#define __TEST_H__

typedef struct Data_ Data;

struct Data_
{
  XCBConnection *conn;
  CARD8          depth;
  XCBDRAWABLE    draw;
  XCBGCONTEXT    gc;
  CARD8          format;
  XCBImage      *image;
};

#endif /* __TEST_H__ */
