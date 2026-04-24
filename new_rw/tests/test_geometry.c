#include "../rw.h"

#include <assert.h>
#include <math.h>

static int
near(float a, float b)
{
    return fabsf(a - b) < 0.0001f;
}

static void
test_geometry_create_defaults(void)
{
    RwGeometry *geo = rw_geometry_create(3, 1, RW_GEO_POSITIONS | RW_GEO_NORMALS | RW_GEO_PRELIT | RW_GEO_TEXTURED);

    assert(geo);
    assert(geo->num_vertices == 3);
    assert(geo->num_triangles == 1);
    assert(geo->triangles);
    assert(geo->morph_target.vertices);
    assert(geo->morph_target.normals);
    assert(geo->colors);
    assert(geo->texcoords);
    assert(geo->num_materials == 1);
    assert(geo->materials[0]);
    assert(geo->materials[0]->color.r == 255);

    rw_geometry_destroy(geo);
}

static void
test_geometry_build_meshes(void)
{
    RwGeometry *geo = rw_geometry_create(4, 3, RW_GEO_POSITIONS);
    RwMesh *meshes;

    assert(geo);
    geo->mat_space = 2;
    geo->materials = rw_realloc(geo->materials, 2 * sizeof(*geo->materials));
    assert(geo->materials);
    geo->materials[1] = rw_material_create();
    assert(geo->materials[1]);
    geo->num_materials = 2;

    geo->triangles[0].v[0] = 0; geo->triangles[0].v[1] = 1; geo->triangles[0].v[2] = 2; geo->triangles[0].mat_id = 0;
    geo->triangles[1].v[0] = 2; geo->triangles[1].v[1] = 1; geo->triangles[1].v[2] = 3; geo->triangles[1].mat_id = 1;
    geo->triangles[2].v[0] = 3; geo->triangles[2].v[1] = 0; geo->triangles[2].v[2] = 2; geo->triangles[2].mat_id = 0;

    assert(rw_geometry_build_meshes(geo));
    assert(geo->mesh_header);
    assert(geo->mesh_header->num_meshes == 2);
    assert(geo->mesh_header->total_indices == 9);
    meshes = (RwMesh *)(geo->mesh_header + 1);
    assert(meshes[0].material == geo->materials[0]);
    assert(meshes[0].num_indices == 6);
    assert(meshes[0].indices[0] == 0 && meshes[0].indices[1] == 1 && meshes[0].indices[2] == 2);
    assert(meshes[0].indices[3] == 3 && meshes[0].indices[4] == 0 && meshes[0].indices[5] == 2);
    assert(meshes[1].material == geo->materials[1]);
    assert(meshes[1].num_indices == 3);
    assert(meshes[1].indices[0] == 2 && meshes[1].indices[1] == 1 && meshes[1].indices[2] == 3);

    rw_geometry_lock(geo, RW_LOCK_POLYGONS);
    assert(geo->mesh_header == NULL);
    rw_geometry_unlock(geo);
    assert(geo->mesh_header != NULL);

    rw_geometry_destroy(geo);
}

static void
test_geometry_bounding_sphere(void)
{
    RwGeometry *geo = rw_geometry_create(2, 0, RW_GEO_POSITIONS);

    assert(geo);
    geo->morph_target.vertices[0] = (RwV3d){-1.0f, -2.0f, 0.0f};
    geo->morph_target.vertices[1] = (RwV3d){3.0f, 2.0f, 0.0f};
    rw_geometry_calc_bounding_sphere(geo);
    assert(near(geo->morph_target.bounding_sphere.center.x, 1.0f));
    assert(near(geo->morph_target.bounding_sphere.center.y, 0.0f));
    assert(near(geo->morph_target.bounding_sphere.center.z, 0.0f));
    assert(near(geo->morph_target.bounding_sphere.radius, sqrtf(8.0f)));

    rw_geometry_destroy(geo);
}

int
main(void)
{
    assert(rw_engine_init(NULL));
    test_geometry_create_defaults();
    test_geometry_build_meshes();
    test_geometry_bounding_sphere();
    rw_engine_term();
    return 0;
}
