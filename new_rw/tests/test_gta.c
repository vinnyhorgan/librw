#include "../rw.h"
#include "../rw_gl_internal.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void
set_identity16(float *m)
{
    int i;

    for (i = 0; i < 16; i++)
        m[i] = (i % 5) == 0 ? 1.0f : 0.0f;
}

static RwAtomic *
make_colored_quad(float x0, float y0, float x1, float y1, float z, RwRGBA color)
{
    RwGeometry *geo;
    RwAtomic *atomic;
    RwFrame *frame;
    int i;

    geo = rw_geometry_create(4, 2, RW_GEO_POSITIONS | RW_GEO_PRELIT | RW_GEO_NORMALS);
    assert(geo);
    rw_geometry_lock(geo, RW_LOCK_ALL);
    geo->morph_target.vertices[0] = (RwV3d){x0, y0, z};
    geo->morph_target.vertices[1] = (RwV3d){x1, y0, z};
    geo->morph_target.vertices[2] = (RwV3d){x1, y1, z};
    geo->morph_target.vertices[3] = (RwV3d){x0, y1, z};
    for (i = 0; i < 4; i++) {
        geo->morph_target.normals[i] = (RwV3d){0.0f, 0.0f, -1.0f};
        geo->colors[i] = color;
    }
    geo->triangles[0].v[0] = 0;
    geo->triangles[0].v[1] = 1;
    geo->triangles[0].v[2] = 2;
    geo->triangles[1].v[0] = 0;
    geo->triangles[1].v[1] = 2;
    geo->triangles[1].v[2] = 3;
    rw_geometry_unlock(geo);
    assert(rw_geometry_build_meshes(geo));
    rw_geometry_calc_bounding_sphere(geo);
    rw_material_set_color(geo->materials[0], (RwRGBA){255, 255, 255, 255});

    atomic = rw_atomic_create();
    assert(atomic);
    frame = rw_frame_create();
    assert(frame);
    rw_atomic_set_frame(atomic, frame);
    rw_atomic_set_geometry(atomic, geo);
    rw_geometry_destroy(geo);
    rw_atomic_set_pipeline(atomic, rw_gl_default_pipeline());
    return atomic;
}

static RwAtomic *
make_skinned_character(RwSkin **skin_out, RwHAnimHier **hier_out)
{
    RwGeometry *geo;
    RwAtomic *atomic;
    RwFrame *frame;
    RwSkin *skin;
    RwHAnimHier *hier;
    RwHAnimNodeInfo node = {0, 0, 0, NULL};
    RwHAnimKeyFrame anim[2];
    int i;

    geo = rw_geometry_create(3, 1, RW_GEO_POSITIONS | RW_GEO_PRELIT | RW_GEO_NORMALS);
    assert(geo);
    rw_geometry_lock(geo, RW_LOCK_ALL);
    geo->morph_target.vertices[0] = (RwV3d){ 0.0f,  0.34f, 1.35f};
    geo->morph_target.vertices[1] = (RwV3d){-0.20f, -0.26f, 1.35f};
    geo->morph_target.vertices[2] = (RwV3d){ 0.20f, -0.26f, 1.35f};
    for (i = 0; i < 3; i++)
        geo->morph_target.normals[i] = (RwV3d){0.0f, 0.0f, -1.0f};
    geo->colors[0] = (RwRGBA){255, 245, 80, 255};
    geo->colors[1] = (RwRGBA){255, 60, 220, 255};
    geo->colors[2] = (RwRGBA){80, 220, 255, 255};
    geo->triangles[0].v[0] = 0;
    geo->triangles[0].v[1] = 1;
    geo->triangles[0].v[2] = 2;
    rw_geometry_unlock(geo);
    assert(rw_geometry_build_meshes(geo));
    rw_geometry_calc_bounding_sphere(geo);

    skin = rw_skin_create(1, 3, 4);
    assert(skin);
    set_identity16(skin->inverse_matrices);
    for (i = 0; i < 3; i++)
        skin->weights[i * 4] = 1.0f;
    geo->skin = skin;

    frame = rw_frame_create();
    assert(frame);
    hier = rw_hanim_create(1, &node);
    assert(hier);
    anim[0].q = (RwQuat){0.0f, 0.0f, 0.0f, 1.0f};
    anim[0].t = (RwV3d){-0.03f, 0.0f, 0.0f};
    anim[0].time = 0.0f;
    anim[1].q = (RwQuat){0.0f, 0.0f, 0.0f, 1.0f};
    anim[1].t = (RwV3d){0.03f, 0.0f, 0.0f};
    anim[1].time = 1.0f;
    hier->anim_data = anim;
    hier->num_frames = 2;
    hier->frame_rate = 1.0f;
    rw_hanim_attach(hier, frame);
    rw_hanim_interpolate(hier, 0.5f);
    rw_hanim_update_matrices(hier);
    hier->anim_data = NULL;

    atomic = rw_atomic_create();
    assert(atomic);
    rw_atomic_set_frame(atomic, frame);
    rw_atomic_set_geometry(atomic, geo);
    rw_geometry_destroy(geo);
    rw_atomic_set_hanim_hierarchy(atomic, hier);
    rw_skin_set_pipeline(atomic);

    *skin_out = skin;
    *hier_out = hier;
    return atomic;
}

