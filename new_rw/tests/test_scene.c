#include "../rw.h"

#include <assert.h>
#include <math.h>

static int
near(float a, float b)
{
    return fabsf(a - b) < 0.0001f;
}

static int render_count;
static int callback_count;

static void
render_atomic(RwAtomic *atomic)
{
    assert(atomic);
    render_count++;
}

static void
callback_atomic(RwAtomic *atomic)
{
    assert(atomic);
    callback_count++;
}

static void
test_atomic_clump_world_render(void)
{
    RwObjPipeline pipe = {NULL, NULL, render_atomic};
    RwGeometry *geo = rw_geometry_create(1, 0, RW_GEO_POSITIONS);
    RwAtomic *atomic = rw_atomic_create();
    RwClump *clump = rw_clump_create();
    RwWorld *world = rw_world_create();

    assert(geo);
    assert(atomic);
    assert(clump);
    assert(world);

    rw_atomic_set_geometry(atomic, geo);
    assert(geo->ref_count == 2);
    rw_geometry_destroy(geo);
    assert(atomic->geometry->ref_count == 1);

    rw_atomic_set_pipeline(atomic, &pipe);
    rw_atomic_set_render_cb(atomic, callback_atomic);
    rw_clump_add_atomic(clump, atomic);
    assert(atomic->clump == clump);

    rw_world_add_clump(world, clump);
    assert(clump->world == world);
    assert(atomic->world == world);

    render_count = 0;
    callback_count = 0;
    rw_world_render(world);
    assert(render_count == 1);
    assert(callback_count == 1);

    rw_clump_remove_atomic(clump, atomic);
    assert(atomic->clump == NULL && atomic->world == NULL);
    rw_atomic_destroy(atomic);
    rw_clump_destroy(clump);
    rw_world_destroy(world);
}

static void
test_world_lights(void)
{
    RwWorld *world = rw_world_create();
    RwAtomic *atomic = rw_atomic_create();
    RwFrame *aframe = rw_frame_create();
    RwFrame *lframe = rw_frame_create();
    RwLight *ambient = rw_light_create(RW_LIGHT_AMBIENT);
    RwLight *dir = rw_light_create(RW_LIGHT_DIRECTIONAL);
    RwLight *point = rw_light_create(RW_LIGHT_POINT);
    RwWorldLights lights;

    assert(world && atomic && aframe && lframe && ambient && dir && point);
    rw_atomic_set_frame(atomic, aframe);
    atomic->bounding_sphere.center = (RwV3d){0, 0, 0};
    atomic->bounding_sphere.radius = 1.0f;
    rw_frame_translate(lframe, (RwV3d){0, 0, 2}, RW_COMBINE_REPLACE);
    rw_light_set_frame(point, lframe);
    rw_light_set_radius(point, 2.0f);
    rw_light_set_color(ambient, (RwRGBAf){0.25f, 0.5f, 0.75f, 1.0f});

    rw_world_add_light(world, ambient);
    rw_world_add_light(world, dir);
    rw_world_add_light(world, point);
    rw_world_enumerate_lights(world, atomic, &lights);

    assert(lights.num_ambients == 1);
    assert(lights.ambient.r == 0.25f && lights.ambient.g == 0.5f && lights.ambient.b == 0.75f);
    assert(lights.num_directionals == 1 && lights.directionals[0] == dir);
    assert(lights.num_locals == 1 && lights.locals[0] == point);

    rw_light_destroy(ambient);
    rw_light_destroy(dir);
    rw_light_destroy(point);
    rw_frame_destroy(lframe);
    rw_frame_destroy(aframe);
    rw_atomic_destroy(atomic);
    rw_world_destroy(world);
}

static void
test_camera_frustum(void)
{
    RwCamera *camera = rw_camera_create();
    RwFrame *frame = rw_frame_create();
    RwSphere visible = {{0, 0, 5}, 1.0f};
    RwSphere outside = {{10, 0, 5}, 1.0f};
    RwSphere behind = {{0, 0, -1}, 0.25f};
    RwSphere moved_visible = {{0, 0, 15}, 1.0f};
    RwSphere moved_behind = {{0, 0, 5}, 1.0f};

    assert(camera);
    assert(frame);
    rw_camera_set_near_far(camera, 1.0f, 20.0f);
    rw_camera_set_view_window(camera, 1.0f);
    rw_camera_begin_update(camera);
    assert(rw_engine.current_camera == camera);
    assert(near(camera->frustum[0].normal.z, 1.0f));
    assert(near(camera->frustum[0].dist, -1.0f));
    assert(near(camera->frustum[1].normal.z, -1.0f));
    assert(near(camera->frustum[1].dist, 20.0f));
    assert(rw_camera_frustum_test_sphere(camera, &visible));
    assert(!rw_camera_frustum_test_sphere(camera, &outside));
    assert(!rw_camera_frustum_test_sphere(camera, &behind));

    rw_frame_translate(frame, (RwV3d){0, 0, 10}, RW_COMBINE_REPLACE);
    rw_camera_set_frame(camera, frame);
    rw_camera_begin_update(camera);
    assert(near(camera->frustum[0].dist, -11.0f));
    assert(rw_camera_frustum_test_sphere(camera, &moved_visible));
    assert(!rw_camera_frustum_test_sphere(camera, &moved_behind));

    rw_frame_destroy(frame);
    rw_camera_destroy(camera);
}

int
main(void)
{
    assert(rw_engine_init(NULL));
    test_atomic_clump_world_render();
    test_world_lights();
    test_camera_frustum();
    rw_engine_term();
    return 0;
}
