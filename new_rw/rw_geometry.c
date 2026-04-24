#include "rw.h"

#include <string.h>

static void
rw_geometry_free_meshes(RwGeometry *geo)
{
    rw_free(geo->mesh_header);
    geo->mesh_header = NULL;
}

static int
rw_geometry_alloc_data(RwGeometry *geo)
{
    if (geo->num_triangles) {
        geo->triangles = rw_malloc((size_t)geo->num_triangles * sizeof(*geo->triangles));
        if (!geo->triangles)
            return 0;
        memset(geo->triangles, 0, (size_t)geo->num_triangles * sizeof(*geo->triangles));
    }

    if (geo->num_vertices) {
        geo->morph_target.vertices = rw_malloc((size_t)geo->num_vertices * sizeof(*geo->morph_target.vertices));
        if (!geo->morph_target.vertices)
            return 0;
        memset(geo->morph_target.vertices, 0, (size_t)geo->num_vertices * sizeof(*geo->morph_target.vertices));
    }

    if ((geo->geo_flags & RW_GEO_NORMALS) && geo->num_vertices) {
        geo->morph_target.normals = rw_malloc((size_t)geo->num_vertices * sizeof(*geo->morph_target.normals));
        if (!geo->morph_target.normals)
            return 0;
        memset(geo->morph_target.normals, 0, (size_t)geo->num_vertices * sizeof(*geo->morph_target.normals));
    }

    if ((geo->geo_flags & RW_GEO_PRELIT) && geo->num_vertices) {
        geo->colors = rw_malloc((size_t)geo->num_vertices * sizeof(*geo->colors));
        if (!geo->colors)
            return 0;
        memset(geo->colors, 0xFF, (size_t)geo->num_vertices * sizeof(*geo->colors));
    }

    if ((geo->geo_flags & RW_GEO_TEXTURED) && geo->num_vertices) {
        geo->texcoords = rw_malloc((size_t)geo->num_vertices * sizeof(*geo->texcoords));
        if (!geo->texcoords)
            return 0;
        memset(geo->texcoords, 0, (size_t)geo->num_vertices * sizeof(*geo->texcoords));
    }

    return 1;
}

RwGeometry *
rw_geometry_create(int num_verts, int num_tris, uint32_t flags)
{
    RwGeometry *geo;

    if (num_verts < 0 || num_verts > UINT16_MAX || num_tris < 0)
        return NULL;

    geo = rw_malloc(sizeof(*geo));
    if (!geo)
        return NULL;

    memset(geo, 0, sizeof(*geo));
    geo->geo_flags = flags;
    geo->num_vertices = num_verts;
    geo->num_triangles = num_tris;
    geo->ref_count = 1;
    geo->mat_space = 1;
    geo->materials = rw_malloc(sizeof(*geo->materials));
    if (!geo->materials)
        goto fail;
    geo->materials[0] = rw_material_create();
    if (!geo->materials[0])
        goto fail;
    geo->num_materials = 1;

    if (!rw_geometry_alloc_data(geo))
        goto fail;

    return geo;

fail:
    rw_geometry_destroy(geo);
    return NULL;
}

void
rw_geometry_destroy(RwGeometry *geo)
{
    int i;

    if (!geo)
        return;

    geo->ref_count--;
    if (geo->ref_count > 0)
        return;

    rw_free(geo->triangles);
    rw_free(geo->colors);
    rw_free(geo->texcoords);
    rw_free(geo->morph_target.vertices);
    rw_free(geo->morph_target.normals);
    rw_geometry_free_meshes(geo);
    if (geo->materials) {
        for (i = 0; i < geo->num_materials; i++)
            rw_material_destroy(geo->materials[i]);
        rw_free(geo->materials);
    }
    rw_free(geo);
}

void
rw_geometry_lock(RwGeometry *geo, uint32_t lock_flags)
{
    if (!geo)
        return;

    geo->locked |= lock_flags;
    if (lock_flags & RW_LOCK_POLYGONS)
        rw_geometry_free_meshes(geo);
}

