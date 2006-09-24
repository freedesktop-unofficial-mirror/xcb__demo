#ifndef __TEST_H__
#define __TEST_H__

typedef struct Data_ Data;

struct Data_
{
  xcb_connection_t *conn;
  uint8_t          depth;
  xcb_drawable_t    draw;
  xcb_gcontext_t    gc;
  xcb_colormap_t    cmap;
  uint8_t          format;

  xcb_image_t      *image;
  uint32_t        *palette;
};

#endif /* __TEST_H__ */
