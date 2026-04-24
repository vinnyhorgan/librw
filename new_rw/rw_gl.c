#define GLAD_GLES2_IMPLEMENTATION
#include "rw.h"
#include "rw_gl_internal.h"
#include <glad/gles2.h>
#include <string.h>
#include <stdio.h>

/* ---- State Cache ---- */

static RwGlState gl_state;

void
rw_gl_state_bind_texture(int unit, unsigned int texid)
{
    if (gl_state.bound_texture[unit] != texid) {
        if (unit != 0)
            glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, texid);
        if (unit != 0)
            glActiveTexture(GL_TEXTURE0);
        gl_state.bound_texture[unit] = texid;
    }
}

static void
gl_set_blend(int enable, int src, int dst)
{
    if (gl_state.blend != enable) {
        if (enable) glEnable(GL_BLEND);
        else        glDisable(GL_BLEND);
        gl_state.blend = enable;
    }
    if (enable && (gl_state.blend_src != src || gl_state.blend_dst != dst)) {
        glBlendFunc(src, dst);
        gl_state.blend_src = src;
        gl_state.blend_dst = dst;
    }
}

static void
gl_set_depth_test(int enable)
{
    if (gl_state.depth_test != enable) {
        if (enable) glEnable(GL_DEPTH_TEST);
        else        glDisable(GL_DEPTH_TEST);
        gl_state.depth_test = enable;
    }
}

static void
gl_set_depth_write(int enable)
{
    if (gl_state.depth_write != enable) {
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
        gl_state.depth_write = enable;
    }
}

static void
gl_set_cull(int enable, int mode)
{
    if (gl_state.cull != enable) {
        if (enable) glEnable(GL_CULL_FACE);
        else        glDisable(GL_CULL_FACE);
        gl_state.cull = enable;
    }
    if (enable && gl_state.cull_mode != mode) {
        glCullFace(mode);
        gl_state.cull_mode = mode;
    }
}

void
rw_gl_state_use_program(unsigned int prog)
{
    if (gl_state.current_program != prog) {
        glUseProgram(prog);
        gl_state.current_program = prog;
    }
}

static void
gl_state_reset(void)
{
    memset(&gl_state, 0, sizeof(gl_state));
    gl_state.depth_test = -1;
    gl_state.depth_write = -1;
    gl_state.blend = -1;
    gl_state.blend_src = -1;
    gl_state.blend_dst = -1;
    gl_state.cull = -1;
    gl_state.cull_mode = -1;
    gl_state.alpha_test = -1;
    gl_state.alpha_ref = -1.0f;
    gl_state.current_program = (unsigned int)-1;
    gl_state.bound_texture[0] = (unsigned int)-1;
    gl_state.bound_texture[1] = (unsigned int)-1;
    gl_state.bound_texture[2] = (unsigned int)-1;
    gl_state.bound_texture[3] = (unsigned int)-1;
}

void
rw_gl_state_flush_render(void)
{
    uint32_t *rs = rw_engine.render_states;

    gl_set_depth_test(rs[RW_STATE_ZTESTENABLE]);
    gl_set_depth_write(rs[RW_STATE_ZWRITEENABLE]);

    switch (rs[RW_STATE_CULLMODE]) {
    case RW_CULL_NONE:  gl_set_cull(0, GL_BACK);   break;
    case RW_CULL_CW:    gl_set_cull(1, GL_FRONT);   break;
    case RW_CULL_CCW:   gl_set_cull(1, GL_BACK);    break;
    }

    {
        static const int blend_map[] = {
            GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
            GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
            GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA
        };
        int src = (rs[RW_STATE_SRCBLEND] < 8) ? blend_map[rs[RW_STATE_SRCBLEND]] : GL_ONE;
        int dst = (rs[RW_STATE_DESTBLEND] < 8) ? blend_map[rs[RW_STATE_DESTBLEND]] : GL_ZERO;
        int do_blend = (rs[RW_STATE_SRCBLEND] != RW_BLEND_ONE || rs[RW_STATE_DESTBLEND] != RW_BLEND_ZERO);
        gl_set_blend(do_blend, src, dst);
    }
}

/* ---- Shader Uniforms ---- */

static unsigned int shader_programs[SHADER_COUNT];
static RwShaderUniforms shader_uniforms[SHADER_COUNT];
static unsigned int white_texture;