void
rw_geometry_unlock(RwGeometry *geo)
{
    if (!geo)
        return;

    if (!geo->mesh_header)
        rw_geometry_build_meshes(geo);
    geo->locked = 0;
}

int
rw_geometry_build_meshes(RwGeometry *geo)
{
    int i;
    uint32_t total_indices;
    uint32_t *counts;
    uint16_t *indices;
    RwMesh *meshes;
    size_t size;

    if (!geo || !geo->triangles || geo->num_materials <= 0)
        return 0;
    if (geo->geo_flags & RW_GEO_TRISTRIP)
        return 0;

    counts = rw_malloc((size_t)geo->num_materials * sizeof(*counts));
    if (!counts)
        return 0;
    memset(counts, 0, (size_t)geo->num_materials * sizeof(*counts));

    for (i = 0; i < geo->num_triangles; i++) {
        if (geo->triangles[i].mat_id >= (uint16_t)geo->num_materials) {
            rw_free(counts);
            return 0;
        }
        if (geo->triangles[i].v[0] >= (uint16_t)geo->num_vertices ||
            geo->triangles[i].v[1] >= (uint16_t)geo->num_vertices ||
            geo->triangles[i].v[2] >= (uint16_t)geo->num_vertices) {
            rw_free(counts);
            return 0;
        }
        counts[geo->triangles[i].mat_id] += 3;
    }

    total_indices = (uint32_t)geo->num_triangles * 3u;
    size = sizeof(RwMeshHeader) + (size_t)geo->num_materials * sizeof(RwMesh) + (size_t)total_indices * sizeof(uint16_t);
    rw_geometry_free_meshes(geo);
    geo->mesh_header = rw_malloc(size);
    if (!geo->mesh_header) {
        rw_free(counts);
        return 0;
    }
    memset(geo->mesh_header, 0, size);

    geo->mesh_header->flags = 0;
    geo->mesh_header->num_meshes = (uint16_t)geo->num_materials;
    geo->mesh_header->total_indices = total_indices;

    meshes = (RwMesh *)(geo->mesh_header + 1);
    indices = (uint16_t *)(meshes + geo->num_materials);
    for (i = 0; i < geo->num_materials; i++) {
        meshes[i].indices = indices;
        meshes[i].num_indices = counts[i];
        meshes[i].material = geo->materials[i];
        indices += counts[i];
        counts[i] = 0;
    }

    for (i = 0; i < geo->num_triangles; i++) {
        RwTriangle *tri = &geo->triangles[i];
        RwMesh *mesh = &meshes[tri->mat_id];
        uint32_t idx = counts[tri->mat_id];

        mesh->indices[idx++] = tri->v[0];
        mesh->indices[idx++] = tri->v[1];
        mesh->indices[idx++] = tri->v[2];
        counts[tri->mat_id] = idx;
    }

    rw_free(counts);
    return 1;
}

void
rw_geometry_calc_bounding_sphere(RwGeometry *geo)
{
    int i;
    RwV3d min, max, ext;

    if (!geo || !geo->morph_target.vertices || geo->num_vertices <= 0) {
        if (geo) {
            geo->morph_target.bounding_sphere.center = (RwV3d){0, 0, 0};
            geo->morph_target.bounding_sphere.radius = 0.0f;
        }
        return;
    }

    min = geo->morph_target.vertices[0];
    max = geo->morph_target.vertices[0];
    for (i = 1; i < geo->num_vertices; i++) {
        RwV3d v = geo->morph_target.vertices[i];
        if (v.x < min.x) min.x = v.x;
        if (v.y < min.y) min.y = v.y;
        if (v.z < min.z) min.z = v.z;
        if (v.x > max.x) max.x = v.x;
        if (v.y > max.y) max.y = v.y;
        if (v.z > max.z) max.z = v.z;
    }

    geo->morph_target.bounding_sphere.center = rw_v3d_scale(rw_v3d_add(min, max), 0.5f);
    ext = rw_v3d_sub(max, geo->morph_target.bounding_sphere.center);
    geo->morph_target.bounding_sphere.radius = rw_v3d_length(ext);
}
