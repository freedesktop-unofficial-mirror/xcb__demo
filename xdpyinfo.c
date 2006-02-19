#include <X11/XCB/xcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

XCBConnection *c;

void print_setup();
void print_formats();
void list_extensions(void (*)(int, char *));
void print_extension(int, char *);
void query_extension(int, char *);
void list_screens();
void print_screen(XCBSCREEN *s);

int main(int argc, char **argv)
{
    void (*ext_printer)(int, char *) = print_extension;
    int screen;

    c = XCBConnect(0, &screen);
    if(!c)
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

    XCBDisconnect(c);

    exit(0);
}

void print_setup()
{
    printf("version number:    %d.%d", XCBGetSetup(c)->protocol_major_version, XCBGetSetup(c)->protocol_minor_version);
    fputs("\n" "vendor string:    ", stdout);
    fwrite(XCBConnSetupSuccessRepVendor(XCBGetSetup(c)), 1, XCBConnSetupSuccessRepVendorLength(XCBGetSetup(c)), stdout);
    printf("\n" "vendor release number:    %d", (int) XCBGetSetup(c)->release_number);
    /* "\n" "XFree86 version: %d.%d.%d.%d" */
    printf("\n" "maximum request size:  %d bytes", XCBGetSetup(c)->maximum_request_length * 4);
    printf("\n" "motion buffer size:  %d", (int)XCBGetSetup(c)->motion_buffer_size);
    printf("\n" "bitmap unit, bit order, padding:    %d, %s, %d", XCBGetSetup(c)->bitmap_format_scanline_unit, (XCBGetSetup(c)->bitmap_format_bit_order == LSBFirst) ? "LSBFirst" : "MSBFirst", XCBGetSetup(c)->bitmap_format_scanline_pad);
    printf("\n" "image byte order:    %s", (XCBGetSetup(c)->image_byte_order == LSBFirst) ? "LSBFirst" : "MSBFirst");

    print_formats();

    printf("\n" "keycode range:    minimum %d, maximum %d", XCBGetSetup(c)->min_keycode.id, XCBGetSetup(c)->max_keycode.id);
}

void print_formats()
{
    int i = XCBConnSetupSuccessRepPixmapFormatsLength(XCBGetSetup(c));
    XCBFORMAT *p = XCBConnSetupSuccessRepPixmapFormats(XCBGetSetup(c));
    printf("\n" "number of supported pixmap formats:    %d", i);
    fputs("\n" "supported pixmap formats:", stdout);
    for(--i; i >= 0; --i, ++p)
	printf("\n" "    depth %d, bits_per_pixel %d, scanline_pad %d", p->depth, p->bits_per_pixel, p->scanline_pad);
}

void list_extensions(void (*ext_printer)(int, char *))
{
    XCBListExtensionsRep *r;
    XCBSTRIter i;

    r = XCBListExtensionsReply(c, XCBListExtensions(c), 0);
    if(!r)
    {
	fputs("ListExtensions failed.\n", stderr);
	return;
    }

    i = XCBListExtensionsNamesIter(r);
    printf("\n" "number of extensions:    %d", i.rem);
    for(; i.rem; XCBSTRNext(&i))
    {
	fputs("\n" "    ", stdout);
	ext_printer(XCBSTRNameLength(i.data), XCBSTRName(i.data));
    }
    free(r);
}

void print_extension(int len, char *name)
{
    fwrite(name, 1, len, stdout);
}

void query_extension(int len, char *name)
{
    XCBQueryExtensionRep *r;
    int comma = 0;

    r = XCBQueryExtensionReply(c, XCBQueryExtension(c, len, name), 0);
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
    XCBSCREENIter i;
    int cur;

    i = XCBConnSetupSuccessRepRootsIter(XCBGetSetup(c));
    printf("\n" "number of screens:    %d" "\n", i.rem);
    for(cur = 1; i.rem; XCBSCREENNext(&i), ++cur)
    {
	printf("\n" "screen #%d:", cur);
	print_screen(i.data);
    }
}

void print_screen(XCBSCREEN *s)
{
    printf("\n" "  dimensions:    %dx%d pixels (%dx%d millimeters)", s->width_in_pixels, s->height_in_pixels, s->width_in_millimeters, s->height_in_millimeters);
}