static const char *shader_uniform_names[U_NUM_UNIFORMS] = {
    "u_proj", "u_view", "u_world", "u_amb_light",
    "u_light_params[0]", "u_light_position[0]", "u_light_direction[0]", "u_light_color[0]",
    "u_mat_color", "u_surf_props", "u_alpha_ref",
    "u_fog_data", "u_fog_color", "u_bone_matrices[0]", "u_xform"
};

/* ---- Shader Sources ---- */

static const char *default_vert_src =
    "precision highp float;\n"
    "uniform mat4 u_proj;\n"
    "uniform mat4 u_view;\n"
    "uniform mat4 u_world;\n"
    "uniform vec4 u_amb_light;\n"
    "uniform vec4 u_mat_color;\n"
    "uniform vec4 u_surf_props;\n"
    "uniform vec4 u_fog_data;\n"
    "attribute vec3 in_pos;\n"
    "attribute vec3 in_normal;\n"
    "attribute vec2 in_tex0;\n"
    "attribute vec4 in_color;\n"
    "varying vec2 v_tex0;\n"
    "varying vec4 v_color;\n"
    "varying float v_fog;\n"
    "#if defined(DIRECTIONALS) || defined(POINTLIGHTS)\n"
    "uniform vec4 u_light_params[8];\n"
    "uniform vec4 u_light_position[8];\n"
    "uniform vec4 u_light_direction[8];\n"
    "uniform vec4 u_light_color[8];\n"
    "vec3 DoDynamicLight(vec3 V, vec3 N){\n"
    "    vec3 color = vec3(0.0);\n"
    "    for(int i = 0; i < 8; i++){\n"
    "        if(u_light_params[i].x == 0.0) break;\n"
    "        #ifdef DIRECTIONALS\n"
    "        if(u_light_params[i].x == 1.0){\n"
    "            float d = max(0.0, dot(N, -u_light_direction[i].xyz));\n"
    "            color += d * u_light_color[i].rgb;\n"
    "        }\n"
    "        #endif\n"
    "        #ifdef POINTLIGHTS\n"
    "        if(u_light_params[i].x == 2.0){\n"
    "            vec3 L = u_light_position[i].xyz - V;\n"
    "            float dist = length(L);\n"
    "            float atten = max(0.0, 1.0 - dist / u_light_params[i].y);\n"
    "            float d = max(0.0, dot(N, normalize(L)));\n"
    "            color += d * u_light_color[i].rgb * atten;\n"
    "        }\n"
    "        #endif\n"
    "    }\n"
    "    return color;\n"
    "}\n"
    "#endif\n"
    "#ifdef SKIN\n"
    "uniform mat4 u_bone_matrices[64];\n"
    "attribute vec4 in_weights;\n"
    "attribute vec4 in_indices;\n"
    "#endif\n"
    "void main(){\n"
    "    vec4 Vertex;\n"
    "    vec3 Normal;\n"
    "    #ifdef SKIN\n"
    "    vec3 SkinVertex = vec3(0.0);\n"
    "    vec3 SkinNormal = vec3(0.0);\n"
    "    for(int i = 0; i < 4; i++){\n"
    "        int idx = int(in_indices[i] * 255.0 + 0.5);\n"
    "        SkinVertex += (u_bone_matrices[idx] * vec4(in_pos, 1.0)).xyz * in_weights[i];\n"
    "        SkinNormal += (mat3(u_bone_matrices[idx]) * in_normal) * in_weights[i];\n"
    "    }\n"
    "    Vertex = vec4(SkinVertex, 1.0);\n"
    "    Normal = SkinNormal;\n"
    "    #else\n"
    "    Vertex = u_world * vec4(in_pos, 1.0);\n"
    "    Normal = mat3(u_world) * in_normal;\n"
    "    #endif\n"
    "    gl_Position = u_proj * u_view * Vertex;\n"
    "    v_tex0 = in_tex0;\n"
    "    v_color = in_color;\n"
    "    v_color.rgb += u_amb_light.rgb * u_surf_props.x;\n"
    "    #if defined(DIRECTIONALS) || defined(POINTLIGHTS)\n"
    "    v_color.rgb += DoDynamicLight(Vertex.xyz, Normal) * u_surf_props.z;\n"
    "    #endif\n"
    "    v_color = clamp(v_color, 0.0, 1.0) * u_mat_color;\n"
    "    v_fog = clamp((gl_Position.w - u_fog_data.y) * u_fog_data.z, 0.0, 1.0);\n"
    "}\n";

