#include "../rw.h"
#include "../rw_gl_internal.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static RwGeometry *
make_triangle_geometry(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    RwGeometry *geo;
    RwMaterial *mat;

    geo = rw_geometry_create(3, 1, RW_GEO_POSITIONS | RW_GEO_PRELIT | RW_GEO_NORMALS);
    assert(geo);
    rw_geometry_lock(geo, RW_LOCK_ALL);
    geo->morph_target.vertices[0] = (RwV3d){ 0.0f,  0.6f, 1.0f};
    geo->morph_target.vertices[1] = (RwV3d){-0.6f, -0.6f, 1.0f};
    geo->morph_target.vertices[2] = (RwV3d){ 0.6f, -0.6f, 1.0f};
    geo->morph_target.normals[0] = (RwV3d){0.0f, 0.0f, -1.0f};
    geo->morph_target.normals[1] = (RwV3d){0.0f, 0.0f, -1.0f};
    geo->morph_target.normals[2] = (RwV3d){0.0f, 0.0f, -1.0f};
    geo->colors[0] = (RwRGBA){255, 255, 255, 255};
    geo->colors[1] = (RwRGBA){255, 255, 255, 255};
    geo->colors[2] = (RwRGBA){255, 255, 255, 255};
    geo->triangles[0].v[0] = 0;
    geo->triangles[0].v[1] = 1;
    geo->triangles[0].v[2] = 2;
    geo->triangles[0].mat_id = 0;
    rw_geometry_unlock(geo);

    mat = rw_material_create();
    assert(mat);
    rw_material_set_color(mat, (RwRGBA){r, g, b, a});
    rw_material_destroy(geo->materials[0]);
    geo->materials[0] = mat;
    assert(rw_geometry_build_meshes(geo));
    rw_geometry_calc_bounding_sphere(geo);
    return geo;
}

static void
draw_triangle(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    RwGeometry *geo;
    RwAtomic *atomic;
    RwFrame *atomic_frame;
    RwClump *clump;
    RwWorld *world;

    world = rw_world_create();
    clump = rw_clump_create();
    atomic = rw_atomic_create();
    atomic_frame = rw_frame_create();
    assert(world && clump && atomic && atomic_frame);

    geo = make_triangle_geometry(r, g, b, a);
    rw_atomic_set_geometry(atomic, geo);
    rw_geometry_destroy(geo);
    rw_atomic_set_frame(atomic, atomic_frame);
    rw_atomic_set_pipeline(atomic, rw_gl_default_pipeline());
    rw_clump_add_atomic(clump, atomic);
    rw_world_add_clump(world, clump);

    rw_world_render(world);

    rw_clump_destroy(clump);
    rw_frame_destroy(atomic_frame);
    rw_world_destroy(world);
}

static void
expect_center(int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t p[4];

    glReadPixels(w / 2, h / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    fprintf(stderr, "center pixel: R=%d G=%d B=%d A=%d\n", p[0], p[1], p[2], p[3]);
    assert(p[0] == r);
    assert(p[1] == g);
    assert(p[2] == b);
}

static void
expect_center_near(int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t p[4];

    glReadPixels(w / 2, h / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    fprintf(stderr, "center pixel: R=%d G=%d B=%d A=%d\n", p[0], p[1], p[2], p[3]);
    assert(p[0] >= r - 1 && p[0] <= r + 1);
    assert(p[1] >= g - 1 && p[1] <= g + 1);
    assert(p[2] >= b - 1 && p[2] <= b + 1);
}

static void
test_triangle_render_states(void)
{
    GLFWwindow *win;
    RwCamera *camera;
    RwFrame *camera_frame;
    GLenum err;
    int w = 96, h = 96;

    win = glfwCreateWindow(w, h, "test_triangle", NULL, NULL);
    assert(win);
    glfwMakeContextCurrent(win);
    gladLoadGLES2(glfwGetProcAddress);

    assert(rw_engine_init(NULL));
    assert(rw_engine_open());
    assert(rw_engine_start());

    camera = rw_camera_create();
    camera_frame = rw_frame_create();
    assert(camera && camera_frame);
    rw_camera_set_frame(camera, camera_frame);
    rw_camera_begin_update(camera);

    glViewport(0, 0, w, h);
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);
    rw_set_render_state(RW_STATE_ZWRITEENABLE, 0);

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    rw_camera_clear(camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
    rw_set_render_state(RW_STATE_SRCBLEND, RW_BLEND_ONE);
    rw_set_render_state(RW_STATE_DESTBLEND, RW_BLEND_ZERO);
    draw_triangle(255, 0, 0, 255);
    expect_center(w, h, 255, 0, 0);

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    rw_camera_clear(camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
    rw_set_render_state(RW_STATE_ALPHATEST, 1);
    rw_set_render_state(RW_STATE_ALPHAREF, 200);
    draw_triangle(255, 0, 0, 128);
    expect_center(w, h, 0, 0, 255);
    rw_set_render_state(RW_STATE_ALPHATEST, 0);

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    rw_camera_clear(camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
    rw_set_render_state(RW_STATE_SRCBLEND, RW_BLEND_SRCALPHA);
    rw_set_render_state(RW_STATE_DESTBLEND, RW_BLEND_INVSRCALPHA);
    draw_triangle(255, 0, 0, 128);
    expect_center_near(w, h, 128, 0, 127);

    err = glGetError();
    fprintf(stderr, "GL error after triangle states: 0x%x\n", err);
    assert(err == GL_NO_ERROR);

    rw_camera_destroy(camera);
    rw_frame_destroy(camera_frame);
    rw_engine_stop();
    rw_engine_close();
    rw_engine_term();

    glfwDestroyWindow(win);
    fprintf(stderr, "test_triangle_render_states: PASS\n");
}

int
main(void)
{
    assert(glfwInit());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    test_triangle_render_states();

    glfwTerminate();
    return 0;
}
