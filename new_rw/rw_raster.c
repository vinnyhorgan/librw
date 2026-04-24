#include "rw.h"

#include <string.h>

RwRaster *
rw_raster_create(int w, int h, int depth, uint32_t flags)
{
    RwRaster *r;

    if (w < 0 || h < 0 || depth < 0)
        return NULL;

    r = rw_malloc(sizeof(*r));
    if (!r)
        return NULL;

    memset(r, 0, sizeof(*r));
    r->width = w;
    r->height = h;
    r->depth = depth;
    r->flags = flags;
    r->gl.filter = RW_TEX_FILTER_NEAREST;
    r->gl.address_u = RW_TEX_WRAP_WRAP;
    r->gl.address_v = RW_TEX_WRAP_WRAP;
    return r;
}

void
rw_raster_destroy(RwRaster *r)
{
    if (!r)
        return;

    rw_free(r->pixels);
    rw_free(r);
}

uint8_t *
rw_raster_lock(RwRaster *r)
{
    size_t size;

    if (!r)
        return NULL;
    if (r->pixels)
        return r->pixels;

    size = (size_t)r->width * (size_t)r->height * (size_t)((r->depth + 7) / 8);
    if (size == 0)
        return NULL;

    r->pixels = rw_malloc(size);
    if (r->pixels)
        memset(r->pixels, 0, size);
    return r->pixels;
}

void
rw_raster_unlock(RwRaster *r)
{
    (void)r;
}

RwImage *
rw_image_load(const char *filename)
{
    (void)filename;
    return NULL;
}

void
rw_image_destroy(RwImage *img)
{
    if (!img)
        return;

    rw_free(img->pixels);
    rw_free(img);
}

RwRaster *
rw_raster_from_image(RwImage *img)
{
    RwRaster *r;
    size_t size;

    if (!img || !img->pixels)
        return NULL;

    r = rw_raster_create(img->width, img->height, img->depth, 0);
    if (!r)
        return NULL;

    size = (size_t)img->stride * (size_t)img->height;
    r->pixels = rw_malloc(size);
    if (!r->pixels) {
        rw_raster_destroy(r);
        return NULL;
    }
    memcpy(r->pixels, img->pixels, size);
    return r;
}
