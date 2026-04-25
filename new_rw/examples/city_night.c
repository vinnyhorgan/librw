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
    rw_set_render_state(RW_STATE_VERTEXALPHA, c.a < 255);
    rw_set_render_state(RW_STATE_SRCBLEND, RW_BLEND_SRCALPHA);
    rw_set_render_state(RW_STATE_DESTBLEND, RW_BLEND_INVSRCALPHA);
    rw_im2d_render_indexed(RW_PRIM_TRILIST, v, 4, idx, 6);
    rw_set_render_state(RW_STATE_VERTEXALPHA, 0);
}

static void
draw_building(float x, float y, float w, float h, RwRGBA body, RwRGBA lit)
{
    int col, row;
    float wx = w / 6.0f;
    float wy = h / 9.0f;

    draw_rect(x, y, w, h, body);
    draw_rect(x, y, w, 5.0f, (RwRGBA){lit.r, lit.g, lit.b, 255});
    for (row = 0; row < 6; row++) {
        for (col = 0; col < 4; col++) {
            if (((row + col) & 1) == 0)
                draw_rect(x + wx + (float)col * wx * 1.2f,
                          y + wy + (float)row * wy * 1.15f,
                          wx * 0.55f, wy * 0.42f, lit);
        }
    }
}

int
main(void)
{
    ExApp app;

    if (!ex_app_init(&app, "rw example: im2d neon skyline", 960, 540))
        return 1;
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);

    printf("city_night: ESC closes the window\n");
    while (ex_app_running(&app)) {
        float t = (float)glfwGetTime();
        float train = fmodf(t * 170.0f, (float)app.width + 260.0f) - 260.0f;
        float ground = (float)app.height - 104.0f;

        ex_app_begin_frame(&app);
        glClearColor(0.015f, 0.018f, 0.035f, 1.0f);
        rw_camera_clear(app.camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);

        draw_rect(0.0f, 0.0f, (float)app.width, (float)app.height, (RwRGBA){5, 7, 20, 255});
        draw_rect(0.0f, ground, (float)app.width, 104.0f, (RwRGBA){12, 18, 32, 255});
        draw_rect(0.0f, ground, (float)app.width, 4.0f, (RwRGBA){66, 232, 255, 255});

        draw_building(70.0f, ground - 210.0f, 104.0f, 210.0f,
                      (RwRGBA){20, 34, 58, 255}, (RwRGBA){37, 245, 147, 255});
        draw_building(210.0f, ground - 282.0f, 142.0f, 282.0f,
                      (RwRGBA){22, 31, 52, 255}, (RwRGBA){255, 213, 73, 255});
        draw_building(405.0f, ground - 342.0f, 122.0f, 342.0f,
                      (RwRGBA){18, 42, 70, 255}, (RwRGBA){88, 210, 255, 255});
        draw_building(606.0f, ground - 258.0f, 158.0f, 258.0f,
                      (RwRGBA){24, 30, 58, 255}, (RwRGBA){255, 70, 194, 255});
        draw_building(812.0f, ground - 312.0f, 96.0f, 312.0f,
                      (RwRGBA){18, 36, 54, 255}, (RwRGBA){137, 255, 91, 255});

        draw_rect(train, ground + 24.0f, 230.0f, 34.0f, (RwRGBA){236, 40, 132, 255});
        draw_rect(train + 14.0f, ground + 32.0f, 54.0f, 10.0f, (RwRGBA){246, 255, 150, 255});
        draw_rect(train + 88.0f, ground + 32.0f, 54.0f, 10.0f, (RwRGBA){246, 255, 150, 255});
        draw_rect(train + 162.0f, ground + 32.0f, 54.0f, 10.0f, (RwRGBA){246, 255, 150, 255});

        ex_app_swap(&app);
    }

    ex_app_shutdown(&app);
    return 0;
}
