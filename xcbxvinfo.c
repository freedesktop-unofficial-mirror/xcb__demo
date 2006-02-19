#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <X11/XCB/xcb.h>
#include <X11/XCB/xv.h>

static void PrintUsage()
{
    fprintf(stderr, "Usage:  xvinfo [-display host:dpy]\n");
    exit(0);
}

XCBSCREEN *ScreenOfDisplay (XCBConnection *c, int screen)
{
    XCBSCREENIter iter = XCBConnSetupSuccessRepRootsIter (XCBGetSetup (c));
    for (; iter.rem; --screen, XCBSCREENNext (&iter))
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
    XCBConnection *c;
    int scrn;
    char *display_name = NULL;
    char *name = NULL;
    XCBWINDOW root_window = {0};
    XCBSCREEN *screen;
    XCBXvQueryExtensionRep *query_ext;
    XCBXvQueryAdaptorsRep *adaptors_rep;
    XCBXvAdaptorInfoIter adaptors_iter;
    XCBXvAdaptorInfo *ainfo;
    XCBXvFormat *format;
    XCBXvQueryPortAttributesRep *attr_rep;
    XCBXvAttributeInfoIter attr_iter;
    XCBXvAttributeInfo *attribute;

    int nscreens, nattr, i, j, k;

    if ((argc != 1) && (argc != 3))
        PrintUsage();

    if (argc != 1) {
        if (strcmp(argv[1], "-display"))
            PrintUsage();
        display_name = argv[2];
    }

    if (!display_name) display_name = getenv("DISPLAY");
    if (!(c = XCBConnect(display_name, &scrn)))
    {
        fprintf(stderr, "xcbxvinfo: Unable to open display %s\n", display_name);
        exit(1);
    }

    if (!(query_ext = XCBXvQueryExtensionReply(c, XCBXvQueryExtension(c), NULL)))
    {
        fprintf(stderr, "xvinfo: No X-Video extension on %s\n", display_name);
        exit(0);
    }
    else
    {
        fprintf(stdout, "X-Video Extension version %i.%i\n", query_ext->major, query_ext->minor);
    }

    free(query_ext);

    nscreens = XCBConnSetupSuccessRepRootsIter(XCBGetSetup(c)).rem;

    for (i = 0; i < nscreens; i++)
    {
        fprintf(stdout, "screen #%i\n", i);

        screen = ScreenOfDisplay(c, scrn);
        if (screen) root_window = screen->root;

        adaptors_rep = XCBXvQueryAdaptorsReply(c, XCBXvQueryAdaptors(c, root_window), NULL);
        if (!adaptors_rep->num_adaptors) {
            fprintf(stdout, " no adaptors present.\n");
            continue;
        }

        adaptors_iter = XCBXvQueryAdaptorsInfoIter(adaptors_rep);

        for (j = 0; j < adaptors_rep->num_adaptors; j++)
        {
            ainfo = adaptors_iter.data;
            name = ExtractString(XCBXvAdaptorInfoName(ainfo), XCBXvAdaptorInfoNameLength(ainfo));
            fprintf(stdout, "  Adaptor #%i: \"%s\"\n", j, name);
            fprintf(stdout, "    number of ports: %i\n", ainfo->num_ports);
            fprintf(stdout, "    port base: %li\n", ainfo->base_id.xid);
            fprintf(stdout, "    operations supported: ");
            free(name);

            switch(ainfo->type & (XCBXvTypeInputMask | XCBXvTypeOutputMask)) {
                case XCBXvTypeInputMask:
                    if (ainfo->type & XCBXvTypeVideoMask)
                        fprintf(stdout, "PutVideo ");
                    if (ainfo->type & XCBXvTypeStillMask)
                        fprintf(stdout, "PutStill ");
                    if (ainfo->type & XCBXvTypeImageMask)
                        fprintf(stdout, "PutImage ");
                    break;
                case XCBXvTypeOutputMask:
                    if (ainfo->type & XCBXvTypeVideoMask)
                        fprintf(stdout, "GetVideo ");
                    if (ainfo->type & XCBXvTypeStillMask)
                        fprintf(stdout, "GetStill ");
                    break;
                default:
                    fprintf(stdout, "none ");
                    break;
            }
            fprintf(stdout, "\n");

            format = XCBXvAdaptorInfoFormats(ainfo);

            fprintf(stdout, "    supported visuals:\n");
            for (k=0; k < ainfo->num_formats; k++, format++)
                fprintf(stdout, "      depth %i, visualID 0x%2lx\n",
                        format->depth, format->visual.id);

            attr_rep = XCBXvQueryPortAttributesReply(c,
                    XCBXvQueryPortAttributes(c, ainfo->base_id), NULL);
            nattr = attr_rep->num_attributes;
            attr_iter = XCBXvQueryPortAttributesAttributesIter(attr_rep);

            if (nattr) {            
                fprintf(stdout, "    number of attributes: %i\n", nattr);

                for (k = 0; k < nattr; k++) {
                    attribute = attr_iter.data;
                    fprintf(stdout, "      \"%s\" (range %li to %li)\n",
                            XCBXvAttributeInfoName(attribute),
                            attribute->min,
                            attribute->max);

                    if (attribute->flags & XCBXvAttributeFlagSettable)
                        fprintf(stdout, "              client settable attribute\n");

                    if (attribute->flags & XCBXvAttributeFlagGettable) {
                        XCBATOM the_atom;
                        XCBInternAtomRep *atom_rep;

                        fprintf(stdout, "              client gettable attribute");

                        atom_rep = XCBInternAtomReply(c,
                                XCBInternAtom(c,
                                    1, 
                                    /*XCBXvAttributeInfoNameLength(attribute),*/
                                    strlen(XCBXvAttributeInfoName(attribute)),
                                    XCBXvAttributeInfoName(attribute)),
                                NULL);
                        the_atom = atom_rep->atom;

                        if (the_atom.xid != 0) {
                            XCBXvGetPortAttributeRep *pattr_rep =
                                XCBXvGetPortAttributeReply(c,
                                        XCBXvGetPortAttribute(c, ainfo->base_id, the_atom),
                                        NULL);
                            if (pattr_rep) fprintf(stdout, " (current value is %li)", pattr_rep->value);
                            free(pattr_rep);
                        }
                        fprintf(stdout, "\n");
                        free(atom_rep);
                    }
                    XCBXvAttributeInfoNext(&attr_iter);
                }
            }
            else {
                fprintf(stdout, "    no port attributes defined\n");
            }

            XCBXvQueryEncodingsRep *qencodings_rep;
            qencodings_rep = XCBXvQueryEncodingsReply(c, XCBXvQueryEncodings(c, ainfo->base_id), NULL);
            int nencode = qencodings_rep->num_encodings;
            XCBXvEncodingInfoIter encoding_iter = XCBXvQueryEncodingsInfoIter(qencodings_rep);
            XCBXvEncodingInfo *encoding;

            int ImageEncodings = 0;
            if (nencode) {
                int n;
                for (n = 0; n < nencode; n++) {
                    encoding = encoding_iter.data;
                    name = ExtractString(XCBXvEncodingInfoName(encoding), XCBXvEncodingInfoNameLength(encoding));
                    if (!nstrcmp(name, strlen(name), "XV_IMAGE"))
                        ImageEncodings++;
                    XCBXvEncodingInfoNext(&encoding_iter);
                    free(name);
                }

                if(nencode - ImageEncodings) {
                    fprintf(stdout, "    number of encodings: %i\n", nencode - ImageEncodings);

                    /* Reset the iter. */
                    encoding_iter = XCBXvQueryEncodingsInfoIter(qencodings_rep);
                    for(n = 0; n < nencode; n++) {
                        encoding = encoding_iter.data;
                        name = ExtractString(XCBXvEncodingInfoName(encoding), XCBXvEncodingInfoNameLength(encoding));
                        if(nstrcmp(name, strlen(name), "XV_IMAGE")) {
                            fprintf(stdout,
                                    "      encoding ID #%li: \"%*s\"\n",
                                    encoding->encoding.xid,
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
                        XCBXvEncodingInfoNext(&encoding_iter);
                    }
                }

                if(ImageEncodings && (ainfo->type & XCBXvTypeImageMask)) {
                    char imageName[5] = {0, 0, 0, 0, 0};
                    encoding_iter = XCBXvQueryEncodingsInfoIter(qencodings_rep);
                    for(n = 0; n < nencode; n++) {
                        encoding = encoding_iter.data;
                        name = ExtractString(XCBXvEncodingInfoName(encoding), XCBXvEncodingInfoNameLength(encoding));
                        if(!nstrcmp(name, strlen(name), "XV_IMAGE")) {
                            fprintf(stdout, 
                                    "    maximum XvImage size: %i x %i\n",	
                                    encoding->width, encoding->height);
                            break;
                        }
                        free(name);
                    }
                    XCBXvListImageFormatsRep *formats_rep;
                    formats_rep = XCBXvListImageFormatsReply(c,
                            XCBXvListImageFormats(c, ainfo->base_id),
                            NULL);

                    int numImages = formats_rep->num_formats;
                    XCBXvImageFormatInfo *format;
                    XCBXvImageFormatInfoIter formats_iter = XCBXvListImageFormatsFormatIter(formats_rep);
                    fprintf(stdout, "    Number of image formats: %i\n",
                            numImages);

                    for(n = 0; n < numImages; n++) {
                        format = formats_iter.data;
                        memcpy(imageName, &(format->id), 4);
                        fprintf(stdout, "      id: 0x%lx", format->id);
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
                                (format->type == XCBXvImageFormatInfoTypeRGB) ? "RGB" : "YUV",
                                (format->format == XCBXvImageFormatInfoFormatPacked) ? "packed" : "planar");

                        if(format->type == XCBXvImageFormatInfoTypeRGB) {
                            fprintf(stdout, "        depth: %i\n", 
                                    format->depth);

                            fprintf(stdout, "        red, green, blue masks: " 
                                    "0x%lx, 0x%lx, 0x%lx\n", 
                                    format->red_mask,
                                    format->green_mask,
                                    format->blue_mask);
                        } else {

                        }
                        XCBXvImageFormatInfoNext(&formats_iter);
                    }
                    free(formats_rep);
                }
            }
            free(qencodings_rep);
            XCBXvAdaptorInfoNext(&adaptors_iter);
        }
    }

    free(adaptors_rep);
    adaptors_rep = NULL;

    return 1;
}
