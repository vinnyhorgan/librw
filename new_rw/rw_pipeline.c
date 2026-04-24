#include "rw.h"
#include "rw_gl_internal.h"
#include <glad/gles2.h>
#include <string.h>

static void
default_instance(RwAtomic *a)
{
    RwGeometry *geo = a->geometry;
    RwGlMeshData *data;
    int stride, num_verts, num_indices, i;

    if (!geo) return;

    if (geo->gl_data) {
        rw_gl_destroy_instance_data(geo);
    }

    data = rw_malloc(sizeof(RwGlMeshData));
    if (!data) return;
    memset(data, 0, sizeof(RwGlMeshData));

    stride = 12;
    data->pos_offset = 0;

    data->normal_offset = -1;
    if (geo->geo_flags & RW_GEO_NORMALS) {
        data->normal_offset = stride;
        stride += 12;
    }

    data->tex_offset = -1;
    if (geo->geo_flags & RW_GEO_TEXTURED) {
        data->tex_offset = stride;
        stride += 8;
    }

    data->color_offset = -1;
    if (geo->geo_flags & RW_GEO_PRELIT) {
        data->color_offset = stride;
        stride += 4;
    }

    data->weights_offset = -1;
    data->indices_offset = -1;
    data->has_skin = 0;
    if (geo->skin) {
        data->has_skin = 1;
        data->weights_offset = stride;
        stride += 16;
        data->indices_offset = stride;
        stride += 4;
    }

    data->stride = stride;
    num_verts = geo->num_vertices;

    {
        uint8_t *vbuf = rw_malloc((size_t)num_verts * stride);
        if (!vbuf) { rw_free(data); return; }

        for (i = 0; i < num_verts; i++) {
            uint8_t *v = vbuf + i * stride;
            float *fp;

            fp = (float *)(v + data->pos_offset);
            fp[0] = geo->morph_target.vertices[i].x;
            fp[1] = geo->morph_target.vertices[i].y;
            fp[2] = geo->morph_target.vertices[i].z;

            if (data->normal_offset >= 0) {
                fp = (float *)(v + data->normal_offset);
                fp[0] = geo->morph_target.normals[i].x;
                fp[1] = geo->morph_target.normals[i].y;
                fp[2] = geo->morph_target.normals[i].z;
            }

            if (data->tex_offset >= 0) {
                fp = (float *)(v + data->tex_offset);
                fp[0] = geo->texcoords[i].u;
                fp[1] = geo->texcoords[i].v;
            }

            if (data->color_offset >= 0) {
                uint8_t *cp = v + data->color_offset;
                cp[0] = geo->colors[i].r;
                cp[1] = geo->colors[i].g;
                cp[2] = geo->colors[i].b;
                cp[3] = geo->colors[i].a;
            }

            if (data->has_skin) {
                int j;

                fp = (float *)(v + data->weights_offset);
                for (j = 0; j < 4; j++)
                    fp[j] = j < geo->skin->num_weights ? geo->skin->weights[i * geo->skin->num_weights + j] : 0.0f;
                {
                    uint8_t *ip = v + data->indices_offset;
                    for (j = 0; j < 4; j++) {
                        uint8_t idx = j < geo->skin->num_weights ? geo->skin->bone_indices[i * geo->skin->num_weights + j] : 0;
                        ip[j] = idx < geo->skin->num_bones ? idx : 0;
                    }
                }
            }
        }

        {
            glGenBuffers(1, &data->vbo);
            glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
            glBufferData(GL_ARRAY_BUFFER, num_verts * stride, vbuf, GL_STATIC_DRAW);
            rw_free(vbuf);
        }
    }

    num_indices = 0;
    if (geo->mesh_header)
        num_indices = geo->mesh_header->total_indices;

    if (num_indices > 0) {
        uint16_t *ibuf = rw_malloc(num_indices * sizeof(uint16_t));
        if (ibuf) {
            int idx = 0, m;
            for (m = 0; m < geo->mesh_header->num_meshes; m++) {
                RwMesh *mesh = &((RwMesh *)(geo->mesh_header + 1))[m];
                uint32_t j;
                for (j = 0; j < mesh->num_indices; j++)
                    ibuf[idx++] = mesh->indices[j];
            }
            glGenBuffers(1, &data->ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         num_indices * sizeof(uint16_t), ibuf, GL_STATIC_DRAW);
            rw_free(ibuf);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    geo->gl_data = data;
}

static void
setup_vertex_input(RwGlMeshData *data)
{
    glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data->ibo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, data->stride,
                          (void *)(long)data->pos_offset);

    if (data->normal_offset >= 0) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, data->stride,
                              (void *)(long)data->normal_offset);
    }
    if (data->tex_offset >= 0) {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, data->stride,
                              (void *)(long)data->tex_offset);
    }
    if (data->color_offset >= 0) {
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, data->stride,
                              (void *)(long)data->color_offset);
    }
    if (data->has_skin) {
        if (data->weights_offset >= 0) {
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, data->stride,
                                  (void *)(long)data->weights_offset);
        }
        if (data->indices_offset >= 0) {
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, data->stride,
                                  (void *)(long)data->indices_offset);
        }
    }
}

