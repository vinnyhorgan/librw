#include "rw.h"
#include "rw_gl_internal.h"

#include <string.h>

#define STBI_MALLOC(sz) rw_malloc(sz)
#define STBI_REALLOC(p, sz) rw_realloc(p, sz)
#define STBI_FREE(p) rw_free(p)
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb/stb_image.h"

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
    r->gl.dirty = 1;
    return r;
}

void
rw_raster_destroy(RwRaster *r)
{
    if (!r)
        return;

    rw_gl_delete_texture(&r->gl.texid);
    rw_free(r->pixels);
    rw_free(r);
}

uint8_t *
rw_raster_lock(RwRaster *r)
{
    size_t size;

    if (!r)
        return NULL;
    r->gl.dirty = 1;
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
    if (r)
        r->gl.dirty = 1;
}

RwImage *
rw_image_load(const char *filename)
{
    RwImage *img;
    int width, height, channels;
    uint8_t *pixels;

    if (!filename)
        return NULL;

    pixels = stbi_load(filename, &width, &height, &channels, 4);
    if (!pixels)
        return NULL;

    img = rw_malloc(sizeof(*img));
    if (!img) {
        stbi_image_free(pixels);
        return NULL;
    }

    memset(img, 0, sizeof(*img));
    img->width = width;
    img->height = height;
    img->depth = 32;
    img->bpp = 4;
    img->stride = width * 4;
    img->pixels = pixels;
    (void)channels;
    return img;
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
    r->gl.dirty = 1;
    return r;
}
