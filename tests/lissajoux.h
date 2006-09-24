#ifndef __TEST_H__
#define __TEST_H__

typedef struct Data_ Data;

struct Data_
{
  xcb_connection_t *conn;
  uint8_t          depth;
  xcb_drawable_t    draw;
  xcb_gcontext_t    gc;
  uint8_t          format;
  xcb_image_t      *image;
};

#endif /* __TEST_H__ */
