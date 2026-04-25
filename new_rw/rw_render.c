#include "rw.h"
#include "rw_gl_internal.h"
#include <glad/gles2.h>

#include <string.h>

static int
rw_prim_to_gl(RwPrimitiveType type, GLenum *gl_type, int *min_count)
{
    switch (type) {
    case RW_PRIM_TRILIST:   *gl_type = GL_TRIANGLES;      *min_count = 3; return 1;
    case RW_PRIM_TRISTRIP:  *gl_type = GL_TRIANGLE_STRIP; *min_count = 3; return 1;
    case RW_PRIM_TRIFAN:    *gl_type = GL_TRIANGLE_FAN;   *min_count = 3; return 1;
    case RW_PRIM_LINELIST:  *gl_type = GL_LINES;          *min_count = 2; return 1;
    case RW_PRIM_POLYLINE:  *gl_type = GL_LINE_STRIP;     *min_count = 2; return 1;
    case RW_PRIM_POINTLIST: *gl_type = GL_POINTS;         *min_count = 1; return 1;
    default: return 0;
    }
}

static void
rw_identity_raw(RwRawMatrix *m)
{
    memset(m, 0, sizeof(*m));
    m->right.x = 1.0f;
    m->up.y = 1.0f;
    m->at.z = 1.0f;
    m->posw = 1.0f;
}

static void
rw_im2d_xform(RwRawMatrix *xform)
{
    RwCamera *cam = rw_engine.current_camera;
    int w = cam && cam->frame_buffer ? cam->frame_buffer->width : 0;
    int h = cam && cam->frame_buffer ? cam->frame_buffer->height : 0;

    if (w <= 0 || h <= 0) {
        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
        w = vp[2];
        h = vp[3];
    }

    memset(xform, 0, sizeof(*xform));
    xform->right.x = w > 0 ? 2.0f / (float)w : 1.0f;
    xform->up.y = h > 0 ? -2.0f / (float)h : 1.0f;
    xform->at.z = 1.0f;
    xform->pos.x = w > 0 ? -1.0f : 0.0f;
    xform->pos.y = h > 0 ? 1.0f : 0.0f;
    xform->posw = 1.0f;
}

static void
rw_im3d_lit_state(RwShaderUniforms *u)
{
    RwRawMatrix world;
    RwWorldLights lights;
    float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float surf[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    memset(&lights, 0, sizeof(lights));
    if (rw_engine.current_world)
        rw_world_enumerate_lights(rw_engine.current_world, NULL, &lights);
    rw_gl_set_lights(&lights, SHADER_IM3D_LIT);
    rw_identity_raw(&world);
    rw_gl_set_uniform_mat4(u->locations[U_WORLD], &world);
    rw_gl_set_uniform_vec4(u->locations[U_MAT_COLOR], one);
    rw_gl_set_uniform_vec4(u->locations[U_SURF_PROPS], surf);
}

static void
rw_im2d_bind_texture(void)
{
    RwRaster *r = rw_get_render_state_ptr(RW_STATE_TEXTURERASTER);

    if (r) {
        if ((!r->gl.texid || r->gl.dirty) && r->pixels)
            rw_gl_upload_raster(r);
        if (r->gl.texid) {
            rw_gl_state_bind_texture(0, r->gl.texid);
            rw_gl_set_texture_sampler(r, r->gl.filter, r->gl.address_u, r->gl.address_v);
            return;
        }
    }
    rw_gl_state_bind_texture(0, rw_gl_get_white_texture());
}

/* ---- im2d ---- */

void
rw_im2d_render_primitive(RwPrimitiveType type, RwIm2DVertex *verts, int num_verts)
{
    unsigned int vbo, prog;
    RwShaderUniforms *u;
    RwRawMatrix xform;
    GLenum gl_type;
    int min_count;

    if (!rw_prim_to_gl(type, &gl_type, &min_count) || !verts || num_verts < min_count) return;

    prog = rw_gl_get_program(SHADER_IM2D);
    if (!prog) return;
    u = rw_gl_get_uniforms(SHADER_IM2D);

    rw_im2d_xform(&xform);
    rw_gl_state_use_program(prog);
    rw_gl_set_uniform_mat4(u->locations[U_XFORM], &xform);
    rw_im2d_bind_texture();
    rw_gl_state_flush_render();

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, num_verts * sizeof(RwIm2DVertex), verts, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RwIm2DVertex), (void *)0);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(RwIm2DVertex), (void *)16);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RwIm2DVertex), (void *)20);

    glDrawArrays(gl_type, 0, num_verts);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
}

