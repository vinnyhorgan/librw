#include "example_common.h"
#include <glad/gles2.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>

int
main(void)
{
    ExApp app;
    RwWorld *world;
    RwClump *room;
    RwLight *ambient, *point;
    RwFrame *point_frame;
    RwFrame *frames[5];
    RwRGBA checker_a = {215, 215, 215, 255};
    RwRGBA checker_b = {55, 65, 82, 255};
    int i;

    if (!ex_app_init(&app, "rw example: ported lights room", 960, 640))
        return 1;

    world = rw_world_create();
    room = rw_clump_create();
    assert(world && room);

    rw_clump_add_atomic(room, ex_make_quad(-2.1f, -1.2f, 2.1f, -0.8f, 4.0f, (RwRGBA){255, 255, 255, 255}, ex_make_checker_raster(64, 64, checker_a, checker_b), &frames[0]));
    rw_clump_add_atomic(room, ex_make_quad(-2.1f, 1.0f, 2.1f, 1.4f, 4.0f, (RwRGBA){255, 255, 255, 255}, ex_make_checker_raster(64, 64, checker_a, checker_b), &frames[1]));
    rw_clump_add_atomic(room, ex_make_quad(-2.3f, -1.0f, -1.9f, 1.2f, 4.0f, (RwRGBA){255, 255, 255, 255}, ex_make_checker_raster(64, 64, checker_a, checker_b), &frames[2]));
    rw_clump_add_atomic(room, ex_make_quad(1.9f, -1.0f, 2.3f, 1.2f, 4.0f, (RwRGBA){255, 255, 255, 255}, ex_make_checker_raster(64, 64, checker_a, checker_b), &frames[3]));
    rw_clump_add_atomic(room, ex_make_quad(-2.1f, -1.0f, 2.1f, 1.2f, 4.35f, (RwRGBA){210, 225, 255, 255}, ex_make_checker_raster(64, 64, checker_a, checker_b), &frames[4]));
    rw_world_add_clump(world, room);

    ambient = ex_make_light(RW_LIGHT_AMBIENT, (RwRGBAf){0.12f, 0.12f, 0.14f, 1.0f}, (RwV3d){0, 0, 0}, 0.0f, NULL);
    point = ex_make_light(RW_LIGHT_POINT, (RwRGBAf){1.0f, 0.62f, 0.28f, 1.0f}, (RwV3d){0, 0, 2.0f}, 5.0f, &point_frame);
    rw_world_add_light(world, ambient);
    rw_world_add_light(world, point);
    rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);
    rw_set_render_state(RW_STATE_FOGENABLE, 0);

    printf("lights_room: ESC closes the window\n");
    while (ex_app_running(&app)) {
        float t = (float)glfwGetTime();
        rw_frame_translate(point_frame, (RwV3d){sinf(t * 1.5f) * 1.4f, cosf(t * 1.1f) * 0.75f, 2.0f}, RW_COMBINE_REPLACE);

        ex_app_begin_frame(&app);
        glClearColor(0.025f, 0.025f, 0.03f, 1.0f);
        rw_camera_clear(app.camera, RW_CAMERA_CLEAR_IMAGE | RW_CAMERA_CLEAR_Z);
        rw_world_render(world);
        ex_app_swap(&app);
    }

    rw_clump_destroy(room);
    for (i = 0; i < 5; i++)
        rw_frame_destroy(frames[i]);
    rw_light_destroy(ambient);
    rw_light_destroy(point);
    rw_frame_destroy(point_frame);
    rw_world_destroy(world);
    ex_app_shutdown(&app);
    return 0;
}
