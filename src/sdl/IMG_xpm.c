/*
    SDL_image:  An example image loading library for use with SDL
    Copyright (C) 1999-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* $Id: IMG_xpm.c,v 1.10 2004/01/04 22:04:38 slouken Exp $ */

/*
 * XPM (X PixMap) image loader:
 *
 * Supports the XPMv3 format, EXCEPT:
 * - hotspot coordinates are ignored
 * - only colour ('c') colour symbols are used
 * - rgb.txt is not used (for portability), so only RGB colours
 *   are recognized (#rrggbb etc) - only a few basic colour names are
 *   handled
 *
 * The result is an 8bpp indexed surface if possible, otherwise 32bpp.
 * The colourkey is correctly set if transparency is used.
 *
 * Besides the standard API, also provides
 *
 *     SDL_Surface *IMG_ReadXPMFromArray(char **xpm)
 *
 * that reads the image data from an XPM file included in the C source.
 *
 * TODO: include rgb.txt here. The full table (from solaris 2.6) only
 * requires about 13K in binary form.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

//#include "SDL_image.h"


#ifdef LOAD_XPM

/* See if an image is contained in a data source */
#if 0
int IMG_isXPM(SDL_RWops *src)
{
        char magic[9];

        return (SDL_RWread(src, magic, sizeof (magic), 1)
                && memcmp(magic, "/* XPM */", 9) == 0);
}
#endif

/* Hash table to look up colors from pixel strings */
#define STARTING_HASH_SIZE 256

struct hash_entry {
        char *key;
        Uint32 color;
        struct hash_entry *next;
};

struct color_hash {
        struct hash_entry **table;
        struct hash_entry *entries; /* array of all entries */
        struct hash_entry *next_free;
        int size;
        int maxnum;
};

static int hash_key(const char *key, int cpp, int size)
{
        int hash;

        hash = 0;
        while ( cpp-- > 0 ) {
                hash = hash * 33 + *key++;
        }
        return hash & (size - 1);
}

static struct color_hash *create_colorhash(int maxnum)
{
        int bytes, s;
        struct color_hash *hash;

        /* we know how many entries we need, so we can allocate
           everything here */
        hash = malloc(sizeof *hash);
        if (!hash)
                return NULL;

        /* use power-of-2 sized hash table for decoding speed */
        for (s = STARTING_HASH_SIZE; s < maxnum; s <<= 1)
                ;
        hash->size = s;
        hash->maxnum = maxnum;
        bytes = hash->size * sizeof (struct hash_entry **);
        hash->entries = NULL;        /* in case malloc fails */
        hash->table = malloc(bytes);
        if (!hash->table)
                return NULL;
        memset(hash->table, 0, bytes);
        hash->entries = malloc(maxnum * sizeof (struct hash_entry));
        if (!hash->entries)
                return NULL;
        hash->next_free = hash->entries;
        return hash;
}

static int add_colorhash(struct color_hash *hash,
                         char *key, int cpp, Uint32 color)
{
        const int indexkey = hash_key(key, cpp, hash->size);
        struct hash_entry *e = hash->next_free++;
        e->color = color;
        e->key = key;
        e->next = hash->table[indexkey];
        hash->table[indexkey] = e;
        return 1;
}

/* fast lookup that works if cpp == 1 */
#define QUICK_COLORHASH(hash, key) ((hash)->table[*(const Uint8 *)(key)]->color)

static Uint32 get_colorhash(struct color_hash *hash, const char *key, int cpp)
{
        struct hash_entry *entry = hash->table[hash_key(key, cpp, hash->size)];
        while (entry) {
                if (memcmp(key, entry->key, cpp) == 0)
                        return entry->color;
                entry = entry->next;
        }
        return 0;                /* garbage in - garbage out */
}

static void free_colorhash(struct color_hash *hash)
{
        if (hash && hash->table) {
                free(hash->table);
                free(hash->entries);
                free(hash);
        }
}

/* portable case-insensitive string comparison */
static int string_equal(const char *a, const char *b, size_t n)
{
        while (*a && *b && n) {
                if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
                        return 0;
                a++;
                b++;
                n--;
        }
        return *a == *b;
}

#define ARRAYSIZE(a) (int)(sizeof (a) / sizeof ((a)[0]))

/*
 * convert colour spec to RGB (in 0xrrggbb format).
 * return 1 if successful.
 */
