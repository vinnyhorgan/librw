#include "example_common.h"
#include <glad/gles2.h>

#include <math.h>
#include <stdio.h>

static RwIm2DVertex
v2(float x, float y, RwRGBA c)
{
    return (RwIm2DVertex){x, y, 0.0f, 1.0f, c.r, c.g, c.b, c.a, 0.0f, 0.0f};
}

static void
shape_vertex(RwIm2DVertex *v, float cx, float cy, float scale, float x, float y, RwRGBA c)
{
    *v = v2(cx + x * scale, cy - y * scale, c);
}

static void
draw_line_list(float cx, float cy, float scale)
{
    RwIm2DVertex v[32];
    int i;

    for (i = 0; i < 16; i++) {
        float a = (float)i * 0.39269908f;
        shape_vertex(&v[i * 2 + 0], cx, cy, scale, 0.0f, 0.0f, (RwRGBA){255, 86, 86, 255});
        shape_vertex(&v[i * 2 + 1], cx, cy, scale, cosf(a), sinf(a), (RwRGBA){255, 255, 255, 255});
    }
    rw_im2d_render_primitive(RW_PRIM_LINELIST, v, 32);
}

static void
draw_polyline(float cx, float cy, float scale)
{
    RwIm2DVertex v[17];
    int i;

    for (i = 0; i < 17; i++) {
        float a = (float)i * 0.39269908f;
        float r = (i & 1) ? 0.52f : 1.0f;
        shape_vertex(&v[i], cx, cy, scale, cosf(a) * r, sinf(a) * r,
                     (RwRGBA){88, 184, 255, 255});
    }
    rw_im2d_render_primitive(RW_PRIM_POLYLINE, v, 17);
}

static void
draw_tri_list(float cx, float cy, float scale)
{
    RwIm2DVertex v[18];
    RwRGBA colors[6] = {
        {82, 125, 255, 255}, {255, 91, 99, 255},  {91, 220, 119, 255},
        {255, 220, 75, 255}, {225, 92, 230, 255}, {81, 225, 230, 255},
    };
    float p[6][3][2] = {
        {{0.0f, 1.0f}, {-0.5f, 0.35f}, {0.5f, 0.35f}},
        {{-0.5f, 0.35f}, {-0.2f, -0.15f}, {0.5f, 0.35f}},
        {{0.5f, 0.35f}, {-0.2f, -0.15f}, {1.0f, 0.0f}},
        {{-0.2f, -0.15f}, {0.45f, -0.75f}, {-0.7f, -0.75f}},
        {{-0.7f, -0.75f}, {-0.5f, 0.35f}, {-1.0f, 0.0f}},
        {{-0.5f, 0.35f}, {-0.7f, -0.75f}, {-0.2f, -0.15f}},
    };
    int i, j;

    for (i = 0; i < 6; i++)
        for (j = 0; j < 3; j++)
            shape_vertex(&v[i * 3 + j], cx, cy, scale, p[i][j][0], p[i][j][1], colors[i]);
    rw_im2d_render_primitive(RW_PRIM_TRILIST, v, 18);
}

static void
draw_tri_strip(float cx, float cy, float scale)
{
    RwIm2DVertex v[18];
    int i;

    for (i = 0; i < 9; i++) {
        float a = 1.5707963f - (float)i * 0.78539816f;
        shape_vertex(&v[i * 2 + 0], cx, cy, scale, cosf(a), sinf(a),
                     (RwRGBA){215, 96, 255, 255});
        shape_vertex(&v[i * 2 + 1], cx, cy, scale, cosf(a) * 0.48f, sinf(a) * 0.48f,
                     (RwRGBA){255, 255, 255, 255});
    }
    rw_im2d_render_primitive(RW_PRIM_TRISTRIP, v, 18);
}

static void
draw_tri_fan(float cx, float cy, float scale)
{
    RwIm2DVertex v[18];
    int i;

    v[0] = v2(cx, cy, (RwRGBA){255, 255, 255, 255});
    for (i = 0; i < 17; i++) {
        float a = 1.5707963f + (float)i * 0.39269908f;
        shape_vertex(&v[i + 1], cx, cy, scale, cosf(a), sinf(a),
                     (RwRGBA){255, 224, 77, 255});
    }
    rw_im2d_render_primitive(RW_PRIM_TRIFAN, v, 18);
}

static void
draw_indexed_grid(float cx, float cy, float scale)
{
    RwIm2DVertex v[21];
    uint16_t idx[45];
    int row, col, out = 0, tri = 0;

    for (row = 0; row < 5; row++) {
        for (col = 0; col <= row; col++) {
            float x = ((float)col - (float)row * 0.5f) * 0.42f;
            float y = 0.95f - (float)row * 0.42f;
            shape_vertex(&v[out++], cx, cy, scale, x, y, (RwRGBA){89, 242, 180, 255});
        }
    }
    for (row = 0; row < 4; row++) {
        int base0 = row * (row + 1) / 2;
        int base1 = (row + 1) * (row + 2) / 2;
        for (col = 0; col <= row; col++) {
            idx[tri++] = (uint16_t)(base0 + col);
            idx[tri++] = (uint16_t)(base1 + col);
            idx[tri++] = (uint16_t)(base1 + col + 1);
        }
    }
    rw_im2d_render_indexed(RW_PRIM_TRILIST, v, 15, idx, tri);
}

int
main(void)
{
    ExApp app;

    if (!ex_app_init(&app, "rw example: ported im2d primitives", 960, 640))
        return 1;
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);
    rw_set_render_state_ptr(RW_STATE_TEXTURERASTER, NULL);

    printf("im2d_primitives: ESC closes the window\n");
    while (ex_app_running(&app)) {
        float sx, sy;

        ex_app_begin_frame(&app);
        glClearColor(0.04f, 0.04f, 0.04f, 1.0f);
        rw_camera_clear(app.camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
        sx = (float)app.width / 4.0f;
        sy = (float)app.height / 3.0f;
        draw_line_list(sx, sy, 70.0f);
        draw_polyline(sx * 2.0f, sy, 70.0f);
        draw_tri_list(sx * 3.0f, sy, 70.0f);
        draw_tri_strip(sx, sy * 2.0f, 70.0f);
        draw_tri_fan(sx * 2.0f, sy * 2.0f, 70.0f);
        draw_indexed_grid(sx * 3.0f, sy * 2.0f, 82.0f);
        ex_app_swap(&app);
    }

    ex_app_shutdown(&app);
    return 0;
}