static const char *default_frag_src =
    "precision mediump float;\n"
    "uniform sampler2D tex0;\n"
    "uniform highp vec4 u_alpha_ref;\n"
    "uniform highp vec4 u_fog_data;\n"
    "uniform highp vec4 u_fog_color;\n"
    "varying vec2 v_tex0;\n"
    "varying vec4 v_color;\n"
    "varying float v_fog;\n"
    "void main(){\n"
    "    vec4 color = v_color * texture2D(tex0, vec2(v_tex0.x, 1.0 - v_tex0.y));\n"
    "    if(u_fog_data.x > 0.5)\n"
    "        color.rgb = mix(u_fog_color.rgb, color.rgb, v_fog);\n"
    "    if(u_alpha_ref.x > 0.0 && color.a < u_alpha_ref.x) discard;\n"
    "    gl_FragColor = color;\n"
    "}\n";

static const char *im2d_vert_src =
    "precision highp float;\n"
    "uniform mat4 u_xform;\n"
    "attribute vec3 in_pos;\n"
    "attribute vec4 in_color;\n"
    "attribute vec2 in_tex0;\n"
    "varying vec2 v_tex0;\n"
    "varying vec4 v_color;\n"
    "void main(){\n"
    "    gl_Position = u_xform * vec4(in_pos.xy, 0.0, 1.0);\n"
    "    v_tex0 = in_tex0;\n"
    "    v_color = in_color;\n"
    "}\n";

static const char *im2d_frag_src =
    "precision mediump float;\n"
    "uniform sampler2D tex0;\n"
    "varying vec2 v_tex0;\n"
    "varying vec4 v_color;\n"
    "void main(){\n"
    "    gl_FragColor = v_color * texture2D(tex0, vec2(v_tex0.x, 1.0 - v_tex0.y));\n"
    "}\n";

static const char *im3d_vert_src =
    "precision highp float;\n"
    "uniform mat4 u_proj;\n"
    "uniform mat4 u_view;\n"
    "attribute vec3 in_pos;\n"
    "attribute vec3 in_normal;\n"
    "attribute vec4 in_color;\n"
    "varying vec4 v_color;\n"
    "void main(){\n"
    "    gl_Position = u_proj * u_view * vec4(in_pos, 1.0);\n"
    "    v_color = in_color;\n"
    "}\n";

static const char *im3d_lit_vert_src =
    "precision highp float;\n"
    "uniform mat4 u_proj;\n"
    "uniform mat4 u_view;\n"
    "uniform mat4 u_world;\n"
    "uniform vec4 u_amb_light;\n"
    "uniform vec4 u_mat_color;\n"
    "uniform vec4 u_surf_props;\n"
    "uniform vec4 u_light_params[8];\n"
    "uniform vec4 u_light_position[8];\n"
    "uniform vec4 u_light_direction[8];\n"
    "uniform vec4 u_light_color[8];\n"
    "attribute vec3 in_pos;\n"
    "attribute vec3 in_normal;\n"
    "attribute vec4 in_color;\n"
    "varying vec4 v_color;\n"
    "vec3 DoDynamicLight(vec3 V, vec3 N){\n"
    "    vec3 color = vec3(0.0);\n"
    "    for(int i = 0; i < 8; i++){\n"
    "        if(u_light_params[i].x == 0.0) break;\n"
    "        if(u_light_params[i].x == 1.0){\n"
    "            float d = max(0.0, dot(N, -u_light_direction[i].xyz));\n"
    "            color += d * u_light_color[i].rgb;\n"
    "        }\n"
    "        if(u_light_params[i].x == 2.0){\n"
    "            vec3 L = u_light_position[i].xyz - V;\n"
    "            float dist = length(L);\n"
    "            float atten = max(0.0, 1.0 - dist / u_light_params[i].y);\n"
    "            float d = max(0.0, dot(N, normalize(L)));\n"
    "            color += d * u_light_color[i].rgb * atten;\n"
    "        }\n"
    "    }\n"
    "    return color;\n"
    "}\n"
    "void main(){\n"
    "    vec4 Vertex = u_world * vec4(in_pos, 1.0);\n"
    "    gl_Position = u_proj * u_view * Vertex;\n"
    "    v_color = in_color;\n"
    "    v_color.rgb += u_amb_light.rgb * u_surf_props.x;\n"
    "    v_color.rgb += DoDynamicLight(Vertex.xyz, mat3(u_world) * in_normal) * u_surf_props.z;\n"
    "    v_color = clamp(v_color, 0.0, 1.0) * u_mat_color;\n"
    "}\n";

