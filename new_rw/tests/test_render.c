#include "../rw.h"
#include "../rw_gl_internal.h"
#include <glad/gles2.h>
#include <GLFW/glfw3.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
test_render_triangle(int skinned)
{
    GLFWwindow *win;
    RwGeometry *geo;
    RwAtomic *atomic;
    RwClump *clump;
    RwWorld *world;
    RwCamera *camera;
    RwFrame *cam_frame;
    RwFrame *atomic_frame;
    RwMaterial *mat;
    RwSkin *skin = NULL;
    RwHAnimHier *hier = NULL;
    int w = 256, h = 256;
    uint8_t pixels[4];
    GLenum err;

    win = glfwCreateWindow(w, h, "test_render", NULL, NULL);
    assert(win);
    glfwMakeContextCurrent(win);
    gladLoadGLES2(glfwGetProcAddress);

    assert(rw_engine_init(NULL));
    assert(rw_engine_open());
    assert(rw_engine_start());

    camera = rw_camera_create();
    assert(camera);
    cam_frame = rw_frame_create();
    assert(cam_frame);
    rw_camera_set_frame(camera, cam_frame);
    rw_camera_set_near_far(camera, 0.1f, 100.0f);
    rw_camera_set_view_window(camera, 1.0f);
    rw_camera_begin_update(camera);

    world = rw_world_create();
    assert(world);

    clump = rw_clump_create();
    assert(clump);

    atomic = rw_atomic_create();
    assert(atomic);
    atomic_frame = rw_frame_create();
    assert(atomic_frame);

    geo = rw_geometry_create(3, 1, RW_GEO_POSITIONS | RW_GEO_PRELIT | RW_GEO_NORMALS);
    assert(geo);
    rw_geometry_lock(geo, RW_LOCK_ALL);
    geo->morph_target.vertices[0] = (RwV3d){ 0.0f,  0.2f, 1.0f};
    geo->morph_target.vertices[1] = (RwV3d){-0.2f, -0.2f, 1.0f};
    geo->morph_target.vertices[2] = (RwV3d){ 0.2f, -0.2f, 1.0f};
    geo->morph_target.normals[0] = (RwV3d){0.0f, 0.0f, -1.0f};
    geo->morph_target.normals[1] = (RwV3d){0.0f, 0.0f, -1.0f};
    geo->morph_target.normals[2] = (RwV3d){0.0f, 0.0f, -1.0f};
    geo->colors[0] = (RwRGBA){255, 0, 0, 255};
    geo->colors[1] = (RwRGBA){0, 255, 0, 255};
    geo->colors[2] = (RwRGBA){0, 0, 255, 255};
    geo->triangles[0].v[0] = 0;
    geo->triangles[0].v[1] = 1;
    geo->triangles[0].v[2] = 2;
    geo->triangles[0].mat_id = 0;
    rw_geometry_unlock(geo);
    assert(rw_geometry_build_meshes(geo));

    if (skinned) {
        RwHAnimNodeInfo node = {0, 0, 0, NULL};
        int i;

        skin = rw_skin_create(1, 3, 4);
        assert(skin);
        for (i = 0; i < 3; i++)
            skin->weights[i * 4] = 1.0f;
        for (i = 0; i < 16; i++)
            skin->inverse_matrices[i] = (i % 5) == 0 ? 1.0f : 0.0f;
        geo->skin = skin;

        hier = rw_hanim_create(1, &node);
        assert(hier);
        rw_hanim_attach(hier, atomic_frame);
        rw_hanim_update_matrices(hier);
    }

    mat = rw_material_create();
    assert(mat);
    {
        RwRaster *white_raster = rw_raster_create(1, 1, 32, 0);
        RwTexture *white_tex;
        uint8_t *pixels;
        assert(white_raster);
        pixels = rw_raster_lock(white_raster);
        pixels[0] = 255; pixels[1] = 255; pixels[2] = 255; pixels[3] = 255;
        rw_raster_unlock(white_raster);
        white_tex = rw_texture_create(white_raster);
        assert(white_tex);
        rw_material_set_texture(mat, white_tex);
        rw_texture_destroy(white_tex);
        /* raster is owned by texture; texture destroy will free it */
    }
    rw_material_set_color(mat, (RwRGBA){255, 255, 255, 255});
    geo->materials[0] = mat;

    rw_atomic_set_geometry(atomic, geo);
    rw_geometry_destroy(geo);
    rw_atomic_set_frame(atomic, atomic_frame);

    if (skinned) {
        rw_atomic_set_hanim_hierarchy(atomic, hier);
        rw_skin_set_pipeline(atomic);
    } else {
        rw_atomic_set_pipeline(atomic, rw_gl_default_pipeline());
    }
    rw_clump_add_atomic(clump, atomic);
    rw_world_add_clump(world, clump);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    rw_world_render(world);

    err = glGetError();
    fprintf(stderr, "GL error after render: 0x%x\n", err);
    assert(err == GL_NO_ERROR);

    glReadPixels(w / 2, h / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    fprintf(stderr, "center pixel: R=%d G=%d B=%d A=%d\n", pixels[0], pixels[1], pixels[2], pixels[3]);

    assert(pixels[0] || pixels[1] || pixels[2]);

    rw_clump_destroy(clump);
    rw_frame_destroy(atomic_frame);
    rw_hanim_destroy(hier);
    rw_skin_destroy(skin);
    rw_world_destroy(world);
    rw_camera_destroy(camera);
    rw_frame_destroy(cam_frame);

    rw_engine_stop();
    rw_engine_close();
    rw_engine_term();

    glfwDestroyWindow(win);
    fprintf(stderr, "test_render_triangle(%s): PASS\n", skinned ? "skinned" : "default");
}

int
main(void)
{
    assert(glfwInit());
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    test_render_triangle(0);
    test_render_triangle(1);

    glfwTerminate();
    return 0;
}