static void
disable_vertex_input(RwGlMeshData *data)
{
    glDisableVertexAttribArray(0);
    if (data->normal_offset >= 0) glDisableVertexAttribArray(1);
    if (data->tex_offset >= 0)    glDisableVertexAttribArray(2);
    if (data->color_offset >= 0)  glDisableVertexAttribArray(3);
    if (data->has_skin) {
        if (data->weights_offset >= 0)  glDisableVertexAttribArray(4);
        if (data->indices_offset >= 0)  glDisableVertexAttribArray(5);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void
default_render(RwAtomic *a)
{
    RwGeometry *geo;
    RwGlMeshData *data;
    RwShaderUniforms *u;
    int shader_idx;
    unsigned int prog;
    int m, index_offset;
    RwWorldLights lights;

    if (!a || !a->geometry || !a->geometry->gl_data) return;
    if (!a->frame) return;

    geo = a->geometry;
    data = (RwGlMeshData *)geo->gl_data;

    if (!geo->mesh_header) return;
    if (rw_engine.current_camera)
        rw_gl_set_camera(rw_engine.current_camera);

    memset(&lights, 0, sizeof(lights));
    if (a->world)
        rw_world_enumerate_lights(a->world, a, &lights);
    shader_idx = rw_gl_select_shader(lights.num_directionals, lights.num_locals, data->has_skin);

    prog = rw_gl_get_program(shader_idx);
    if (!prog) return;
    u = rw_gl_get_uniforms(shader_idx);

    rw_gl_state_use_program(prog);

    {
        RwMatrix *ltm = rw_frame_get_ltm(a->frame);
        rw_gl_upload_matrix(u->locations[U_WORLD], ltm);
    }

    rw_gl_set_lights(&lights, shader_idx);

    if (data->has_skin && a->hanim)
        rw_gl_upload_skin_matrices(a->hanim, geo->skin, shader_idx);

    {
        float fog_data[4] = {0,0,0,0};
        float fog_color[4] = {0,0,0,0};
        if (rw_get_render_state(RW_STATE_FOGENABLE)) {
            RwCamera *cam = rw_engine.current_camera;
            if (cam) {
                float range = cam->far_plane - cam->fog_plane;
                fog_data[0] = 1.0f;
                fog_data[1] = cam->fog_plane;
                fog_data[2] = range > 0.0f ? 1.0f / range : 0.0f;
            }
            {
                uint32_t fc = rw_get_render_state(RW_STATE_FOGCOLOR);
                fog_color[0] = ((fc >> 16) & 0xFF) / 255.0f;
                fog_color[1] = ((fc >>  8) & 0xFF) / 255.0f;
                fog_color[2] = ((fc >>  0) & 0xFF) / 255.0f;
                fog_color[3] = ((fc >> 24) & 0xFF) / 255.0f;
            }
        }
        rw_gl_set_uniform_vec4(u->locations[U_FOG_DATA], fog_data);
        rw_gl_set_uniform_vec4(u->locations[U_FOG_COLOR], fog_color);
    }

    {
        float aref[4] = {0,0,0,0};
        if (rw_get_render_state(RW_STATE_ALPHATEST))
            aref[0] = (float)rw_get_render_state(RW_STATE_ALPHAREF) / 255.0f;
        rw_gl_set_uniform_vec4(u->locations[U_ALPHA_REF], aref);
    }

    rw_gl_state_flush_render();

    setup_vertex_input(data);

    index_offset = 0;
    for (m = 0; m < geo->mesh_header->num_meshes; m++) {
        RwMesh *mesh = &((RwMesh *)(geo->mesh_header + 1))[m];
        RwMaterial *mat = mesh->material;

        if (mat) {
            float mc[4] = {
                mat->color.r / 255.0f, mat->color.g / 255.0f,
                mat->color.b / 255.0f, mat->color.a / 255.0f
            };
            float sp[4] = { mat->surface.ambient, mat->surface.specular, mat->surface.diffuse, 1.0f };
            rw_gl_set_uniform_vec4(u->locations[U_MAT_COLOR], mc);
            rw_gl_set_uniform_vec4(u->locations[U_SURF_PROPS], sp);

            if (mat->texture && mat->texture->raster) {
                RwRaster *r = mat->texture->raster;
                if (!r->gl.texid && r->pixels)
                    rw_gl_upload_raster(r);
                if (r->gl.texid) {
                    rw_gl_state_bind_texture(0, r->gl.texid);
                    rw_gl_set_texture_sampler(r,
                        mat->texture->filter_addressing & 0xFF,
                        (mat->texture->filter_addressing >> 8) & 0xFF,
                        (mat->texture->filter_addressing >> 12) & 0xFF);
                }
            } else {
                rw_gl_state_bind_texture(0, rw_gl_get_white_texture());
            }
        } else {
            rw_gl_state_bind_texture(0, rw_gl_get_white_texture());
        }

        glDrawElements((geo->mesh_header->flags & RW_GEO_TRISTRIP) ? GL_TRIANGLE_STRIP : GL_TRIANGLES,
                       mesh->num_indices,
                       GL_UNSIGNED_SHORT, (void *)(long)index_offset);
        index_offset += mesh->num_indices * sizeof(uint16_t);
    }

    disable_vertex_input(data);
}

static RwObjPipeline default_pipeline = {
    default_instance,
    NULL,
    default_render
};

static RwObjPipeline skin_pipeline = {
    default_instance,
    NULL,
    default_render
};

RwObjPipeline *
rw_gl_default_pipeline(void)
{
    return &default_pipeline;
}

RwObjPipeline *
rw_gl_skin_pipeline(void)
{
    return &skin_pipeline;
}

void
rw_gl_destroy_instance_data(RwGeometry *geo)
{
    RwGlMeshData *data;
    if (!geo || !geo->gl_data) return;
    data = (RwGlMeshData *)geo->gl_data;
    if (data->vbo) glDeleteBuffers(1, &data->vbo);
    if (data->ibo) glDeleteBuffers(1, &data->ibo);
    rw_free(data);
    geo->gl_data = NULL;
}
