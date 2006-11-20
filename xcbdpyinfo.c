#include <xcb/xcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

xcb_connection_t *c;

void print_setup();
void print_formats();
void list_extensions(void (*)(int, char *));
void print_extension(int, char *);
void query_extension(int, char *);
void list_screens();
void print_screen(xcb_screen_t *s);

int main(int argc, char **argv)
{
    void (*ext_printer)(int, char *) = print_extension;
    int screen;

    c = xcb_connect(0, &screen);
    if(xcb_connection_has_error(c))
    {
	fputs("Connect failed.\n", stderr);
	exit(1);
    }

    for(--argc; argc; --argc)
	if(!strcmp(argv[argc], "-queryExtensions"))
	    ext_printer = query_extension;

    /* "name of display:    %s" "\n" */
    print_setup(c);
    /* "\n" "focus:  window 0x%x, revert to %s" (e.g. PointerRoot) */
    list_extensions(ext_printer);
    printf("\n" "default screen number:    %d", screen);
    list_screens();
    fputs("\n", stdout);

    xcb_disconnect(c);

    exit(0);
}

void print_setup()
{
    printf("version number:    %d.%d", xcb_get_setup(c)->protocol_major_version, xcb_get_setup(c)->protocol_minor_version);
    fputs("\n" "vendor string:    ", stdout);
    fwrite(xcb_setup_vendor(xcb_get_setup(c)), 1, xcb_setup_vendor_length(xcb_get_setup(c)), stdout);
    printf("\n" "vendor release number:    %d", (int) xcb_get_setup(c)->release_number);
    /* "\n" "XFree86 version: %d.%d.%d.%d" */
    printf("\n" "maximum request size:  %d bytes", xcb_get_setup(c)->maximum_request_length * 4);
    printf("\n" "motion buffer size:  %d", (int)xcb_get_setup(c)->motion_buffer_size);
    printf("\n" "bitmap unit, bit order, padding:    %d, %s, %d", xcb_get_setup(c)->bitmap_format_scanline_unit, (xcb_get_setup(c)->bitmap_format_bit_order == XCB_IMAGE_ORDER_LSB_FIRST) ? "LSBFirst" : "MSBFirst", xcb_get_setup(c)->bitmap_format_scanline_pad);
    printf("\n" "image byte order:    %s", (xcb_get_setup(c)->image_byte_order == XCB_IMAGE_ORDER_LSB_FIRST) ? "LSBFirst" : "MSBFirst");

    print_formats();

    printf("\n" "keycode range:    minimum %d, maximum %d", xcb_get_setup(c)->min_keycode, xcb_get_setup(c)->max_keycode);
}

void print_formats()
{
    int i = xcb_setup_pixmap_formats_length(xcb_get_setup(c));
    xcb_format_t *p = xcb_setup_pixmap_formats(xcb_get_setup(c));
    printf("\n" "number of supported pixmap formats:    %d", i);
    fputs("\n" "supported pixmap formats:", stdout);
    for(--i; i >= 0; --i, ++p)
	printf("\n" "    depth %d, bits_per_pixel %d, scanline_pad %d", p->depth, p->bits_per_pixel, p->scanline_pad);
}

void list_extensions(void (*ext_printer)(int, char *))
{
    xcb_list_extensions_reply_t *r;
    xcb_str_iterator_t i;

    r = xcb_list_extensions_reply(c, xcb_list_extensions(c), 0);
    if(!r)
    {
	fputs("ListExtensions failed.\n", stderr);
	return;
    }

    i = xcb_list_extensions_names_iterator(r);
    printf("\n" "number of extensions:    %d", i.rem);
    for(; i.rem; xcb_str_next(&i))
    {
	fputs("\n" "    ", stdout);
	ext_printer(xcb_str_name_length(i.data), xcb_str_name(i.data));
    }
    free(r);
}

void print_extension(int len, char *name)
{
    fwrite(name, 1, len, stdout);
}

void query_extension(int len, char *name)
{
    xcb_query_extension_reply_t *r;
    int comma = 0;

    r = xcb_query_extension_reply(c, xcb_query_extension(c, len, name), 0);
    if(!r)
    {
	fputs("QueryExtension failed.\n", stderr);
	return;
    }

    print_extension(len, name);
    fputs("  (", stdout);
    if(r->major_opcode)
    {
	printf("opcode: %d", r->major_opcode);
	comma = 1;
    }
    if(r->first_event)
    {
	if(comma)
	    fputs(", ", stdout);
	printf("base event: %d", r->first_event);
	comma = 1;
    }
    if(r->first_error)
    {
	if(comma)
	    fputs(", ", stdout);
	printf("base error: %d", r->first_error);
    }
    fputs(")", stdout);
}

void list_screens()
{
    xcb_screen_iterator_t i;
    int cur;

    i = xcb_setup_roots_iterator(xcb_get_setup(c));
    printf("\n" "number of screens:    %d" "\n", i.rem);
    for(cur = 1; i.rem; xcb_screen_next(&i), ++cur)
    {
	printf("\n" "screen #%d:", cur);
	print_screen(i.data);
    }
}

void print_screen(xcb_screen_t *s)
{
    printf("\n" "  dimensions:    %dx%d pixels (%dx%d millimeters)", s->width_in_pixels, s->height_in_pixels, s->width_in_millimeters, s->height_in_millimeters);
}
