#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <xcb/xcb.h>
#include <xcb/xv.h>

static void PrintUsage()
{
    fprintf(stderr, "Usage:  xvinfo [-display host:dpy]\n");
    exit(0);
}

xcb_screen_t *ScreenOfDisplay (xcb_connection_t *c, int screen)
{
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator (xcb_get_setup (c));
    for (; iter.rem; --screen, xcb_screen_next (&iter))
        if (screen == 0)
            return iter.data;
    return NULL;
}

static int nstrcmp(char *b, int n, char *s) {
    while (n > 0) {
        if (*s == '\0')
            return 1;
        if (*b - *s != 0)
            return *b - *s;
        b++, s++, --n;
    }
    return -(*s != '\0');
}

/* 
 * Copies a string s of size n and returns it with a NULL appended.
 * String returned is allocated with malloc and should be freed later.
 */
static char *ExtractString(char *s, int n) {
    char *str;
    str = (char *)malloc(sizeof(char) * (n+1));
    strncpy(str, s, n); 
    str[n] = '\0';
    return str;
}

int main(int argc, char *argv[])
{
    xcb_connection_t *c;
    int scrn;
    char *display_name = NULL;
    char *name = NULL;
    xcb_window_t root_window = {0};
    xcb_screen_t *screen;
    xcb_xv_query_extension_reply_t *query_ext;
    xcb_xv_query_adaptors_reply_t *adaptors_rep;
    xcb_xv_adaptor_info_iterator_t adaptors_iter;
    xcb_xv_adaptor_info_t *ainfo;
    xcb_xv_format_t *format;
    xcb_xv_query_port_attributes_reply_t *attr_rep;
    xcb_xv_attribute_info_iterator_t attr_iter;
    xcb_xv_attribute_info_t *attribute;

    int nscreens, nattr, i, j, k;

    if ((argc != 1) && (argc != 3))
        PrintUsage();

    if (argc != 1) {
        if (strcmp(argv[1], "-display"))
            PrintUsage();
        display_name = argv[2];
    }

    if (!display_name) display_name = getenv("DISPLAY");
    c = xcb_connect(display_name, &scrn);
    if (xcb_connection_has_error(c))
    {
        fprintf(stderr, "xcbxvinfo: Unable to open display %s\n", display_name);
        exit(1);
    }

    if (!(query_ext = xcb_xv_query_extension_reply(c, xcb_xv_query_extension(c), NULL)))
    {
        fprintf(stderr, "xvinfo: No X-Video extension on %s\n", display_name);
        exit(0);
    }
    else
    {
        fprintf(stdout, "X-Video Extension version %i.%i\n", query_ext->major, query_ext->minor);
    }

    free(query_ext);

    nscreens = xcb_setup_roots_length(xcb_get_setup(c));

    for (i = 0; i < nscreens; i++)
    {
        fprintf(stdout, "screen #%i\n", i);

        screen = ScreenOfDisplay(c, scrn);
        if (screen) root_window = screen->root;

        adaptors_rep = xcb_xv_query_adaptors_reply(c, xcb_xv_query_adaptors(c, root_window), NULL);
        if (!adaptors_rep->num_adaptors) {
            fprintf(stdout, " no adaptors present.\n");
            free(adaptors_rep);
            continue;
        }

        adaptors_iter = xcb_xv_query_adaptors_info_iterator(adaptors_rep);

        for (j = 0; j < adaptors_rep->num_adaptors; j++)
        {
            ainfo = adaptors_iter.data;
            name = ExtractString(xcb_xv_adaptor_info_name(ainfo), xcb_xv_adaptor_info_name_length(ainfo));
            fprintf(stdout, "  Adaptor #%i: \"%s\"\n", j, name);
            fprintf(stdout, "    number of ports: %i\n", ainfo->num_ports);
            fprintf(stdout, "    port base: %i\n", ainfo->base_id);
            fprintf(stdout, "    operations supported: ");
            free(name);

            switch(ainfo->type & (XCB_XV_TYPE_INPUT_MASK | XCB_XV_TYPE_OUTPUT_MASK)) {
                case XCB_XV_TYPE_INPUT_MASK:
                    if (ainfo->type & XCB_XV_TYPE_VIDEO_MASK)
                        fprintf(stdout, "PutVideo ");
                    if (ainfo->type & XCB_XV_TYPE_STILL_MASK)
                        fprintf(stdout, "PutStill ");
                    if (ainfo->type & XCB_XV_TYPE_IMAGE_MASK)
                        fprintf(stdout, "PutImage ");
                    break;
                case XCB_XV_TYPE_OUTPUT_MASK:
                    if (ainfo->type & XCB_XV_TYPE_VIDEO_MASK)
                        fprintf(stdout, "GetVideo ");
                    if (ainfo->type & XCB_XV_TYPE_STILL_MASK)
                        fprintf(stdout, "GetStill ");
                    break;
                default:
                    fprintf(stdout, "none ");
                    break;
            }
            fprintf(stdout, "\n");

            format = xcb_xv_adaptor_info_formats(ainfo);

            fprintf(stdout, "    supported visuals:\n");
            for (k=0; k < ainfo->num_formats; k++, format++)
                fprintf(stdout, "      depth %i, visualID 0x%2x\n",
                        format->depth, format->visual);

            attr_rep = xcb_xv_query_port_attributes_reply(c,
                    xcb_xv_query_port_attributes(c, ainfo->base_id), NULL);
            nattr = attr_rep->num_attributes;
            attr_iter = xcb_xv_query_port_attributes_attributes_iterator(attr_rep);

            if (nattr) {            
                fprintf(stdout, "    number of attributes: %i\n", nattr);

                for (k = 0; k < nattr; k++) {
                    attribute = attr_iter.data;
                    fprintf(stdout, "      \"%s\" (range %i to %i)\n",
                            xcb_xv_attribute_info_name(attribute),
                            attribute->min,
                            attribute->max);

                    if (attribute->flags & XCB_XV_ATTRIBUTE_FLAG_SETTABLE)
                        fprintf(stdout, "              client settable attribute\n");

                    if (attribute->flags & XCB_XV_ATTRIBUTE_FLAG_GETTABLE) {
                        xcb_atom_t the_atom;
                        xcb_intern_atom_reply_t *atom_rep;

                        fprintf(stdout, "              client gettable attribute");

                        atom_rep = xcb_intern_atom_reply(c,
                                xcb_intern_atom(c,
                                    1, 
                                    /*xcb_xv_attribute_info_name_length(attribute),*/
                                    strlen(xcb_xv_attribute_info_name(attribute)),
                                    xcb_xv_attribute_info_name(attribute)),
                                NULL);
                        the_atom = atom_rep->atom;

                        if (the_atom != 0) {
                            xcb_xv_get_port_attribute_reply_t *pattr_rep =
                                xcb_xv_get_port_attribute_reply(c,
                                        xcb_xv_get_port_attribute(c, ainfo->base_id, the_atom),
                                        NULL);
                            if (pattr_rep) fprintf(stdout, " (current value is %i)", pattr_rep->value);
                            free(pattr_rep);
                        }
                        fprintf(stdout, "\n");
                        free(atom_rep);
                    }
                    xcb_xv_attribute_info_next(&attr_iter);
                }
            }
            else {
                fprintf(stdout, "    no port attributes defined\n");
            }

            xcb_xv_query_encodings_reply_t *qencodings_rep;
            qencodings_rep = xcb_xv_query_encodings_reply(c, xcb_xv_query_encodings(c, ainfo->base_id), NULL);
            int nencode = qencodings_rep->num_encodings;
            xcb_xv_encoding_info_iterator_t encoding_iter = xcb_xv_query_encodings_info_iterator(qencodings_rep);
            xcb_xv_encoding_info_t *encoding;

            int ImageEncodings = 0;
            if (nencode) {
                int n;
                for (n = 0; n < nencode; n++) {
                    encoding = encoding_iter.data;
                    name = ExtractString(xcb_xv_encoding_info_name(encoding), xcb_xv_encoding_info_name_length(encoding));
                    if (!nstrcmp(name, strlen(name), "XV_IMAGE"))
                        ImageEncodings++;
                    xcb_xv_encoding_info_next(&encoding_iter);
                    free(name);
                }

                if(nencode - ImageEncodings) {
                    fprintf(stdout, "    number of encodings: %i\n", nencode - ImageEncodings);

                    /* Reset the iter. */
                    encoding_iter = xcb_xv_query_encodings_info_iterator(qencodings_rep);
                    for(n = 0; n < nencode; n++) {
                        encoding = encoding_iter.data;
                        name = ExtractString(xcb_xv_encoding_info_name(encoding), xcb_xv_encoding_info_name_length(encoding));
                        if(nstrcmp(name, strlen(name), "XV_IMAGE")) {
                            fprintf(stdout,
                                    "      encoding ID #%i: \"%*s\"\n",
                                    encoding->encoding,
                                    strlen(name),
                                    name);
                            fprintf(stdout, "        size: %i x %i\n",
                                    encoding->width,
                                    encoding->height);
                            fprintf(stdout, "        rate: %f\n",
                                    (float)encoding->rate.numerator/
                                    (float)encoding->rate.denominator);
                            free(name);
                        }
                        xcb_xv_encoding_info_next(&encoding_iter);
                    }
                }

                if(ImageEncodings && (ainfo->type & XCB_XV_TYPE_IMAGE_MASK)) {
                    char imageName[5] = {0, 0, 0, 0, 0};
                    encoding_iter = xcb_xv_query_encodings_info_iterator(qencodings_rep);
                    for(n = 0; n < nencode; n++) {
                        encoding = encoding_iter.data;
                        name = ExtractString(xcb_xv_encoding_info_name(encoding), xcb_xv_encoding_info_name_length(encoding));
                        if(!nstrcmp(name, strlen(name), "XV_IMAGE")) {
                            fprintf(stdout, 
                                    "    maximum XvImage size: %i x %i\n",	
                                    encoding->width, encoding->height);
                            break;
                        }
                        free(name);
                    }
                    xcb_xv_list_image_formats_reply_t *formats_rep;
                    formats_rep = xcb_xv_list_image_formats_reply(c,
                            xcb_xv_list_image_formats(c, ainfo->base_id),
                            NULL);

                    int numImages = formats_rep->num_formats;
                    xcb_xv_image_format_info_t *format;
                    xcb_xv_image_format_info_iterator_t formats_iter = xcb_xv_list_image_formats_format_iterator(formats_rep);
                    fprintf(stdout, "    Number of image formats: %i\n",
                            numImages);

                    for(n = 0; n < numImages; n++) {
                        format = formats_iter.data;
                        memcpy(imageName, &(format->id), 4);
                        fprintf(stdout, "      id: 0x%x", format->id);
                        if(isprint(imageName[0]) && isprint(imageName[1]) &&
                                isprint(imageName[2]) && isprint(imageName[3])) 
                        {
                            fprintf(stdout, " (%s)\n", imageName);
                        } else {
                            fprintf(stdout, "\n");
                        }
                        fprintf(stdout, "        guid: ");
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[0]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[1]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[2]);
                        fprintf(stdout, "%02x-", (unsigned char) 
                                format->guid[3]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[4]);
                        fprintf(stdout, "%02x-", (unsigned char) 
                                format->guid[5]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[6]);
                        fprintf(stdout, "%02x-", (unsigned char) 
                                format->guid[7]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[8]);
                        fprintf(stdout, "%02x-", (unsigned char) 
                                format->guid[9]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[10]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[11]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[12]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[13]);
                        fprintf(stdout, "%02x", (unsigned char) 
                                format->guid[14]);
                        fprintf(stdout, "%02x\n", (unsigned char) 
                                format->guid[15]);

                        fprintf(stdout, "        bits per pixel: %i\n",
                                format->bpp);
                        fprintf(stdout, "        number of planes: %i\n",
                                format->num_planes);
                        fprintf(stdout, "        type: %s (%s)\n", 
                                (format->type == XCB_XV_IMAGE_FORMAT_INFO_TYPE_RGB) ? "RGB" : "YUV",
                                (format->format == XCB_XV_IMAGE_FORMAT_INFO_FORMAT_PACKED) ? "packed" : "planar");

                        if(format->type == XCB_XV_IMAGE_FORMAT_INFO_TYPE_RGB) {
                            fprintf(stdout, "        depth: %i\n", 
                                    format->depth);

                            fprintf(stdout, "        red, green, blue masks: " 
                                    "0x%x, 0x%x, 0x%x\n", 
                                    format->red_mask,
                                    format->green_mask,
                                    format->blue_mask);
                        } else {

                        }
                        xcb_xv_image_format_info_next(&formats_iter);
                    }
                    free(formats_rep);
                }
            }
            free(qencodings_rep);
            xcb_xv_adaptor_info_next(&adaptors_iter);
        }
        free(adaptors_rep);
    }

    return 0;
}