static int color_to_rgb(const char *spec, size_t speclen, Uint32 *rgb)
{
        /* poor man's rgb.txt */
        static struct { const char *name; Uint32 rgb; } known[] = {
                {"none",  0xffffffff},
                {"black", 0x00000000},
                {"white", 0x00ffffff},
                {"red",   0x00ff0000},
                {"green", 0x0000ff00},
                {"blue",  0x000000ff}
        };

        if (spec[0] == '#') {
                char buf[7];
                switch (speclen) {
                case 4:
                        buf[0] = buf[1] = spec[1];
                        buf[2] = buf[3] = spec[2];
                        buf[4] = buf[5] = spec[3];
                        break;
                case 7:
                        memcpy(buf, spec + 1, 6);
                        break;
                case 13:
                        buf[0] = spec[1];
                        buf[1] = spec[2];
                        buf[2] = spec[5];
                        buf[3] = spec[6];
                        buf[4] = spec[9];
                        buf[5] = spec[10];
                        break;
                }
                buf[6] = '\0';
                *rgb = strtol(buf, NULL, 16);
                return 1;
        } else {
                int i;
                for (i = 0; i < ARRAYSIZE(known); i++)
                        if (string_equal(known[i].name, spec, speclen)) {
                                *rgb = known[i].rgb;
                                return 1;
                        }
                return 0;
        }
}

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static char *linebuf;
static int buflen;
static const char *error;

/*
 * Read next line from the source.
 * If len > 0, it's assumed to be at least len chars (for efficiency).
 * Return NULL and set error upon EOF or parse error.
 */
static const char *get_next_line(const char ***lines, SDL_RWops *src, int len)
{
        if (lines) {
                return *(*lines)++;
        } else {
                char c;
                int n;
                do {
                        if (SDL_RWread(src, &c, 1, 1) <= 0) {
                                error = "Premature end of data";
                                return NULL;
                        }
                } while (c != '"');
                if (len) {
                        len += 4;        /* "\",\n\0" */
                        if (len > buflen){
                                buflen = len;
                                linebuf = realloc(linebuf, buflen);
                                if (!linebuf) {
                                        error = "Out of memory";
                                        return NULL;
                                }
                        }
                        if (SDL_RWread(src, linebuf, len - 1, 1) <= 0) {
                                error = "Premature end of data";
                                return NULL;
                        }
                        n = len - 2;
                } else {
                        n = 0;
                        do {
                                if (n >= buflen - 1) {
                                        if (buflen == 0)
                                                buflen = 16;
                                        buflen *= 2;
                                        linebuf = realloc(linebuf, buflen);
                                        if (!linebuf) {
                                                error = "Out of memory";
                                                return NULL;
                                        }
                                }
                                if (SDL_RWread(src, linebuf + n, 1, 1) <= 0) {
                                        error = "Premature end of data";
                                        return NULL;
                                }
                        } while (linebuf[n++] != '"');
                        n--;
                }
                linebuf[n] = '\0';
                return linebuf;
        }
}

#define SKIPSPACE(p)                            \
do {                                            \
        while (isspace((unsigned char)*(p)))        \
              ++(p);                                \
} while (0)

#define SKIPNONSPACE(p)                                 \
do {                                                    \
        while (!isspace((unsigned char)*(p)) && *p)        \
              ++(p);                                        \
} while (0)