static RwLight *
make_point_light(float x, float y, float z)
{
    RwLight *light;
    RwFrame *frame;
    RwMatrix matrix;

    light = rw_light_create(RW_LIGHT_POINT);
    assert(light);
    frame = rw_frame_create();
    assert(frame);
    rw_matrix_set_identity(&matrix);
    matrix.pos = (RwV3d){x, y, z};
    rw_frame_set_matrix(frame, &matrix);
    rw_light_set_frame(light, frame);
    rw_light_set_radius(light, 4.0f);
    rw_light_set_color(light, (RwRGBAf){0.35f, 0.35f, 0.30f, 1.0f});
    return light;
}

static void
add_city_atomic(RwClump *city, RwAtomic *atomic)
{
    rw_frame_add_child(rw_clump_get_frame(city), atomic->frame);
    rw_clump_add_atomic(city, atomic);
}

static void
draw_hud(void)
{
    RwIm2DVertex bar[4] = {
        {12.0f, 12.0f, 0.0f, 1.0f, 40, 220, 70, 255, 0.0f, 0.0f},
        {88.0f, 12.0f, 0.0f, 1.0f, 40, 220, 70, 255, 1.0f, 0.0f},
        {88.0f, 26.0f, 0.0f, 1.0f, 40, 220, 70, 255, 1.0f, 1.0f},
        {12.0f, 26.0f, 0.0f, 1.0f, 40, 220, 70, 255, 0.0f, 1.0f},
    };
    uint16_t indices[6] = {0, 1, 2, 0, 2, 3};

    rw_im2d_render_indexed(RW_PRIM_TRILIST, bar, 4, indices, 6);
}

int
main(void)
{
    GLFWwindow *win;
    RwWorld *world;
    RwClump *city;
    RwCamera *camera;
    RwFrame *camera_frame;
    RwLight *ambient;
    RwLight *point;
    RwFrame *point_frame;
    RwSkin *skin = NULL;
    RwHAnimHier *hier = NULL;
    uint8_t center[4];
    uint8_t hud[4];
    GLenum err;
    int w = 256;
    int h = 192;

    assert(glfwInit());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    win = glfwCreateWindow(w, h, "test_gta", NULL, NULL);
    assert(win);
    glfwMakeContextCurrent(win);
    gladLoadGLES2(glfwGetProcAddress);

    assert(rw_engine_init(NULL));
    assert(rw_engine_open());
    assert(rw_engine_start());

    glViewport(0, 0, w, h);
    glClearColor(0.05f, 0.08f, 0.12f, 1.0f);
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);
    rw_set_render_state(RW_STATE_FOGENABLE, 1);
    rw_set_render_state(RW_STATE_FOGCOLOR, 0xFF223040u);

    camera = rw_camera_create();
    assert(camera);
    camera_frame = rw_frame_create();
    assert(camera_frame);
    rw_camera_set_frame(camera, camera_frame);
    rw_camera_set_near_far(camera, 0.1f, 12.0f);
    rw_camera_set_view_window(camera, 0.75f);
    camera->fog_plane = 2.2f;
    rw_camera_begin_update(camera);

    world = rw_world_create();
    assert(world);
    city = rw_clump_create();
    assert(city);
    add_city_atomic(city, make_colored_quad(-1.6f, -0.72f, 1.6f, -0.25f, 2.0f, (RwRGBA){70, 80, 82, 255}));
    add_city_atomic(city, make_colored_quad(-0.68f, -0.24f, -0.26f, 0.60f, 2.6f, (RwRGBA){210, 135, 72, 255}));
    add_city_atomic(city, make_colored_quad(0.28f, -0.24f, 0.75f, 0.54f, 2.9f, (RwRGBA){78, 145, 205, 255}));
    add_city_atomic(city, make_skinned_character(&skin, &hier));
    rw_world_add_clump(world, city);

    ambient = rw_light_create(RW_LIGHT_AMBIENT);
    assert(ambient);
    rw_light_set_color(ambient, (RwRGBAf){0.08f, 0.08f, 0.08f, 1.0f});
    rw_world_add_light(world, ambient);
    point = make_point_light(0.0f, 0.2f, 0.7f);
    point_frame = point->frame;
    rw_world_add_light(world, point);

    rw_camera_clear(camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
    rw_world_render(world);
    draw_hud();

    err = glGetError();
    fprintf(stderr, "GL error after gta scene: 0x%x\n", err);
    assert(err == GL_NO_ERROR);

    glReadPixels(w / 2, h / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, center);
    glReadPixels(32, h - 18, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, hud);
    fprintf(stderr, "center pixel: R=%d G=%d B=%d A=%d\n", center[0], center[1], center[2], center[3]);
    fprintf(stderr, "hud pixel: R=%d G=%d B=%d A=%d\n", hud[0], hud[1], hud[2], hud[3]);
    assert(center[0] > 25 || center[1] > 25 || center[2] > 25);
    assert(hud[1] > 180 && hud[0] < 80 && hud[2] < 100);

    rw_clump_destroy(city);
    rw_skin_destroy(skin);
    rw_hanim_destroy(hier);
    rw_world_destroy(world);
    rw_light_destroy(ambient);
    rw_light_destroy(point);
    rw_frame_destroy(point_frame);
    rw_camera_destroy(camera);
    rw_frame_destroy(camera_frame);
    rw_engine_stop();
    rw_engine_close();
    rw_engine_term();

    glfwDestroyWindow(win);
    glfwTerminate();
    fprintf(stderr, "test_gta: PASS\n");
    return 0;
}
