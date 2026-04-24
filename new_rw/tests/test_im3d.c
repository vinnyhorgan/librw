#include "../rw.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void
expect_pixel_nonzero(int x, int y)
{
    uint8_t p[4];

    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    fprintf(stderr, "pixel(%d,%d): R=%d G=%d B=%d A=%d\n", x, y, p[0], p[1], p[2], p[3]);
    assert(p[0] || p[1] || p[2]);
}

static RwCamera *
make_camera(void)
{
    RwCamera *camera = rw_camera_create();
    RwFrame *frame;

    assert(camera);
    frame = rw_frame_create();
    assert(frame);
    rw_camera_set_frame(camera, frame);
    rw_camera_set_near_far(camera, 0.1f, 100.0f);
    rw_camera_set_view_window(camera, 1.0f);
    rw_camera_begin_update(camera);
    return camera;
}

static void
destroy_camera(RwCamera *camera)
{
    RwFrame *frame = camera->frame;

    rw_camera_destroy(camera);
    rw_frame_destroy(frame);
}

static void
test_im3d_render(void)
{
    GLFWwindow *win;
    RwCamera *camera;
    RwWorld *world;
    RwLight *ambient;
    GLenum err;
    int w = 128, h = 128;
    RwIm3DVertex red[3] = {
        {{ 0.0f,  0.3f, 1.0f}, {0.0f, 0.0f, -1.0f}, 255, 0, 0, 255, 0.0f, 0.0f},
        {{-0.3f, -0.3f, 1.0f}, {0.0f, 0.0f, -1.0f}, 255, 0, 0, 255, 0.0f, 0.0f},
        {{ 0.3f, -0.3f, 1.0f}, {0.0f, 0.0f, -1.0f}, 255, 0, 0, 255, 0.0f, 0.0f},
    };
    RwIm3DVertex lit[4] = {
        {{-0.3f,  0.3f, 1.0f}, {0.0f, 0.0f, -1.0f}, 0, 0, 0, 255, 0.0f, 0.0f},
        {{ 0.3f,  0.3f, 1.0f}, {0.0f, 0.0f, -1.0f}, 0, 0, 0, 255, 0.0f, 0.0f},
        {{ 0.3f, -0.3f, 1.0f}, {0.0f, 0.0f, -1.0f}, 0, 0, 0, 255, 0.0f, 0.0f},
        {{-0.3f, -0.3f, 1.0f}, {0.0f, 0.0f, -1.0f}, 0, 0, 0, 255, 0.0f, 0.0f},
    };
    uint16_t indices[6] = {0, 1, 2, 0, 2, 3};

    win = glfwCreateWindow(w, h, "test_im3d", NULL, NULL);
    assert(win);
    glfwMakeContextCurrent(win);
    gladLoadGLES2(glfwGetProcAddress);

    assert(rw_engine_init(NULL));
    assert(rw_engine_open());
    assert(rw_engine_start());

    camera = make_camera();
    glViewport(0, 0, w, h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);

    rw_im3d_transform(red, 3, NULL, RW_IM3D_VERTEXXYZ | RW_IM3D_VERTEXRGBA);
    rw_im3d_render_primitive(RW_PRIM_TRILIST);
    rw_im3d_end();
    expect_pixel_nonzero(w / 2, h / 2);

    world = rw_world_create();
    assert(world);
    ambient = rw_light_create(RW_LIGHT_AMBIENT);
    assert(ambient);
    rw_light_set_color(ambient, (RwRGBAf){0.0f, 1.0f, 0.0f, 1.0f});
    rw_world_add_light(world, ambient);
    rw_world_render(world);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    rw_im3d_transform(lit, 4, NULL, RW_IM3D_VERTEXXYZ | RW_IM3D_VERTEXRGBA | RW_IM3D_LIGHTING);
    rw_im3d_render_indexed(RW_PRIM_TRILIST, indices, 6);
    rw_im3d_end();
    expect_pixel_nonzero(w / 2, h / 2);

    err = glGetError();
    fprintf(stderr, "GL error after im3d render: 0x%x\n", err);
    assert(err == GL_NO_ERROR);

    rw_light_destroy(ambient);
    rw_world_destroy(world);
    destroy_camera(camera);

    rw_engine_stop();
    rw_engine_close();
    rw_engine_term();

    glfwDestroyWindow(win);
    fprintf(stderr, "test_im3d_render: PASS\n");
}

int
main(void)
{
    assert(glfwInit());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    test_im3d_render();

    glfwTerminate();
    return 0;
}