/* read XPM from either array or RWops */
static SDL_Surface *load_xpm(const char **xpm, SDL_RWops *src)
{
        SDL_Surface *image = NULL;
        int indexc;
        int x, y;
        int w, h, ncolors, cpp;
        int indexed;
        Uint8 *dst;
        struct color_hash *colors = NULL;
        SDL_Color *im_colors = NULL;
        char *keystrings = NULL, *nextkey;
        const char *line;
        const char ***xpmlines = NULL;
        int pixels_len;

        error = NULL;
        linebuf = NULL;
        buflen = 0;

        if (xpm)
                xpmlines = &xpm;

        line = get_next_line(xpmlines, src, 0);
        if (!line)
                goto done;
        /*
         * The header string of an XPMv3 image has the format
         *
         * <width> <height> <ncolors> <cpp> [ <hotspot_x> <hotspot_y> ]
         *
         * where the hotspot coords are intended for mouse cursors.
         * Right now we don't use the hotspots but it should be handled
         * one day.
         */
        if (sscanf(line, "%d %d %d %d", &w, &h, &ncolors, &cpp) != 4
           || w <= 0 || h <= 0 || ncolors <= 0 || cpp <= 0) {
                error = "Invalid format description";
                goto done;
        }

        keystrings = malloc(ncolors * cpp);
        if (!keystrings) {
                error = "Out of memory";
                goto done;
        }
        nextkey = keystrings;

        /* Create the new surface */
        if (ncolors <= 256) {
                indexed = 1;
                image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8,
                                             0, 0, 0, 0);
                im_colors = image->format->palette->colors;
                image->format->palette->ncolors = ncolors;
        } else {
                indexed = 0;
                image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
                                             0xff0000, 0x00ff00, 0x0000ff, 0);
        }
        if (!image) {
                /* Hmm, some SDL error (out of memory?) */
                goto done;
        }

        /* Read the colors */
        colors = create_colorhash(ncolors);
        if (!colors) {
                error = "Out of memory";
                goto done;
        }
        for (indexc = 0; indexc < ncolors; ++indexc ) {
                const char *p;
                line = get_next_line(xpmlines, src, 0);
                if (!line)
                        goto done;

                p = line + cpp + 1;

                /* parse a colour definition */
                for (;;) {
                        char nametype;
                        const char *colname;
                        Uint32 rgb, pixel;

                        SKIPSPACE(p);
                        if (!*p) {
                                error = "colour parse error";
                                goto done;
                        }
                        nametype = *p;
                        SKIPNONSPACE(p);
                        SKIPSPACE(p);
                        colname = p;
                        SKIPNONSPACE(p);
                        if (nametype == 's')
                                continue;      /* skip symbolic colour names */

                        if (!color_to_rgb(colname, p - colname, &rgb))
                                continue;

                        memcpy(nextkey, line, cpp);
                        if (indexed) {
                                SDL_Color *c = im_colors + indexc;
                                c->r = (Uint8)(rgb >> 16);
                                c->g = (Uint8)(rgb >> 8);
                                c->b = (Uint8)(rgb);
                                pixel = indexc;
                        } else
                                pixel = rgb;
                        add_colorhash(colors, nextkey, cpp, pixel);
                        nextkey += cpp;
                        if (rgb == 0xffffffff)
                                SDL_SetColorKey(image, SDL_SRCCOLORKEY, pixel);
                        break;
                }
        }

        /* Read the pixels */
        pixels_len = w * cpp;
        dst = image->pixels;
        for (y = 0; y < h; y++) {
                line = get_next_line(xpmlines, src, pixels_len);
                if (indexed) {
                        /* optimization for some common cases */
                        if (cpp == 1)
                                for (x = 0; x < w; x++)
                                        dst[x] = (Uint8)QUICK_COLORHASH(colors,
                                                                 line + x);
                        else
                                for (x = 0; x < w; x++)
                                        dst[x] = (Uint8)get_colorhash(colors,
                                                               line + x * cpp,
                                                               cpp);
                } else {
                        for (x = 0; x < w; x++)
                                ((Uint32*)dst)[x] = get_colorhash(colors,
                                                                line + x * cpp,
                                                                  cpp);
                }
                dst += image->pitch;
        }

done:
        if (error) {
                SDL_FreeSurface(image);
                image = NULL;
                SDL_SetError(error);
        }
        free(keystrings);
        free_colorhash(colors);
        free(linebuf);
        return(image);
}

/* Load a XPM type image from an RWops datasource */
#if 0
SDL_Surface *IMG_LoadXPM_RW(SDL_RWops *src)
{
        if ( !src ) {
                /* The error message has been set in SDL_RWFromFile */
                return NULL;
        }
        return load_xpm(NULL, src);
}
#endif

static inline SDL_Surface *IMG_ReadXPMFromArray(const char **xpm)
{
        return load_xpm(xpm, NULL);
}

#else  /* not LOAD_XPM */

/* See if an image is contained in a data source */
#if 0
int IMG_isXPM(SDL_RWops *src)
{
        return(0);
}

/* Load a XPM type image from an SDL datasource */
SDL_Surface *IMG_LoadXPM_RW(SDL_RWops *src)
{
        return(NULL);
}
#endif

static inline SDL_Surface *IMG_ReadXPMFromArray(const char **xpm)
{
    return NULL;
}
#endif /* not LOAD_XPM */
