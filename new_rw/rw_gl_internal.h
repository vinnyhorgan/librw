#ifndef RW_GL_INTERNAL_H
#define RW_GL_INTERNAL_H

#include "rw.h"

typedef struct {
    int blend;
    int blend_src, blend_dst;
    int depth_test;
    int depth_write;
    int cull;
    int cull_mode;
    int alpha_test;
    float alpha_ref;
    unsigned int bound_texture[4];
    unsigned int current_program;
} RwGlState;

typedef struct {
    unsigned int vbo;
    unsigned int ibo;
    int stride;
    int pos_offset;
    int normal_offset;
    int tex_offset;
    int color_offset;
    int weights_offset;
    int indices_offset;
    int has_skin;
} RwGlMeshData;

enum {
    U_PROJ, U_VIEW, U_WORLD, U_AMB_LIGHT,
    U_LIGHT_PARAMS, U_LIGHT_POSITION, U_LIGHT_DIRECTION, U_LIGHT_COLOR,
    U_MAT_COLOR, U_SURF_PROPS, U_ALPHA_REF,
    U_FOG_DATA, U_FOG_COLOR, U_BONE_MATRICES, U_XFORM,
    U_NUM_UNIFORMS
};

enum {
    SHADER_DEFAULT,
    SHADER_DEFAULT_DIR,
    SHADER_DEFAULT_DIR_POINT,
    SHADER_SKIN,
    SHADER_SKIN_DIR_POINT,
    SHADER_IM2D,
    SHADER_IM3D,
    SHADER_IM3D_LIT,
    SHADER_COUNT
};

typedef struct {
    unsigned int program;
    int locations[U_NUM_UNIFORMS];
} RwShaderUniforms;

/* rw_gl.c */
int   rw_gl_device_init(void);
int   rw_gl_device_shutdown(void);
int   rw_gl_open(void);
int   rw_gl_start(void);
void  rw_gl_stop(void);
void  rw_gl_close(void);

void  rw_gl_set_camera(RwCamera *cam);
void  rw_gl_set_lights(RwWorldLights *lights, int shader_idx);
void  rw_gl_upload_skin_matrices(RwHAnimHier *hier, RwSkin *skin, int shader_idx);
int   rw_gl_select_shader(int num_dir, int num_point, int is_skin);
unsigned int rw_gl_get_program(int shader_idx);
RwShaderUniforms *rw_gl_get_uniforms(int shader_idx);

void  rw_gl_upload_raster(RwRaster *r);
void  rw_gl_set_texture_sampler(RwRaster *r, int filter, int address_u, int address_v);

void  rw_gl_state_flush_render(void);
void  rw_gl_state_bind_texture(int unit, unsigned int texid);
void  rw_gl_state_use_program(unsigned int prog);

void  rw_gl_upload_matrix(int loc, const RwMatrix *m);
void  rw_gl_upload_raw_matrix(int loc, const RwRawMatrix *raw);
void  rw_gl_set_uniform_vec4(int loc, const float *v);
void  rw_gl_set_uniform_mat4(int loc, const RwRawMatrix *m);

/* rw_pipeline.c */
RwObjPipeline *rw_gl_default_pipeline(void);
RwObjPipeline *rw_gl_skin_pipeline(void);
void  rw_gl_destroy_instance_data(RwGeometry *geo);

#endif
