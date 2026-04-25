#include "example_common.h"
#include <glad/gles2.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int
ex_app_init(ExApp *app, const char *title, int width, int height)
{
    const char *frames;

    memset(app, 0, sizeof(*app));
    app->width = width;
    app->height = height;
    frames = getenv("RW_EXAMPLE_FRAMES");
    app->max_frames = frames ? atoi(frames) : 0;

    if (!glfwInit())
        return 0;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    if (app->max_frames > 0)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    app->window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!app->window)
        return 0;
    glfwMakeContextCurrent(app->window);
    glfwSwapInterval(1);
    gladLoadGLES2(glfwGetProcAddress);

    if (!rw_engine_init(NULL) || !rw_engine_open() || !rw_engine_start())
        return 0;

    app->camera = rw_camera_create();
    app->camera_frame = rw_frame_create();
    if (!app->camera || !app->camera_frame)
        return 0;
    rw_camera_set_frame(app->camera, app->camera_frame);
    rw_camera_set_near_far(app->camera, 0.1f, 80.0f);
    rw_camera_set_view_window(app->camera, 0.75f);
    app->camera->fog_plane = 18.0f;
    return 1;
}

void
ex_app_begin_frame(ExApp *app)
{
    int w, h;

    glfwGetFramebufferSize(app->window, &w, &h);
    if (w <= 0 || h <= 0)
        return;
    app->width = w;
    app->height = h;
    glViewport(0, 0, w, h);
    rw_camera_begin_update(app->camera);
}

int
ex_app_running(ExApp *app)
{
    if (glfwWindowShouldClose(app->window))
        return 0;
    if (glfwGetKey(app->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        return 0;
    if (app->max_frames > 0 && app->frame >= app->max_frames)
        return 0;
    return 1;
}

void
ex_app_swap(ExApp *app)
{
    glfwSwapBuffers(app->window);
    glfwPollEvents();
    app->frame++;
}

void
ex_app_shutdown(ExApp *app)
{
    rw_camera_destroy(app->camera);
    rw_frame_destroy(app->camera_frame);
    rw_engine_stop();
    rw_engine_close();
    rw_engine_term();
    if (app->window)
        glfwDestroyWindow(app->window);
    glfwTerminate();
}

RwRaster *
ex_make_checker_raster(int w, int h, RwRGBA a, RwRGBA b)
{
    RwRaster *r = rw_raster_create(w, h, 32, 0);
    uint8_t *p;
    int x, y;

    assert(r);
    p = rw_raster_lock(r);
    assert(p);
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            RwRGBA c = (((x / 8) + (y / 8)) & 1) ? a : b;
            uint8_t *px = p + (y * w + x) * 4;
            px[0] = c.r;
            px[1] = c.g;
            px[2] = c.b;
            px[3] = c.a;
        }
    }
    rw_raster_unlock(r);
    return r;
}

RwAtomic *
ex_make_quad(float x0, float y0, float x1, float y1, float z,
             RwRGBA color, RwRaster *raster, RwFrame **frame_out)
{
    RwGeometry *geo;
    RwAtomic *atomic;
    RwFrame *frame;
    int i;

    geo = rw_geometry_create(4, 2, RW_GEO_POSITIONS | RW_GEO_PRELIT | RW_GEO_NORMALS | RW_GEO_TEXTURED);
    assert(geo);
    rw_geometry_lock(geo, RW_LOCK_ALL);
    geo->morph_target.vertices[0] = (RwV3d){x0, y0, z};
    geo->morph_target.vertices[1] = (RwV3d){x1, y0, z};
    geo->morph_target.vertices[2] = (RwV3d){x1, y1, z};
    geo->morph_target.vertices[3] = (RwV3d){x0, y1, z};
    geo->texcoords[0] = (RwTexCoords){0.0f, 0.0f};
    geo->texcoords[1] = (RwTexCoords){1.0f, 0.0f};
    geo->texcoords[2] = (RwTexCoords){1.0f, 1.0f};
    geo->texcoords[3] = (RwTexCoords){0.0f, 1.0f};
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
    if (raster) {
        RwTexture *tex = rw_texture_create(raster);
        assert(tex);
        rw_texture_set_filter(tex, RW_TEX_FILTER_LINEAR);
        rw_texture_set_addressing(tex, RW_TEX_WRAP_CLAMP, RW_TEX_WRAP_CLAMP);
        rw_material_set_texture(geo->materials[0], tex);
        rw_texture_destroy(tex);
    }
    rw_material_set_color(geo->materials[0], (RwRGBA){255, 255, 255, 255});
    rw_geometry_build_meshes(geo);
    rw_geometry_calc_bounding_sphere(geo);

    atomic = rw_atomic_create();
    frame = rw_frame_create();
    assert(atomic && frame);
    rw_atomic_set_frame(atomic, frame);
    rw_atomic_set_geometry(atomic, geo);
    rw_geometry_destroy(geo);
    rw_atomic_set_pipeline(atomic, rw_gl_default_pipeline());
    if (frame_out)
        *frame_out = frame;
    return atomic;
}

RwLight *
ex_make_light(RwLightType type, RwRGBAf color, RwV3d pos, float radius, RwFrame **frame_out)
{
    RwLight *light = rw_light_create(type);
    RwFrame *frame = NULL;

    assert(light);
    rw_light_set_color(light, color);
    if (type == RW_LIGHT_POINT || type == RW_LIGHT_SPOT || type == RW_LIGHT_DIRECTIONAL) {
        frame = rw_frame_create();
        assert(frame);
        rw_frame_translate(frame, pos, RW_COMBINE_REPLACE);
        rw_light_set_frame(light, frame);
    }
    if (radius > 0.0f)
        rw_light_set_radius(light, radius);
    if (frame_out)
        *frame_out = frame;
    return light;
}
