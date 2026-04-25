#include "example_common.h"
#include <glad/gles2.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void
set_identity16(float *m)
{
    int i;

    for (i = 0; i < 16; i++)
        m[i] = (i % 5) == 0 ? 1.0f : 0.0f;
}

static void
set_weight(RwSkin *skin, int vertex, int slot, uint8_t bone, float weight)
{
    skin->bone_indices[vertex * skin->num_weights + slot] = bone;
    skin->weights[vertex * skin->num_weights + slot] = weight;
}

static RwAtomic *
make_skinned_ribbon(RwFrame **frame_out, RwSkin **skin_out, RwHAnimHier **hier_out)
{
    static const float ys[5] = {-0.90f, -0.45f, 0.0f, 0.45f, 0.90f};
    RwGeometry *geo;
    RwAtomic *atomic;
    RwFrame *root;
    RwFrame *tip;
    RwSkin *skin;
    RwHAnimHier *hier;
    RwHAnimNodeInfo nodes[2] = {{0, 0, RW_HANIM_PUSH, NULL}, {1, 1, RW_HANIM_POP, NULL}};
    int i;

    geo = rw_geometry_create(10, 8, RW_GEO_POSITIONS | RW_GEO_PRELIT | RW_GEO_NORMALS);
    assert(geo);
    rw_geometry_lock(geo, RW_LOCK_ALL);
    for (i = 0; i < 5; i++) {
        float taper = 0.34f - fabsf(ys[i]) * 0.09f;
        geo->morph_target.vertices[i * 2 + 0] = (RwV3d){-taper, ys[i], 1.8f};
        geo->morph_target.vertices[i * 2 + 1] = (RwV3d){ taper, ys[i], 1.8f};
        geo->morph_target.normals[i * 2 + 0] = (RwV3d){0.0f, 0.0f, -1.0f};
        geo->morph_target.normals[i * 2 + 1] = (RwV3d){0.0f, 0.0f, -1.0f};
        geo->colors[i * 2 + 0] = (RwRGBA){255, (uint8_t)(90 + i * 32), 92, 255};
        geo->colors[i * 2 + 1] = (RwRGBA){64, (uint8_t)(170 + i * 16), 255, 255};
    }
    for (i = 0; i < 4; i++) {
        geo->triangles[i * 2 + 0] = (RwTriangle){{(uint16_t)(i * 2), (uint16_t)(i * 2 + 1), (uint16_t)(i * 2 + 3)}, 0};
        geo->triangles[i * 2 + 1] = (RwTriangle){{(uint16_t)(i * 2), (uint16_t)(i * 2 + 3), (uint16_t)(i * 2 + 2)}, 0};
    }
    rw_geometry_unlock(geo);
    rw_geometry_build_meshes(geo);
    rw_geometry_calc_bounding_sphere(geo);

    skin = rw_skin_create(2, 10, 4);
    assert(skin);
    set_identity16(&skin->inverse_matrices[0]);
    set_identity16(&skin->inverse_matrices[16]);
    for (i = 0; i < 4; i++) {
        set_weight(skin, i, 0, 0, 1.0f);
    }
    set_weight(skin, 4, 0, 0, 0.50f);
    set_weight(skin, 4, 1, 1, 0.50f);
    set_weight(skin, 5, 0, 0, 0.50f);
    set_weight(skin, 5, 1, 1, 0.50f);
    for (i = 6; i < 10; i++) {
        set_weight(skin, i, 0, 1, 1.0f);
    }
    geo->skin = skin;

    root = rw_frame_create();
    tip = rw_frame_create();
    hier = rw_hanim_create(2, nodes);
    atomic = rw_atomic_create();
    assert(root && tip && hier && atomic);
    rw_frame_add_child(root, tip);
    rw_hanim_attach(hier, root);
    rw_hanim_update_matrices(hier);
    rw_atomic_set_frame(atomic, root);
    rw_atomic_set_geometry(atomic, geo);
    rw_geometry_destroy(geo);
    rw_atomic_set_hanim_hierarchy(atomic, hier);
    rw_skin_set_pipeline(atomic);

    *frame_out = root;
    *skin_out = skin;
    *hier_out = hier;
    return atomic;
}

int
main(void)
{
    ExApp app;
    RwWorld *world;
    RwClump *clump;
    RwAtomic *ribbon;
    RwFrame *ribbon_frame;
    RwSkin *skin;
    RwHAnimHier *hier;
    RwLight *ambient;

    if (!ex_app_init(&app, "rw example: visible skin bend", 800, 600))
        return 1;

    world = rw_world_create();
    clump = rw_clump_create();
    ribbon = make_skinned_ribbon(&ribbon_frame, &skin, &hier);
    assert(world && clump && ribbon);
    rw_clump_add_atomic(clump, ribbon);
    rw_world_add_clump(world, clump);

    ambient = ex_make_light(RW_LIGHT_AMBIENT, (RwRGBAf){0.75f, 0.75f, 0.75f, 1.0f}, (RwV3d){0, 0, 0}, 0.0f, NULL);
    rw_world_add_light(world, ambient);
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);
    rw_set_render_state(RW_STATE_FOGENABLE, 0);

    printf("skin_dancer: ESC closes the window\n");
    while (ex_app_running(&app)) {
        float t = (float)glfwGetTime();
        float bend = sinf(t * 1.8f) * 0.60f;
        float qz = sinf(bend * 0.5f);
        float qw = cosf(bend * 0.5f);

        hier->keyframes[0].q = (RwQuat){0.0f, 0.0f, 0.0f, 1.0f};
        hier->keyframes[0].t = (RwV3d){0.0f, 0.0f, 0.0f};
        hier->keyframes[1].q = (RwQuat){0.0f, 0.0f, qz, qw};
        hier->keyframes[1].t = (RwV3d){sinf(t * 1.8f) * 0.20f, 0.0f, 0.0f};
        rw_hanim_update_matrices(hier);
        rw_frame_scale(ribbon_frame, (RwV3d){1.8f, 1.8f, 1.0f}, RW_COMBINE_REPLACE);

        ex_app_begin_frame(&app);
        glClearColor(0.015f, 0.015f, 0.035f, 1.0f);
        rw_camera_clear(app.camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
        rw_world_render(world);
        ex_app_swap(&app);
    }

    rw_clump_destroy(clump);
    rw_frame_destroy(ribbon_frame);
    rw_skin_destroy(skin);
    rw_hanim_destroy(hier);
    rw_light_destroy(ambient);
    rw_world_destroy(world);
    ex_app_shutdown(&app);
    return 0;
}