void
rw_im2d_render_indexed(RwPrimitiveType type, RwIm2DVertex *verts, int num_verts,
                       uint16_t *indices, int num_indices)
{
    unsigned int vbo, ibo, prog;
    RwShaderUniforms *u;
    RwRawMatrix xform;
    GLenum gl_type;
    int min_count;

    if (!rw_prim_to_gl(type, &gl_type, &min_count) || !verts || !indices ||
        num_verts <= 0 || num_indices < min_count) return;

    prog = rw_gl_get_program(SHADER_IM2D);
    if (!prog) return;
    u = rw_gl_get_uniforms(SHADER_IM2D);

    rw_im2d_xform(&xform);
    rw_gl_state_use_program(prog);
    rw_gl_set_uniform_mat4(u->locations[U_XFORM], &xform);
    rw_im2d_bind_texture();
    rw_gl_state_flush_render();

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, num_verts * sizeof(RwIm2DVertex), verts, GL_STREAM_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(uint16_t), indices, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RwIm2DVertex), (void *)0);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(RwIm2DVertex), (void *)16);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RwIm2DVertex), (void *)20);

    glDrawElements(gl_type, num_indices, GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
}

/* ---- im3d ---- */

static RwIm3DVertex *im3d_verts;
static int im3d_num_verts;
static uint32_t im3d_flags;

void
rw_im3d_transform(RwIm3DVertex *verts, int num_verts, RwMatrix *world, uint32_t flags)
{
    int i;

    if (!verts || num_verts <= 0) return;

    im3d_verts = verts;
    im3d_num_verts = num_verts;
    im3d_flags = flags;

    if (world && (flags & RW_IM3D_VERTEXXYZ)) {
        for (i = 0; i < num_verts; i++) {
            RwV3d p = verts[i].pos;
            verts[i].pos.x = p.x*world->right.x + p.y*world->up.x + p.z*world->at.x + world->pos.x;
            verts[i].pos.y = p.x*world->right.y + p.y*world->up.y + p.z*world->at.y + world->pos.y;
            verts[i].pos.z = p.x*world->right.z + p.y*world->up.z + p.z*world->at.z + world->pos.z;
        }
    }
}

void
rw_im3d_render_primitive(RwPrimitiveType type)
{
    unsigned int vbo, prog;
    RwShaderUniforms *u;
    RwCamera *cam;
    GLenum gl_type;
    int stride, has_normal, min_count;

    if (!rw_prim_to_gl(type, &gl_type, &min_count) || !im3d_verts ||
        im3d_num_verts < min_count) return;

    has_normal = (im3d_flags & RW_IM3D_LIGHTING) && (im3d_flags & RW_IM3D_VERTEXXYZ);
    prog = rw_gl_get_program(has_normal ? SHADER_IM3D_LIT : SHADER_IM3D);
    if (!prog) return;
    u = rw_gl_get_uniforms(has_normal ? SHADER_IM3D_LIT : SHADER_IM3D);

    cam = rw_engine.current_camera;
    if (cam)
        rw_gl_set_camera(cam);

    rw_gl_state_use_program(prog);
    if (has_normal)
        rw_im3d_lit_state(u);
    rw_gl_state_flush_render();

    stride = sizeof(RwIm3DVertex);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, im3d_num_verts * stride, im3d_verts, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void *)24);

    if (has_normal) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)12);
    }

    glDrawArrays(gl_type, 0, im3d_num_verts);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(3);
    if (has_normal) glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
}

void
rw_im3d_render_indexed(RwPrimitiveType type, uint16_t *indices, int num_indices)
{
    unsigned int vbo, ibo, prog;
    RwShaderUniforms *u;
    RwCamera *cam;
    GLenum gl_type;
    int stride, has_normal, min_count;

    if (!rw_prim_to_gl(type, &gl_type, &min_count) || !im3d_verts || !indices ||
        im3d_num_verts <= 0 || num_indices < min_count) return;

    has_normal = (im3d_flags & RW_IM3D_LIGHTING) && (im3d_flags & RW_IM3D_VERTEXXYZ);
    prog = rw_gl_get_program(has_normal ? SHADER_IM3D_LIT : SHADER_IM3D);
    if (!prog) return;
    u = rw_gl_get_uniforms(has_normal ? SHADER_IM3D_LIT : SHADER_IM3D);

    cam = rw_engine.current_camera;
    if (cam)
        rw_gl_set_camera(cam);

    rw_gl_state_use_program(prog);
    if (has_normal)
        rw_im3d_lit_state(u);
    rw_gl_state_flush_render();

    stride = sizeof(RwIm3DVertex);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, im3d_num_verts * stride, im3d_verts, GL_STREAM_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(uint16_t), indices, GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void *)24);

    if (has_normal) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)12);
    }

    glDrawElements(gl_type, num_indices, GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(3);
    if (has_normal) glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
}

void
rw_im3d_end(void)
{
    im3d_verts = NULL;
    im3d_num_verts = 0;
    im3d_flags = 0;
}
