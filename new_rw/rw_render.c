#include "rw.h"
#include "rw_gl_internal.h"
#include <glad/gles2.h>

#include <string.h>

/* ---- im2d ---- */

void
rw_im2d_render_primitive(RwPrimitiveType type, RwIm2DVertex *verts, int num_verts)
{
    unsigned int vbo, prog;
    RwShaderUniforms *u;
    RwCamera *cam;
    float xform[16];
    GLenum gl_type;

    if (!verts || num_verts < 2) return;

    switch (type) {
    case RW_PRIM_TRILIST:  gl_type = GL_TRIANGLES;  break;
    case RW_PRIM_TRISTRIP: gl_type = GL_TRIANGLE_STRIP; break;
    case RW_PRIM_TRIFAN:   gl_type = GL_TRIANGLE_FAN; break;
    case RW_PRIM_LINELIST: gl_type = GL_LINES;       break;
    case RW_PRIM_POLYLINE: gl_type = GL_LINE_STRIP;  break;
    case RW_PRIM_POINTLIST: gl_type = GL_POINTS;     break;
    default: return;
    }

    prog = rw_gl_get_program(SHADER_IM2D);
    if (!prog) return;
    u = rw_gl_get_uniforms(SHADER_IM2D);

    cam = rw_engine.current_camera;
    if (cam) {
        int w = cam->frame_buffer ? cam->frame_buffer->width : 0;
        int h = cam->frame_buffer ? cam->frame_buffer->height : 0;
        float sx = 2.0f / (float)w;
        float sy = -2.0f / (float)h;
        memset(xform, 0, sizeof(xform));
        xform[0]  = sx;
        xform[5]  = sy;
        xform[10] = 1.0f;
        xform[12] = -1.0f;
        xform[13] = 1.0f;
        xform[15] = 1.0f;
    } else {
        memset(xform, 0, sizeof(xform));
        xform[0] = xform[5] = xform[10] = xform[15] = 1.0f;
    }

    rw_gl_state_use_program(prog);
    rw_gl_set_uniform_vec4(u->locations[U_XFORM], xform);

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
    RwCamera *cam;
    float xform[16];
    GLenum gl_type;

    if (!verts || num_verts < 2 || !indices || num_indices < 3) return;

    switch (type) {
    case RW_PRIM_TRILIST:  gl_type = GL_TRIANGLES;  break;
    case RW_PRIM_TRISTRIP: gl_type = GL_TRIANGLE_STRIP; break;
    case RW_PRIM_TRIFAN:   gl_type = GL_TRIANGLE_FAN; break;
    case RW_PRIM_LINELIST: gl_type = GL_LINES;       break;
    case RW_PRIM_POLYLINE: gl_type = GL_LINE_STRIP;  break;
    default: return;
    }

    prog = rw_gl_get_program(SHADER_IM2D);
    if (!prog) return;
    u = rw_gl_get_uniforms(SHADER_IM2D);

    cam = rw_engine.current_camera;
    if (cam) {
        int w = cam->frame_buffer ? cam->frame_buffer->width : 0;
        int h = cam->frame_buffer ? cam->frame_buffer->height : 0;
        float sx = 2.0f / (float)w;
        float sy = -2.0f / (float)h;
        memset(xform, 0, sizeof(xform));
        xform[0]  = sx;
        xform[5]  = sy;
        xform[10] = 1.0f;
        xform[12] = -1.0f;
        xform[13] = 1.0f;
        xform[15] = 1.0f;
    } else {
        memset(xform, 0, sizeof(xform));
        xform[0] = xform[5] = xform[10] = xform[15] = 1.0f;
    }

    rw_gl_state_use_program(prog);
    rw_gl_set_uniform_vec4(u->locations[U_XFORM], xform);

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
    int stride, has_normal;

    if (!im3d_verts || im3d_num_verts < 2) return;

    switch (type) {
    case RW_PRIM_TRILIST:  gl_type = GL_TRIANGLES;  break;
    case RW_PRIM_TRISTRIP: gl_type = GL_TRIANGLE_STRIP; break;
    case RW_PRIM_TRIFAN:   gl_type = GL_TRIANGLE_FAN; break;
    case RW_PRIM_LINELIST: gl_type = GL_LINES;       break;
    case RW_PRIM_POLYLINE: gl_type = GL_LINE_STRIP;  break;
    case RW_PRIM_POINTLIST: gl_type = GL_POINTS;     break;
    default: return;
    }

    has_normal = (im3d_flags & RW_IM3D_LIGHTING) && (im3d_flags & RW_IM3D_VERTEXXYZ);
    prog = rw_gl_get_program(has_normal ? SHADER_IM3D_LIT : SHADER_IM3D);
    if (!prog) return;
    u = rw_gl_get_uniforms(has_normal ? SHADER_IM3D_LIT : SHADER_IM3D);

    cam = rw_engine.current_camera;
    if (cam) {
        RwMatrix *ltm = rw_frame_get_ltm(cam->frame);
        if (ltm && has_normal)
            rw_gl_upload_matrix(u->locations[U_WORLD], ltm);
    }

    rw_gl_state_use_program(prog);

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
    int stride, has_normal;

    if (!im3d_verts || im3d_num_verts < 2 || !indices || num_indices < 3) return;

    switch (type) {
    case RW_PRIM_TRILIST:  gl_type = GL_TRIANGLES;  break;
    case RW_PRIM_TRISTRIP: gl_type = GL_TRIANGLE_STRIP; break;
    case RW_PRIM_TRIFAN:   gl_type = GL_TRIANGLE_FAN; break;
    case RW_PRIM_LINELIST: gl_type = GL_LINES;       break;
    case RW_PRIM_POLYLINE: gl_type = GL_LINE_STRIP;  break;
    default: return;
    }

    has_normal = (im3d_flags & RW_IM3D_LIGHTING) && (im3d_flags & RW_IM3D_VERTEXXYZ);
    prog = rw_gl_get_program(has_normal ? SHADER_IM3D_LIT : SHADER_IM3D);
    if (!prog) return;
    u = rw_gl_get_uniforms(has_normal ? SHADER_IM3D_LIT : SHADER_IM3D);

    cam = rw_engine.current_camera;
    if (cam) {
        RwMatrix *ltm = rw_frame_get_ltm(cam->frame);
        if (ltm && has_normal)
            rw_gl_upload_matrix(u->locations[U_WORLD], ltm);
    }

    rw_gl_state_use_program(prog);

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
