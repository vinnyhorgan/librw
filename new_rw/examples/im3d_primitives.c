#include "example_common.h"
#include <glad/gles2.h>

#include <math.h>
#include <stdio.h>

static RwIm3DVertex
v3(float x, float y, float z, RwRGBA c)
{
    return (RwIm3DVertex){{x, y, z}, {0.0f, 0.0f, -1.0f}, c.r, c.g, c.b, c.a, 0.0f, 0.0f};
}

static void
draw_fan(float cx, float cy, float z, float t)
{
    RwIm3DVertex v[18];
    int i;

    v[0] = v3(cx, cy, z, (RwRGBA){255, 255, 255, 255});
    for (i = 0; i < 17; i++) {
        float a = t + (float)i * 0.39269908f;
        v[i + 1] = v3(cx + cosf(a) * 0.52f, cy + sinf(a) * 0.52f, z,
                      (RwRGBA){255, 216, 72, 255});
    }
    rw_im3d_transform(v, 18, NULL, RW_IM3D_VERTEXXYZ | RW_IM3D_VERTEXRGBA);
    rw_im3d_render_primitive(RW_PRIM_TRIFAN);
    rw_im3d_end();
}

static void
draw_strip(float cx, float cy, float z, float t)
{
    RwIm3DVertex v[18];
    int i;

    for (i = 0; i < 9; i++) {
        float a = t + (float)i * 0.78539816f;
        v[i * 2 + 0] = v3(cx + cosf(a) * 0.56f, cy + sinf(a) * 0.56f, z,
                          (RwRGBA){218, 105, 255, 255});
        v[i * 2 + 1] = v3(cx + cosf(a) * 0.24f, cy + sinf(a) * 0.24f, z,
                          (RwRGBA){105, 245, 255, 255});
    }
    rw_im3d_transform(v, 18, NULL, RW_IM3D_VERTEXXYZ | RW_IM3D_VERTEXRGBA);
    rw_im3d_render_primitive(RW_PRIM_TRISTRIP);
    rw_im3d_end();
}

static void
draw_lines(float cx, float cy, float z, float t)
{
    RwIm3DVertex v[32];
    int i;

    for (i = 0; i < 16; i++) {
        float a = t + (float)i * 0.39269908f;
        v[i * 2 + 0] = v3(cx, cy, z, (RwRGBA){255, 90, 90, 255});
        v[i * 2 + 1] = v3(cx + cosf(a) * 0.58f, cy + sinf(a) * 0.58f, z,
                          (RwRGBA){255, 255, 255, 255});
    }
    rw_im3d_transform(v, 32, NULL, RW_IM3D_VERTEXXYZ | RW_IM3D_VERTEXRGBA);
    rw_im3d_render_primitive(RW_PRIM_LINELIST);
    rw_im3d_end();
}

static void
draw_indexed_triangles(float cx, float cy, float z, float t)
{
    RwIm3DVertex v[11];
    uint16_t idx[30];
    int i;

    v[0] = v3(cx, cy, z, (RwRGBA){255, 255, 255, 255});
    for (i = 0; i < 10; i++) {
        float a = t + (float)i * 0.62831853f;
        float r = (i & 1) ? 0.24f : 0.62f;
        RwRGBA c = (i & 1) ? (RwRGBA){115, 255, 180, 255} : (RwRGBA){70, 190, 255, 255};
        v[i + 1] = v3(cx + cosf(a) * r, cy + sinf(a) * r, z, c);
        idx[i * 3 + 0] = 0;
        idx[i * 3 + 1] = (uint16_t)(i + 1);
        idx[i * 3 + 2] = (uint16_t)(i == 9 ? 1 : i + 2);
    }
    rw_im3d_transform(v, 11, NULL, RW_IM3D_VERTEXXYZ | RW_IM3D_VERTEXRGBA);
    rw_im3d_render_indexed(RW_PRIM_TRILIST, idx, 30);
    rw_im3d_end();
}

int
main(void)
{
    ExApp app;

    if (!ex_app_init(&app, "rw example: ported im3d primitives", 960, 640))
        return 1;
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);
    rw_set_render_state_ptr(RW_STATE_TEXTURERASTER, NULL);

    printf("im3d_primitives: ESC closes the window\n");
    while (ex_app_running(&app)) {
        float t = (float)glfwGetTime() * 0.7f;

        ex_app_begin_frame(&app);
        glClearColor(0.01f, 0.012f, 0.025f, 1.0f);
        rw_camera_clear(app.camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
        draw_lines(-0.58f, 0.42f, 1.55f, t);
        draw_strip(0.58f, 0.42f, 1.55f, -t);
        draw_fan(-0.58f, -0.42f, 1.55f, t * 1.3f);
        draw_indexed_triangles(0.58f, -0.42f, 1.55f, -t * 1.1f);
        ex_app_swap(&app);
    }

    ex_app_shutdown(&app);
    return 0;
}