static const char *im3d_frag_src =
    "precision mediump float;\n"
    "varying vec4 v_color;\n"
    "void main(){\n"
    "    gl_FragColor = v_color;\n"
    "}\n";

/* ---- Shader Compile/Link ---- */

static unsigned int
compile_shader(GLenum type, const char **sources, int count)
{
    unsigned int shader = glCreateShader(type);
    GLint ok;
    glShaderSource(shader, count, sources, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "rw: shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static unsigned int
link_program(unsigned int vs, unsigned int fs)
{
    unsigned int prog = glCreateProgram();
    GLint ok;

    glBindAttribLocation(prog, 0, "in_pos");
    glBindAttribLocation(prog, 1, "in_normal");
    glBindAttribLocation(prog, 2, "in_tex0");
    glBindAttribLocation(prog, 3, "in_color");
    glBindAttribLocation(prog, 4, "in_weights");
    glBindAttribLocation(prog, 5, "in_indices");

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "rw: program link error: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

static void
cache_uniforms(RwShaderUniforms *u, unsigned int prog)
{
    int i;
    u->program = prog;
    for (i = 0; i < U_NUM_UNIFORMS; i++)
        u->locations[i] = glGetUniformLocation(prog, shader_uniform_names[i]);
}

static int
build_shader(int index, const char *defines, const char *vert_src, const char *frag_src)
{
    const char *version = "#version 100\n";
    const char *vert_parts[4];
    const char *frag_parts[2];
    int vert_count = 0, frag_count = 0;
    unsigned int vs, fs, prog;

    vert_parts[vert_count++] = version;
    if (defines && defines[0])
        vert_parts[vert_count++] = defines;
    vert_parts[vert_count++] = vert_src;

    vs = compile_shader(GL_VERTEX_SHADER, vert_parts, vert_count);
    if (!vs) return 0;

    frag_parts[frag_count++] = version;
    frag_parts[frag_count++] = frag_src;
    fs = compile_shader(GL_FRAGMENT_SHADER, frag_parts, frag_count);
    if (!fs) { glDeleteShader(vs); return 0; }

    prog = link_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) return 0;

    shader_programs[index] = prog;
    cache_uniforms(&shader_uniforms[index], prog);
    return 1;
}

static int
compile_all_shaders(void)
{
    if (!build_shader(SHADER_DEFAULT, "",
            default_vert_src, default_frag_src)) return 0;
    if (!build_shader(SHADER_DEFAULT_DIR,
            "#define DIRECTIONALS\n",
            default_vert_src, default_frag_src)) return 0;
    if (!build_shader(SHADER_DEFAULT_DIR_POINT,
            "#define DIRECTIONALS\n#define POINTLIGHTS\n",
            default_vert_src, default_frag_src)) return 0;
    if (!build_shader(SHADER_SKIN,
            "#define SKIN\n",
            default_vert_src, default_frag_src)) return 0;
    if (!build_shader(SHADER_SKIN_DIR_POINT,
            "#define DIRECTIONALS\n#define POINTLIGHTS\n#define SKIN\n",
            default_vert_src, default_frag_src)) return 0;
    if (!build_shader(SHADER_IM2D, "",
            im2d_vert_src, im2d_frag_src)) return 0;
    if (!build_shader(SHADER_IM3D, "",
            im3d_vert_src, im3d_frag_src)) return 0;
    if (!build_shader(SHADER_IM3D_LIT, "",
            im3d_lit_vert_src, im3d_frag_src)) return 0;
    return 1;
}

static void
delete_all_shaders(void)
{
    int i;
    for (i = 0; i < SHADER_COUNT; i++) {
        if (shader_programs[i]) {
            glDeleteProgram(shader_programs[i]);
            shader_programs[i] = 0;
        }
    }
}

/* ---- Uniform Helpers ---- */

void
rw_gl_set_uniform_mat4(int loc, const RwRawMatrix *m)
{
    if (loc >= 0)
        glUniformMatrix4fv(loc, 1, GL_FALSE, (const float *)m);
}

void
rw_gl_set_uniform_vec4(int loc, const float *v)
{
    if (loc >= 0)
        glUniform4fv(loc, 1, v);
}

void
rw_gl_upload_matrix(int loc, const RwMatrix *m)
{
    RwRawMatrix raw;
    rw_matrix_to_raw(&raw, m);
    rw_gl_set_uniform_mat4(loc, &raw);
}

void
rw_gl_upload_raw_matrix(int loc, const RwRawMatrix *raw)
{
    rw_gl_set_uniform_mat4(loc, raw);
}

/* ---- Texture Upload ---- */

void
rw_gl_upload_raster(RwRaster *r)
{
    unsigned int texid;
    if (!r || !r->pixels) return;

    if (r->gl.texid)
        glDeleteTextures(1, &r->gl.texid);

    glGenTextures(1, &texid);
    rw_gl_state_bind_texture(0, texid);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 r->width, r->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, r->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    r->gl.texid = texid;
    r->gl.internal_format = GL_RGBA;
    r->gl.gl_format = GL_RGBA;
    r->gl.gl_type = GL_UNSIGNED_BYTE;
    r->gl.bpp = 4;
}

void
rw_gl_set_texture_sampler(RwRaster *r, int filter, int address_u, int address_v)
{
    static const int filter_map[] = {
        GL_NEAREST, GL_LINEAR,
        GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST,
        GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR
    };
    static const int wrap_map[] = {
        GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE
    };

    if (!r || !r->gl.texid) return;

    rw_gl_state_bind_texture(0, r->gl.texid);

    if (filter >= 0 && filter < 6)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_map[filter]);
    if (filter == RW_TEX_FILTER_NEAREST || filter == RW_TEX_FILTER_LINEAR) {
        int mag = (filter == RW_TEX_FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
    }
    if (address_u >= 0 && address_u < 4)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_map[address_u]);
    if (address_v >= 0 && address_v < 4)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_map[address_v]);
}

