#include "../rw.h"

#include <assert.h>
#include <string.h>

static void
test_material_defaults_and_texture_ref(void)
{
    RwMaterial *mat = rw_material_create();
    RwTexture *tex = rw_texture_create(NULL);
    RwRGBA color = {1, 2, 3, 4};

    assert(mat);
    assert(tex);
    assert(mat->color.r == 255 && mat->color.g == 255 && mat->color.b == 255 && mat->color.a == 255);
    assert(mat->surface.ambient == 1.0f && mat->surface.specular == 1.0f && mat->surface.diffuse == 1.0f);

    rw_material_set_color(mat, color);
    rw_material_set_surface(mat, 0.2f, 0.3f, 0.4f);
    assert(mat->color.r == 1 && mat->color.g == 2 && mat->color.b == 3 && mat->color.a == 4);
    assert(mat->surface.ambient == 0.2f && mat->surface.specular == 0.3f && mat->surface.diffuse == 0.4f);

    rw_material_set_texture(mat, tex);
    assert(mat->texture == tex);
    assert(tex->ref_count == 2);
    rw_material_set_texture(mat, NULL);
    assert(mat->texture == NULL);
    assert(tex->ref_count == 1);

    rw_texture_destroy(tex);
    rw_material_destroy(mat);
}

static void
test_texdict_add_find_destroy(void)
{
    RwTexDict *td = rw_texdict_create();
    RwTexture *tex = rw_texture_create(NULL);

    assert(td);
    assert(tex);
    memcpy(tex->name, "white", 6);

    rw_texdict_add(td, tex);
    assert(tex->dict == td);
    assert(rw_texdict_find(td, "white") == tex);
    assert(rw_texdict_find(td, "missing") == NULL);

    rw_texdict_destroy(td);
}

static void
test_raster_lock_and_copy_image(void)
{
    RwRaster *r = rw_raster_create(2, 2, 32, 0);
    RwImage *img;
    RwRaster *copy;
    uint8_t *pixels;

    assert(r);
    pixels = rw_raster_lock(r);
    assert(pixels);
    pixels[0] = 42;
    assert(rw_raster_lock(r) == pixels);
    rw_raster_destroy(r);

    img = rw_malloc(sizeof(*img));
    assert(img);
    memset(img, 0, sizeof(*img));
    img->width = 1;
    img->height = 1;
    img->depth = 32;
    img->bpp = 4;
    img->stride = 4;
    img->pixels = rw_malloc(4);
    assert(img->pixels);
    img->pixels[0] = 7;
    img->pixels[1] = 8;
    img->pixels[2] = 9;
    img->pixels[3] = 10;

    copy = rw_raster_from_image(img);
    assert(copy);
    assert(copy->pixels[0] == 7 && copy->pixels[1] == 8 && copy->pixels[2] == 9 && copy->pixels[3] == 10);
    rw_raster_destroy(copy);
    rw_image_destroy(img);
}

int
main(void)
{
    assert(rw_engine_init(NULL));
    test_material_defaults_and_texture_ref();
    test_texdict_add_find_destroy();
    test_raster_lock_and_copy_image();
    rw_engine_term();
    return 0;
}
