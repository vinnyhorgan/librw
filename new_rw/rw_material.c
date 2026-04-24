#include "rw.h"

#include <string.h>

#define RW_TEX_FILTER_MASK 0xFFu
#define RW_TEX_ADDRESS_U_MASK 0xF00u
#define RW_TEX_ADDRESS_V_MASK 0xF000u

RwMaterial *
rw_material_create(void)
{
    RwMaterial *mat = rw_malloc(sizeof(*mat));

    if (!mat)
        return NULL;

    memset(mat, 0, sizeof(*mat));
    mat->color.r = 255;
    mat->color.g = 255;
    mat->color.b = 255;
    mat->color.a = 255;
    mat->surface.ambient = 1.0f;
    mat->surface.specular = 1.0f;
    mat->surface.diffuse = 1.0f;
    mat->ref_count = 1;
    return mat;
}

void
rw_material_destroy(RwMaterial *mat)
{
    if (!mat)
        return;

    mat->ref_count--;
    if (mat->ref_count > 0)
        return;

    if (mat->texture)
        rw_texture_destroy(mat->texture);
    rw_free(mat);
}

void
rw_material_set_texture(RwMaterial *mat, RwTexture *tex)
{
    if (!mat || mat->texture == tex)
        return;

    if (tex)
        tex->ref_count++;
    if (mat->texture)
        rw_texture_destroy(mat->texture);
    mat->texture = tex;
}

void
rw_material_set_color(RwMaterial *mat, RwRGBA color)
{
    if (mat)
        mat->color = color;
}

void
rw_material_set_surface(RwMaterial *mat, float amb, float spec, float diff)
{
    if (!mat)
        return;

    mat->surface.ambient = amb;
    mat->surface.specular = spec;
    mat->surface.diffuse = diff;
}

RwTexture *
rw_texture_create(RwRaster *raster)
{
    RwTexture *tex = rw_malloc(sizeof(*tex));

    if (!tex)
        return NULL;

    memset(tex, 0, sizeof(*tex));
    tex->raster = raster;
    rw_link_init(&tex->in_dict);
    tex->filter_addressing = (RW_TEX_WRAP_WRAP << 12) | (RW_TEX_WRAP_WRAP << 8) | RW_TEX_FILTER_NEAREST;
    tex->ref_count = 1;
    return tex;
}

void
rw_texture_destroy(RwTexture *tex)
{
    if (!tex)
        return;

    tex->ref_count--;
    if (tex->ref_count > 0)
        return;

    if (tex->dict) {
        rw_link_remove(&tex->in_dict);
        tex->dict = NULL;
    }
    if (tex->raster)
        rw_raster_destroy(tex->raster);
    rw_free(tex);
}

void
rw_texture_set_filter(RwTexture *tex, int filter)
{
    if (!tex)
        return;

    tex->filter_addressing = (tex->filter_addressing & ~RW_TEX_FILTER_MASK) | (uint32_t)(filter & RW_TEX_FILTER_MASK);
}

void
rw_texture_set_addressing(RwTexture *tex, int u, int v)
{
    if (!tex)
        return;

    tex->filter_addressing = (tex->filter_addressing & ~(RW_TEX_ADDRESS_U_MASK | RW_TEX_ADDRESS_V_MASK)) |
                             ((uint32_t)(u & 0xF) << 8) |
                             ((uint32_t)(v & 0xF) << 12);
}

RwTexDict *
rw_texdict_create(void)
{
    RwTexDict *td = rw_malloc(sizeof(*td));

    if (!td)
        return NULL;

    memset(td, 0, sizeof(*td));
    rw_linklist_init(&td->textures);
    rw_link_init(&td->in_global);
    return td;
}

void
rw_texdict_destroy(RwTexDict *td)
{
    RwLink *link;

    if (!td)
        return;

    while (!rw_linklist_isempty(&td->textures)) {
        RwTexture *tex;

        link = td->textures.link.next;
        tex = RW_LINK_DATA(link, RwTexture, in_dict);
        tex->dict = NULL;
        rw_link_remove(link);
        rw_texture_destroy(tex);
    }
    rw_free(td);
}

void
rw_texdict_add(RwTexDict *td, RwTexture *tex)
{
    if (!td || !tex)
        return;

    if (tex->dict)
        rw_link_remove(&tex->in_dict);
    tex->dict = td;
    rw_linklist_append(&td->textures, &tex->in_dict);
}

RwTexture *
rw_texdict_find(RwTexDict *td, const char *name)
{
    RwLink *link;

    if (!td || !name)
        return NULL;

    RW_FORLIST(link, td->textures) {
        RwTexture *tex = RW_LINK_DATA(link, RwTexture, in_dict);
        if (strncmp(tex->name, name, sizeof(tex->name)) == 0)
            return tex;
    }
    return NULL;
}