unsigned int
rw_gl_get_white_texture(void)
{
    static const uint8_t white[4] = {255, 255, 255, 255};

    if (!white_texture) {
        glGenTextures(1, &white_texture);
        rw_gl_state_bind_texture(0, white_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    return white_texture;
}

/* ---- Device Init/Shutdown ---- */

int
rw_gl_device_init(void)
{
    gl_state_reset();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    return 1;
}

int
rw_gl_device_shutdown(void)
{
    if (white_texture) {
        glDeleteTextures(1, &white_texture);
        white_texture = 0;
    }
    delete_all_shaders();
    gl_state_reset();
    return 1;
}

/* ---- Camera ---- */

void
rw_gl_set_camera(RwCamera *cam)
{
    int i;
    int width, height;

    if (!cam) return;

    width = cam->frame_buffer ? cam->frame_buffer->width : 0;
    height = cam->frame_buffer ? cam->frame_buffer->height : 0;
    if (width > 0 && height > 0)
        glViewport(0, 0, width, height);

    for (i = SHADER_DEFAULT; i < SHADER_COUNT; i++) {
        if (!shader_programs[i]) continue;
        rw_gl_state_use_program(shader_programs[i]);
        rw_gl_upload_raw_matrix(shader_uniforms[i].locations[U_PROJ], &cam->dev_proj);
        rw_gl_upload_raw_matrix(shader_uniforms[i].locations[U_VIEW], &cam->dev_view);
    }
}

/* ---- Lighting ---- */

void
rw_gl_set_lights(RwWorldLights *lights, int shader_idx)
{
    RwShaderUniforms *u;
    float params[8][4], positions[8][4], directions[8][4], colors[8][4];
    int i, idx;

    if (!lights || shader_idx < 0 || shader_idx >= SHADER_COUNT) return;
    if (!shader_programs[shader_idx]) return;

    u = &shader_uniforms[shader_idx];
    rw_gl_state_use_program(shader_programs[shader_idx]);
    rw_gl_set_uniform_vec4(u->locations[U_AMB_LIGHT], &lights->ambient.r);

    memset(params, 0, sizeof(params));
    memset(positions, 0, sizeof(positions));
    memset(directions, 0, sizeof(directions));
    memset(colors, 0, sizeof(colors));

    for (i = 0; i < lights->num_directionals && i < 8; i++) {
        RwLight *l = lights->directionals[i];
        RwMatrix *ltm = rw_frame_get_ltm(l->frame);
        params[i][0] = 1.0f;
        directions[i][0] = -ltm->at.x;
        directions[i][1] = -ltm->at.y;
        directions[i][2] = -ltm->at.z;
        colors[i][0] = l->color.r;
        colors[i][1] = l->color.g;
        colors[i][2] = l->color.b;
        colors[i][3] = l->color.a;
    }

    for (i = 0; i < lights->num_locals && i < 8; i++) {
        idx = lights->num_directionals + i;
        if (idx >= 8) break;
        {
            RwLight *l = lights->locals[i];
            RwMatrix *ltm = rw_frame_get_ltm(l->frame);
            params[idx][0] = 2.0f;
            params[idx][1] = l->radius;
            positions[idx][0] = ltm->pos.x;
            positions[idx][1] = ltm->pos.y;
            positions[idx][2] = ltm->pos.z;
            positions[idx][3] = 1.0f;
            colors[idx][0] = l->color.r;
            colors[idx][1] = l->color.g;
            colors[idx][2] = l->color.b;
            colors[idx][3] = l->color.a;
        }
    }

    if (u->locations[U_LIGHT_PARAMS] >= 0)
        glUniform4fv(u->locations[U_LIGHT_PARAMS], 8, &params[0][0]);
    if (u->locations[U_LIGHT_POSITION] >= 0)
        glUniform4fv(u->locations[U_LIGHT_POSITION], 8, &positions[0][0]);
    if (u->locations[U_LIGHT_DIRECTION] >= 0)
        glUniform4fv(u->locations[U_LIGHT_DIRECTION], 8, &directions[0][0]);
    if (u->locations[U_LIGHT_COLOR] >= 0)
        glUniform4fv(u->locations[U_LIGHT_COLOR], 8, &colors[0][0]);
}

/* ---- Skin ---- */

void
rw_gl_upload_skin_matrices(RwHAnimHier *hier, RwSkin *skin, int shader_idx)
{
    RwShaderUniforms *u;
    float matrices[64][16];
    int i;

    if (!hier || !skin || shader_idx < 0 || shader_idx >= SHADER_COUNT) return;
    if (!shader_programs[shader_idx]) return;

    u = &shader_uniforms[shader_idx];
    if (u->locations[U_BONE_MATRICES] < 0) return;

    rw_gl_state_use_program(shader_programs[shader_idx]);

    for (i = 0; i < hier->num_nodes && i < 64; i++) {
        RwRawMatrix world_raw, inv_raw, result;
        float *inv = &skin->inverse_matrices[i * 16];

        rw_matrix_to_raw(&world_raw, &hier->matrices[i]);

        inv_raw.right.x  = inv[0];  inv_raw.right.y  = inv[1];
        inv_raw.right.z  = inv[2];  inv_raw.rightw   = inv[3];
        inv_raw.up.x     = inv[4];  inv_raw.up.y     = inv[5];
        inv_raw.up.z     = inv[6];  inv_raw.upw      = inv[7];
        inv_raw.at.x     = inv[8];  inv_raw.at.y     = inv[9];
        inv_raw.at.z     = inv[10]; inv_raw.atw      = inv[11];
        inv_raw.pos.x    = inv[12]; inv_raw.pos.y    = inv[13];
        inv_raw.pos.z    = inv[14]; inv_raw.posw     = inv[15];

        rw_raw_matrix_multiply(&result, &world_raw, &inv_raw);
        memcpy(matrices[i], &result, 64);
    }

    glUniformMatrix4fv(u->locations[U_BONE_MATRICES],
                       hier->num_nodes < 64 ? hier->num_nodes : 64,
                       GL_FALSE, &matrices[0][0]);
}

/* ---- Shader Selection ---- */

int
rw_gl_select_shader(int num_dir, int num_point, int is_skin)
{
    if (is_skin) {
        if (num_dir > 0 || num_point > 0)
            return SHADER_SKIN_DIR_POINT;
        return SHADER_SKIN;
    }
    if (num_dir > 0 && num_point > 0)
        return SHADER_DEFAULT_DIR_POINT;
    if (num_dir > 0)
        return SHADER_DEFAULT_DIR;
    return SHADER_DEFAULT;
}

unsigned int
rw_gl_get_program(int shader_idx)
{
    if (shader_idx >= 0 && shader_idx < SHADER_COUNT)
        return shader_programs[shader_idx];
    return 0;
}

RwShaderUniforms *
rw_gl_get_uniforms(int shader_idx)
{
    if (shader_idx >= 0 && shader_idx < SHADER_COUNT)
        return &shader_uniforms[shader_idx];
    return NULL;
}

/* ---- Engine Hooks ---- */

int
rw_gl_open(void)
{
    rw_gl_device_init();
    return 1;
}

int
rw_gl_start(void)
{
    return compile_all_shaders();
}

void
rw_gl_stop(void)
{
    delete_all_shaders();
}

void
rw_gl_close(void)
{
    rw_gl_device_shutdown();
}
