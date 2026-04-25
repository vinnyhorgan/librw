#include "example_common.h"
#include <glad/gles2.h>

#include <math.h>
#include <stdio.h>

static void
draw_rect(float x, float y, float w, float h, RwRGBA c)
{
    RwIm2DVertex v[4];
    uint16_t idx[6] = {0, 1, 2, 0, 2, 3};

    v[0] = (RwIm2DVertex){x, y, 0.0f, 1.0f, c.r, c.g, c.b, c.a, 0.0f, 0.0f};
    v[1] = (RwIm2DVertex){x + w, y, 0.0f, 1.0f, c.r, c.g, c.b, c.a, 1.0f, 0.0f};
    v[2] = (RwIm2DVertex){x + w, y + h, 0.0f, 1.0f, c.r, c.g, c.b, c.a, 1.0f, 1.0f};
    v[3] = (RwIm2DVertex){x, y + h, 0.0f, 1.0f, c.r, c.g, c.b, c.a, 0.0f, 1.0f};

    rw_set_render_state_ptr(RW_STATE_TEXTURERASTER, NULL);
    rw_im2d_render_indexed(RW_PRIM_TRILIST, v, 4, idx, 6);
}

int
main(void)
{
    ExApp app;
    RwIm3DVertex star[11];
    uint16_t idx[30];
    int i;

    if (!ex_app_init(&app, "rw example: im3d triangle fan", 900, 600))
        return 1;

    for (i = 0; i < 10; i++) {
        idx[i * 3 + 0] = 0;
        idx[i * 3 + 1] = (uint16_t)(i + 1);
        idx[i * 3 + 2] = (uint16_t)(i == 9 ? 1 : i + 2);
    }

    printf("immediate_playground: ESC closes the window\n");
    while (ex_app_running(&app)) {
        float t = (float)glfwGetTime();

        ex_app_begin_frame(&app);
        glClearColor(0.0f, 0.0f, 0.015f, 1.0f);
        rw_camera_clear(app.camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
        draw_rect(0.0f, 0.0f, (float)app.width, (float)app.height, (RwRGBA){1, 2, 10, 255});

        star[0] = (RwIm3DVertex){{0.0f, 0.0f, 1.35f}, {0.0f, 0.0f, -1.0f}, 255, 255, 255, 255, 0.0f, 0.0f};
        for (i = 0; i < 10; i++) {
            float a = t * 0.55f + (float)i * 0.62831853f;
            float r = (i & 1) ? 0.26f : 0.78f;
            RwRGBA colors[5] = {
                {0, 255, 242, 255},
                {112, 255, 166, 255},
                {255, 245, 108, 255},
                {255, 105, 180, 255},
                {166, 143, 255, 255},
            };
            RwRGBA c = colors[i / 2];
            star[i + 1] = (RwIm3DVertex){{cosf(a) * r, sinf(a) * r, 1.35f},
                                         {0.0f, 0.0f, -1.0f},
                                         c.r, c.g, c.b, c.a, 0.0f, 0.0f};
        }
        rw_im3d_transform(star, 11, NULL, RW_IM3D_VERTEXXYZ | RW_IM3D_VERTEXRGBA);
        rw_im3d_render_indexed(RW_PRIM_TRILIST, idx, 30);
        rw_im3d_end();

        ex_app_swap(&app);
    }

    ex_app_shutdown(&app);
    return 0;
}
