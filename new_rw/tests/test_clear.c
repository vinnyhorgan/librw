#include "../rw.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void
expect_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t p[4];

    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
    fprintf(stderr, "pixel(%d,%d): R=%d G=%d B=%d A=%d\n", x, y, p[0], p[1], p[2], p[3]);
    assert(p[0] == r);
    assert(p[1] == g);
    assert(p[2] == b);
}

static void
test_camera_clear(void)
{
    GLFWwindow *win;
    RwCamera *camera;
    GLenum err;
    int w = 64, h = 64;

    win = glfwCreateWindow(w, h, "test_clear", NULL, NULL);
    assert(win);
    glfwMakeContextCurrent(win);
    gladLoadGLES2(glfwGetProcAddress);

    assert(rw_engine_init(NULL));
    assert(rw_engine_open());
    assert(rw_engine_start());

    camera = rw_camera_create();
    assert(camera);
    rw_camera_begin_update(camera);

    glViewport(0, 0, w, h);
    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    rw_camera_clear(camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
    expect_pixel(w / 2, h / 2, 64, 128, 191);

    err = glGetError();
    fprintf(stderr, "GL error after camera clear: 0x%x\n", err);
    assert(err == GL_NO_ERROR);

    rw_camera_destroy(camera);
    rw_engine_stop();
    rw_engine_close();
    rw_engine_term();

    glfwDestroyWindow(win);
    fprintf(stderr, "test_camera_clear: PASS\n");
}

int
main(void)
{
    assert(glfwInit());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    test_camera_clear();

    glfwTerminate();
    return 0;
}
