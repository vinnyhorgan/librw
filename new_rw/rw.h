#ifndef RW_H
#define RW_H

#include <stdint.h>
#include <stddef.h>
#include <math.h>

typedef int32_t  rw_int32;
typedef uint32_t rw_uint32;
typedef int16_t  rw_int16;
typedef uint16_t rw_uint16;
typedef int8_t   rw_int8;
typedef uint8_t  rw_uint8;
typedef float    rw_float32;
typedef int      rw_bool32;

typedef struct { float x, y; }          RwV2d;
typedef struct { float x, y, z; }       RwV3d;
typedef struct { float x, y, z, w; }    RwV4d;
typedef struct { float x, y, z, w; }    RwQuat;
typedef struct { float r, g, b, a; }    RwRGBAf;
typedef struct { uint8_t r, g, b, a; }  RwRGBA;
typedef struct { float u, v; }          RwTexCoords;

typedef struct {
    RwV3d right;   uint32_t _pad0;
    RwV3d up;      uint32_t _pad1;
    RwV3d at;      uint32_t _pad2;
    RwV3d pos;     uint32_t _pad3;
} RwMatrix;

typedef struct {
    RwV3d right;  float rightw;
    RwV3d up;     float upw;
    RwV3d at;     float atw;
    RwV3d pos;    float posw;
} RwRawMatrix;

typedef struct { float x, y, z, w; uint8_t r, g, b, a; float u, v; } RwIm2DVertex;
typedef struct { RwV3d pos; RwV3d normal; uint8_t r, g, b, a; float u, v; } RwIm3DVertex;

typedef struct { RwV3d sup, inf; }               RwBBox;
typedef struct { RwV3d center; float radius; }    RwSphere;
typedef struct { int32_t x, y, w, h; }           RwRect;

typedef struct RwLink { struct RwLink *next, *prev; } RwLink;
typedef struct { RwLink link; } RwLinkList;

typedef struct RwFrame       RwFrame;
typedef struct RwGeometry    RwGeometry;
typedef struct RwAtomic      RwAtomic;
typedef struct RwClump       RwClump;
typedef struct RwWorld       RwWorld;
typedef struct RwCamera      RwCamera;
typedef struct RwLight       RwLight;
typedef struct RwMaterial    RwMaterial;
typedef struct RwTexture     RwTexture;
typedef struct RwRaster      RwRaster;
typedef struct RwImage       RwImage;
typedef struct RwTexDict     RwTexDict;
typedef struct RwObjPipeline RwObjPipeline;
typedef struct RwSkin        RwSkin;
typedef struct RwHAnimHier   RwHAnimHier;

typedef enum {
    RW_LIGHT_AMBIENT,
    RW_LIGHT_DIRECTIONAL,
    RW_LIGHT_POINT,
    RW_LIGHT_SPOT
} RwLightType;

typedef enum {
    RW_COMBINE_REPLACE,
    RW_COMBINE_PRECONCAT,
    RW_COMBINE_POSTCONCAT
} RwCombineOp;

typedef enum {
    RW_PRIM_NONE,
    RW_PRIM_LINELIST,
    RW_PRIM_POLYLINE,
    RW_PRIM_TRILIST,
    RW_PRIM_TRISTRIP,
    RW_PRIM_TRIFAN,
    RW_PRIM_POINTLIST
} RwPrimitiveType;

enum {
    RW_GEO_TRISTRIP   = 0x01,
    RW_GEO_POSITIONS  = 0x02,
    RW_GEO_TEXTURED   = 0x04,
    RW_GEO_PRELIT     = 0x08,
    RW_GEO_NORMALS    = 0x10,
    RW_GEO_LIGHT      = 0x20,
    RW_GEO_MODULATE   = 0x40,
};

enum {
    RW_LOCK_POLYGONS  = 0x0001,
    RW_LOCK_VERTICES  = 0x0002,
    RW_LOCK_NORMALS   = 0x0004,
    RW_LOCK_PRELIGHT  = 0x0008,
    RW_LOCK_TEXCOORDS = 0x0010,
    RW_LOCK_ALL       = 0x0FFF,
};

enum {
    RW_STATE_ZTESTENABLE,
    RW_STATE_ZWRITEENABLE,
    RW_STATE_CULLMODE,
    RW_STATE_SRCBLEND,
    RW_STATE_DESTBLEND,
    RW_STATE_ALPHATEST,
    RW_STATE_ALPHAREF,
    RW_STATE_FOGENABLE,
    RW_STATE_FOGCOLOR,
    RW_STATE_TEXTURERASTER,
    RW_STATE_VERTEXALPHA,
    RW_STATE_NUM_STATES
};

enum {
    RW_CULL_NONE,
    RW_CULL_CW,
    RW_CULL_CCW,
};

enum {
    RW_BLEND_ZERO,
    RW_BLEND_ONE,
    RW_BLEND_SRCCOLOR,
    RW_BLEND_INVSRCCOLOR,
    RW_BLEND_SRCALPHA,
    RW_BLEND_INVSRCALPHA,
    RW_BLEND_DESTALPHA,
    RW_BLEND_INVDESTALPHA,
};

enum {
    RW_TEX_FILTER_NEAREST,
    RW_TEX_FILTER_LINEAR,
    RW_TEX_FILTER_MIP_NEAREST,
    RW_TEX_FILTER_MIP_LINEAR,
    RW_TEX_FILTER_LINEAR_MIP_NEAREST,
    RW_TEX_FILTER_LINEAR_MIP_LINEAR,
};

enum {
    RW_TEX_WRAP_NONE,
    RW_TEX_WRAP_WRAP,
    RW_TEX_WRAP_MIRROR,
    RW_TEX_WRAP_CLAMP,
};

enum {
    RW_CAMERA_CLEAR_IMAGE   = 0x01,
    RW_CAMERA_CLEAR_Z       = 0x02,
    RW_CAMERA_CLEAR_STENCIL = 0x04,
};

enum {
    RW_IM3D_VERTEXXYZ  = 0x01,
    RW_IM3D_VERTEXUV   = 0x02,
    RW_IM3D_VERTEXRGBA = 0x04,
    RW_IM3D_LIGHTING   = 0x08,
};

enum {
    RW_FRAME_SUBTREESYNC   = 0x01,
    RW_FRAME_HIERARCHYSYNC = 0x02,
};

struct RwFrame {
    uint8_t type, subtype, flags, priv_flags;
    RwMatrix matrix;
    RwMatrix ltm;
    RwFrame *parent;
    RwFrame *child;
    RwFrame *next;
    RwFrame *root;
    RwLink in_dirty;
    uint32_t dirty;
};

typedef struct {
    uint16_t v[3];
    uint16_t mat_id;
} RwTriangle;

typedef struct {
    uint16_t *indices;
    uint32_t  num_indices;
    RwMaterial *material;
} RwMesh;

typedef struct {
    uint32_t flags;
    uint16_t num_meshes;
    uint32_t total_indices;
} RwMeshHeader;

typedef struct {
    RwV3d *vertices;
    RwV3d *normals;
    RwSphere bounding_sphere;
} RwMorphTarget;

struct RwGeometry {
    uint8_t type, subtype, flags, priv_flags;
    uint32_t geo_flags;
    uint16_t locked;
    int num_vertices;
    int num_triangles;
    RwTriangle *triangles;
    RwRGBA *colors;
    RwTexCoords *texcoords;
    RwMorphTarget morph_target;
    RwMaterial **materials;
    int num_materials;
    int mat_space;
    RwMeshHeader *mesh_header;
    void *gl_data;
    int ref_count;
    RwSkin *skin;
};

struct RwMaterial {
    RwTexture *texture;
    RwRGBA color;
    struct { float ambient, specular, diffuse; } surface;
    RwObjPipeline *pipeline;
    int ref_count;
};

typedef struct {
    int internal_format;
    int gl_format;
    int gl_type;
    int bpp;
    unsigned int texid;
    int filter, address_u, address_v;
    int has_alpha;
} RwGlRaster;

struct RwRaster {
    int width, height, depth;
    uint32_t flags;
    RwGlRaster gl;
    uint8_t *pixels;
};

struct RwImage {
    int width, height, depth, bpp;
    int stride;
    uint8_t *pixels;
};

struct RwTexture {
    RwRaster *raster;
    RwTexDict *dict;
    RwLink in_dict;
    char name[32];
    char mask[32];
    uint32_t filter_addressing;
    int ref_count;
};

struct RwTexDict {
    RwLinkList textures;
    RwLink in_global;
};

struct RwAtomic {
    uint8_t type, subtype, flags, priv_flags;
    RwFrame *frame;
    RwGeometry *geometry;
    RwSphere bounding_sphere;
    RwClump *clump;
    RwLink in_clump;
    RwObjPipeline *pipeline;
    void (*render_cb)(RwAtomic *atomic);
    RwWorld *world;
};

struct RwClump {
    uint8_t type, subtype, flags, priv_flags;
    RwFrame *frame;
    RwLinkList atomics;
    RwLinkList lights;
    RwLinkList cameras;
    RwWorld *world;
    RwLink in_world;
};

struct RwCamera {
    uint8_t type, subtype, flags, priv_flags;
    RwFrame *frame;
    RwV2d view_window;
    RwV2d view_offset;
    float near_plane, far_plane, fog_plane;
    int projection;
    RwMatrix view_matrix;
    RwRawMatrix dev_view, dev_proj;
    float z_scale, z_shift;
    struct { RwV3d normal; float dist; uint8_t cx, cy, cz; } frustum[6];
    RwRaster *frame_buffer;
    RwRaster *z_buffer;
    RwClump *clump;
    RwWorld *world;
};

struct RwLight {
    uint8_t type, subtype, flags, priv_flags;
    RwFrame *frame;
    float radius;
    RwRGBAf color;
    float minus_cos_angle;
    RwLink in_world;
    RwClump *clump;
    RwWorld *world;
};

typedef struct {
    int num_ambients;
    RwRGBAf ambient;
    int num_directionals;
    RwLight *directionals[8];
    int num_locals;
    RwLight *locals[8];
} RwWorldLights;

struct RwWorld {
    uint8_t type, subtype, flags, priv_flags;
    RwLinkList local_lights;
    RwLinkList global_lights;
    RwLinkList clumps;
};

struct RwObjPipeline {
    void (*instance)(RwAtomic *atomic);
    void (*uninstance)(RwAtomic *atomic);
    void (*render)(RwAtomic *atomic);
};

struct RwSkin {
    int num_bones;
    int num_weights;
    float *inverse_matrices;
    uint8_t *bone_indices;
    float *weights;
};

typedef struct {
    int32_t id;
    int32_t index;
    int32_t flags;
    RwFrame *frame;
} RwHAnimNodeInfo;

typedef struct {
    RwQuat q;
    RwV3d t;
    float time;
} RwHAnimKeyFrame;

struct RwHAnimHier {
    int num_nodes;
    RwHAnimNodeInfo *nodes;
    RwMatrix *matrices;
    RwHAnimKeyFrame *keyframes;
    int num_keyframes;
    float current_time;
    int num_frames;
    float frame_rate;
    RwHAnimKeyFrame *anim_data;
};

typedef struct {
    struct RwCamera *current_camera;
    struct RwWorld  *current_world;
    RwLinkList frame_dirty_list;
    uint32_t render_states[RW_STATE_NUM_STATES];
    void *(*malloc)(size_t);
    void *(*realloc)(void*, size_t);
    void  (*free)(void*);
} RwEngine;

extern RwEngine rw_engine;

typedef struct {
    void *(*malloc)(size_t);
    void *(*realloc)(void*, size_t);
    void  (*free)(void*);
} RwMemoryFuncs;

enum {
    RW_ENGINE_DEAD,
    RW_ENGINE_INITIALIZED,
    RW_ENGINE_OPENED,
    RW_ENGINE_STARTED,
};

int  rw_engine_init(RwMemoryFuncs *memfuncs);
int  rw_engine_open(void);
int  rw_engine_start(void);
void rw_engine_stop(void);
void rw_engine_close(void);
void rw_engine_term(void);

void     rw_set_render_state(int state, uint32_t value);
uint32_t rw_get_render_state(int state);

static inline RwV3d
rw_v3d_add(RwV3d a, RwV3d b)
{
    RwV3d r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static inline RwV3d
rw_v3d_sub(RwV3d a, RwV3d b)
{
    RwV3d r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

static inline RwV3d
rw_v3d_scale(RwV3d v, float s)
{
    RwV3d r;
    r.x = v.x * s;
    r.y = v.y * s;
    r.z = v.z * s;
    return r;
}

static inline float
rw_v3d_dot(RwV3d a, RwV3d b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline RwV3d
rw_v3d_cross(RwV3d a, RwV3d b)
{
    RwV3d r;
    r.x = a.y*b.z - a.z*b.y;
    r.y = a.z*b.x - a.x*b.z;
    r.z = a.x*b.y - a.y*b.x;
    return r;
}

static inline float
rw_v3d_length(RwV3d v)
{
    return sqrtf(rw_v3d_dot(v, v));
}

static inline RwV3d
rw_v3d_normalize(RwV3d v)
{
    float len = rw_v3d_length(v);
    if (len > 0.0f)
        return rw_v3d_scale(v, 1.0f / len);
    RwV3d zero = {0, 0, 0};
    return zero;
}

static inline RwV3d
rw_v3d_lerp(RwV3d a, RwV3d b, float t)
{
    RwV3d r;
    r.x = a.x + (b.x - a.x) * t;
    r.y = a.y + (b.y - a.y) * t;
    r.z = a.z + (b.z - a.z) * t;
    return r;
}

static inline void
rw_matrix_set_identity(RwMatrix *m)
{
    static const RwMatrix identity = {
        {1,0,0}, 0,
        {0,1,0}, 0,
        {0,0,1}, 0,
        {0,0,0}, 0,
    };
    *m = identity;
}

static inline void
rw_matrix_multiply(RwMatrix *out, const RwMatrix *a, const RwMatrix *b)
{
    out->right.x = a->right.x*b->right.x + a->right.y*b->up.x + a->right.z*b->at.x;
    out->right.y = a->right.x*b->right.y + a->right.y*b->up.y + a->right.z*b->at.y;
    out->right.z = a->right.x*b->right.z + a->right.y*b->up.z + a->right.z*b->at.z;

    out->up.x = a->up.x*b->right.x + a->up.y*b->up.x + a->up.z*b->at.x;
    out->up.y = a->up.x*b->right.y + a->up.y*b->up.y + a->up.z*b->at.y;
    out->up.z = a->up.x*b->right.z + a->up.y*b->up.z + a->up.z*b->at.z;

    out->at.x = a->at.x*b->right.x + a->at.y*b->up.x + a->at.z*b->at.x;
    out->at.y = a->at.x*b->right.y + a->at.y*b->up.y + a->at.z*b->at.y;
    out->at.z = a->at.x*b->right.z + a->at.y*b->up.z + a->at.z*b->at.z;

    out->pos.x = a->pos.x*b->right.x + a->pos.y*b->up.x + a->pos.z*b->at.x + b->pos.x;
    out->pos.y = a->pos.x*b->right.y + a->pos.y*b->up.y + a->pos.z*b->at.y + b->pos.y;
    out->pos.z = a->pos.x*b->right.z + a->pos.y*b->up.z + a->pos.z*b->at.z + b->pos.z;
}

static inline int
rw_matrix_invert(RwMatrix *out, const RwMatrix *in)
{
    float det;
    float invdet;
    float fA0 = in->right.x*in->up.y - in->right.y*in->up.x;
    float fA1 = in->right.x*in->up.z - in->right.z*in->up.x;
    float fA2 = in->right.x*in->at.y - in->right.y*in->at.x;
    float fA3 = in->right.x*in->at.z - in->right.z*in->at.x;
    float fA4 = in->up.x*in->at.y - in->up.y*in->at.x;
    float fA5 = in->up.x*in->at.z - in->up.z*in->at.x;

    det = fA0*in->at.z - fA1*in->at.y + fA2*in->up.z - fA3*in->up.y + fA4*in->right.z - fA5*in->right.y;
    if (det == 0.0f)
        return 0;
    invdet = 1.0f / det;

    out->right.x = ( in->up.y*in->at.z - in->up.z*in->at.y) * invdet;
    out->right.y = ( in->right.z*in->at.y - in->right.y*in->at.z) * invdet;
    out->right.z = ( in->right.y*in->up.z - in->right.z*in->up.y) * invdet;

    out->up.x = ( in->up.z*in->at.x - in->up.x*in->at.z) * invdet;
    out->up.y = ( in->right.x*in->at.z - in->right.z*in->at.x) * invdet;
    out->up.z = ( in->right.z*in->up.x - in->right.x*in->up.z) * invdet;

    out->at.x = ( in->up.x*in->at.y - in->up.y*in->at.x) * invdet;
    out->at.y = ( in->right.y*in->at.x - in->right.x*in->at.y) * invdet;
    out->at.z = ( in->right.x*in->up.y - in->right.y*in->up.x) * invdet;

    out->pos.x = -(in->pos.x*out->right.x + in->pos.y*out->up.x + in->pos.z*out->at.x);
    out->pos.y = -(in->pos.x*out->right.y + in->pos.y*out->up.y + in->pos.z*out->at.y);
    out->pos.z = -(in->pos.x*out->right.z + in->pos.y*out->up.z + in->pos.z*out->at.z);

    return 1;
}

static inline void
rw_matrix_translate(RwMatrix *m, RwV3d v, RwCombineOp op)
{
    RwMatrix tmp;
    rw_matrix_set_identity(&tmp);
    tmp.pos = v;
    switch (op) {
    case RW_COMBINE_REPLACE:
        *m = tmp;
        break;
    case RW_COMBINE_PRECONCAT: {
        RwMatrix t;
        rw_matrix_multiply(&t, &tmp, m);
        *m = t;
        break;
    }
    case RW_COMBINE_POSTCONCAT: {
        RwMatrix t;
        rw_matrix_multiply(&t, m, &tmp);
        *m = t;
        break;
    }
    }
}

static inline void
rw_matrix_rotate(RwMatrix *m, RwV3d axis, float angle, RwCombineOp op)
{
    RwMatrix tmp;
    float c, s, t;
    float xx, yy, zz, xy, yz, zx;
    float radians = angle * (3.14159265358979323846f / 180.0f);
    RwV3d n = rw_v3d_normalize(axis);

    c = cosf(radians);
    s = sinf(radians);
    t = 1.0f - c;

    xx = n.x*n.x; yy = n.y*n.y; zz = n.z*n.z;
    xy = n.x*n.y; yz = n.y*n.z; zx = n.z*n.x;

    rw_matrix_set_identity(&tmp);
    tmp.right.x = t*xx + c;
    tmp.right.y = t*xy + s*n.z;
    tmp.right.z = t*zx - s*n.y;
    tmp.up.x    = t*xy - s*n.z;
    tmp.up.y    = t*yy + c;
    tmp.up.z    = t*yz + s*n.x;
    tmp.at.x    = t*zx + s*n.y;
    tmp.at.y    = t*yz - s*n.x;
    tmp.at.z    = t*zz + c;

    switch (op) {
    case RW_COMBINE_REPLACE:
        *m = tmp;
        break;
    case RW_COMBINE_PRECONCAT: {
        RwMatrix t2;
        rw_matrix_multiply(&t2, &tmp, m);
        *m = t2;
        break;
    }
    case RW_COMBINE_POSTCONCAT: {
        RwMatrix t2;
        rw_matrix_multiply(&t2, m, &tmp);
        *m = t2;
        break;
    }
    }
}

static inline void
rw_matrix_scale(RwMatrix *m, RwV3d s, RwCombineOp op)
{
    RwMatrix tmp;
    rw_matrix_set_identity(&tmp);
    tmp.right.x = s.x;
    tmp.up.y = s.y;
    tmp.at.z = s.z;

    switch (op) {
    case RW_COMBINE_REPLACE:
        *m = tmp;
        break;
    case RW_COMBINE_PRECONCAT: {
        RwMatrix t;
        rw_matrix_multiply(&t, &tmp, m);
        *m = t;
        break;
    }
    case RW_COMBINE_POSTCONCAT: {
        RwMatrix t;
        rw_matrix_multiply(&t, m, &tmp);
        *m = t;
        break;
    }
    }
}

static inline void
rw_matrix_to_raw(RwRawMatrix *out, const RwMatrix *in)
{
    out->right  = in->right;  out->rightw  = 0.0f;
    out->up     = in->up;     out->upw     = 0.0f;
    out->at     = in->at;     out->atw     = 0.0f;
    out->pos    = in->pos;    out->posw    = 1.0f;
}

static inline void
rw_raw_matrix_multiply(RwRawMatrix *out, const RwRawMatrix *a, const RwRawMatrix *b)
{
    out->right.x = a->right.x*b->right.x + a->right.y*b->up.x + a->right.z*b->at.x + a->rightw*b->pos.x;
    out->right.y = a->right.x*b->right.y + a->right.y*b->up.y + a->right.z*b->at.y + a->rightw*b->pos.y;
    out->right.z = a->right.x*b->right.z + a->right.y*b->up.z + a->right.z*b->at.z + a->rightw*b->pos.z;
    out->rightw  = a->right.x*b->rightw  + a->right.y*b->upw  + a->right.z*b->atw  + a->rightw*b->posw;

    out->up.x = a->up.x*b->right.x + a->up.y*b->up.x + a->up.z*b->at.x + a->upw*b->pos.x;
    out->up.y = a->up.x*b->right.y + a->up.y*b->up.y + a->up.z*b->at.y + a->upw*b->pos.y;
    out->up.z = a->up.x*b->right.z + a->up.y*b->up.z + a->up.z*b->at.z + a->upw*b->pos.z;
    out->upw  = a->up.x*b->rightw  + a->up.y*b->upw  + a->up.z*b->atw  + a->upw*b->posw;

    out->at.x = a->at.x*b->right.x + a->at.y*b->up.x + a->at.z*b->at.x + a->atw*b->pos.x;
    out->at.y = a->at.x*b->right.y + a->at.y*b->up.y + a->at.z*b->at.y + a->atw*b->pos.y;
    out->at.z = a->at.x*b->right.z + a->at.y*b->up.z + a->at.z*b->at.z + a->atw*b->pos.z;
    out->atw  = a->at.x*b->rightw  + a->at.y*b->upw  + a->at.z*b->atw  + a->atw*b->posw;

    out->pos.x = a->pos.x*b->right.x + a->pos.y*b->up.x + a->pos.z*b->at.x + a->posw*b->pos.x;
    out->pos.y = a->pos.x*b->right.y + a->pos.y*b->up.y + a->pos.z*b->at.y + a->posw*b->pos.y;
    out->pos.z = a->pos.x*b->right.z + a->pos.y*b->up.z + a->pos.z*b->at.z + a->posw*b->pos.z;
    out->posw  = a->pos.x*b->rightw  + a->pos.y*b->upw  + a->pos.z*b->atw  + a->posw*b->posw;
}

static inline void
rw_raw_matrix_transpose(RwRawMatrix *out, const RwRawMatrix *in)
{
    RwRawMatrix tmp = *in;

    out->right.x = tmp.right.x;
    out->up.x = tmp.right.y;
    out->at.x = tmp.right.z;
    out->pos.x = tmp.rightw;

    out->right.y = tmp.up.x;
    out->up.y = tmp.up.y;
    out->at.y = tmp.up.z;
    out->pos.y = tmp.upw;

    out->right.z = tmp.at.x;
    out->up.z = tmp.at.y;
    out->at.z = tmp.at.z;
    out->pos.z = tmp.atw;

    out->rightw = tmp.pos.x;
    out->upw = tmp.pos.y;
    out->atw = tmp.pos.z;
    out->posw = tmp.posw;
}

static inline RwQuat
rw_quat_mult(RwQuat a, RwQuat b)
{
    RwQuat r;
    r.w = a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z;
    r.x = a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y;
    r.y = a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x;
    r.z = a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w;
    return r;
}

static inline RwQuat
rw_quat_slerp(RwQuat a, RwQuat b, float t)
{
    float c = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
    if (c < 0.0f) {
        b.w = -b.w; b.x = -b.x; b.y = -b.y; b.z = -b.z;
        c = -c;
    }
    if (c > 0.99999f)
        return a;

    float phi = acosf(c);
    float sinp = sinf(phi);
    float sa = sinf((1.0f - t) * phi) / sinp;
    float sb = sinf(t * phi) / sinp;
    RwQuat r;
    r.w = sa*a.w + sb*b.w;
    r.x = sa*a.x + sb*b.x;
    r.y = sa*a.y + sb*b.y;
    r.z = sa*a.z + sb*b.z;
    return r;
}

static inline void
rw_quat_to_matrix(RwMatrix *out, RwQuat q)
{
    float xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
    float yz = q.y*q.z, zx = q.z*q.x, xy = q.x*q.y;
    float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;

    out->right.x = 1.0f - 2.0f*(yy + zz);
    out->right.y = 2.0f*(xy + wz);
    out->right.z = 2.0f*(zx - wy);

    out->up.x = 2.0f*(xy - wz);
    out->up.y = 1.0f - 2.0f*(xx + zz);
    out->up.z = 2.0f*(yz + wx);

    out->at.x = 2.0f*(zx + wy);
    out->at.y = 2.0f*(yz - wx);
    out->at.z = 1.0f - 2.0f*(xx + yy);

    out->pos.x = 0.0f;
    out->pos.y = 0.0f;
    out->pos.z = 0.0f;
}

static inline RwQuat
rw_quat_from_matrix(const RwMatrix *m)
{
    float tr = m->right.x + m->up.y + m->at.z;
    RwQuat q;

    if (tr > 0.0f) {
        float s = sqrtf(tr + 1.0f);
        q.w = s * 0.5f;
        s = 0.5f / s;
        q.x = (m->up.z - m->at.y) * s;
        q.y = (m->at.x - m->right.z) * s;
        q.z = (m->right.y - m->up.x) * s;
    } else if (m->right.x >= m->up.y && m->right.x >= m->at.z) {
        float s = sqrtf(1.0f + m->right.x - m->up.y - m->at.z);
        q.x = s * 0.5f;
        s = 0.5f / s;
        q.w = (m->up.z - m->at.y) * s;
        q.y = (m->right.y + m->up.x) * s;
        q.z = (m->at.x + m->right.z) * s;
    } else if (m->up.y > m->at.z) {
        float s = sqrtf(1.0f + m->up.y - m->right.x - m->at.z);
        q.y = s * 0.5f;
        s = 0.5f / s;
        q.w = (m->at.x - m->right.z) * s;
        q.x = (m->right.y + m->up.x) * s;
        q.z = (m->up.z + m->at.y) * s;
    } else {
        float s = sqrtf(1.0f + m->at.z - m->right.x - m->up.y);
        q.z = s * 0.5f;
        s = 0.5f / s;
        q.w = (m->right.y - m->up.x) * s;
        q.x = (m->at.x + m->right.z) * s;
        q.y = (m->up.z + m->at.y) * s;
    }
    return q;
}

static inline void
rw_link_init(RwLink *l)
{
    l->next = l;
    l->prev = l;
}

static inline void
rw_linklist_init(RwLinkList *l)
{
    rw_link_init(&l->link);
}

static inline int
rw_linklist_isempty(RwLinkList *l)
{
    return l->link.next == &l->link;
}

static inline void
rw_link_add(RwLink *l, RwLink *n)
{
    n->next = l->next;
    n->prev = l;
    l->next->prev = n;
    l->next = n;
}

static inline void
rw_linklist_add(RwLinkList *l, RwLink *n)
{
    rw_link_add(&l->link, n);
}

static inline void
rw_link_append(RwLink *l, RwLink *n)
{
    n->next = l;
    n->prev = l->prev;
    l->prev->next = n;
    l->prev = n;
}

static inline void
rw_linklist_append(RwLinkList *l, RwLink *n)
{
    rw_link_append(&l->link, n);
}

static inline void
rw_link_remove(RwLink *l)
{
    l->prev->next = l->next;
    l->next->prev = l->prev;
    l->next = l;
    l->prev = l;
}

#define RW_LINK_DATA(link, type, member) \
    ((type*)((uint8_t*)(link) - offsetof(type, member)))

#define RW_FORLIST(iter, list) \
    for ((iter) = (list).link.next; (iter) != &(list).link; (iter) = (iter)->next)

RwFrame  *rw_frame_create(void);
void      rw_frame_destroy(RwFrame *f);
void      rw_frame_add_child(RwFrame *parent, RwFrame *child);
void      rw_frame_remove_child(RwFrame *child);
RwMatrix *rw_frame_get_ltm(RwFrame *f);
void      rw_frame_set_matrix(RwFrame *f, const RwMatrix *m);
void      rw_frame_translate(RwFrame *f, RwV3d v, RwCombineOp op);
void      rw_frame_rotate(RwFrame *f, RwV3d axis, float angle, RwCombineOp op);
void      rw_frame_scale(RwFrame *f, RwV3d s, RwCombineOp op);
void      rw_frame_sync_dirty(void);

RwGeometry *rw_geometry_create(int num_verts, int num_tris, uint32_t flags);
void        rw_geometry_destroy(RwGeometry *geo);
void        rw_geometry_lock(RwGeometry *geo, uint32_t lock_flags);
void        rw_geometry_unlock(RwGeometry *geo);
int         rw_geometry_build_meshes(RwGeometry *geo);
void        rw_geometry_calc_bounding_sphere(RwGeometry *geo);

RwMaterial *rw_material_create(void);
void        rw_material_destroy(RwMaterial *mat);
void        rw_material_set_texture(RwMaterial *mat, RwTexture *tex);
void        rw_material_set_color(RwMaterial *mat, RwRGBA color);
void        rw_material_set_surface(RwMaterial *mat, float amb, float spec, float diff);

RwTexture  *rw_texture_create(RwRaster *raster);
void        rw_texture_destroy(RwTexture *tex);
void        rw_texture_set_filter(RwTexture *tex, int filter);
void        rw_texture_set_addressing(RwTexture *tex, int u, int v);

RwRaster   *rw_raster_create(int w, int h, int depth, uint32_t flags);
void        rw_raster_destroy(RwRaster *r);
uint8_t    *rw_raster_lock(RwRaster *r);
void        rw_raster_unlock(RwRaster *r);

RwImage    *rw_image_load(const char *filename);
void        rw_image_destroy(RwImage *img);
RwRaster   *rw_raster_from_image(RwImage *img);

RwTexDict  *rw_texdict_create(void);
void        rw_texdict_destroy(RwTexDict *td);
void        rw_texdict_add(RwTexDict *td, RwTexture *tex);
RwTexture  *rw_texdict_find(RwTexDict *td, const char *name);

RwAtomic *rw_atomic_create(void);
void      rw_atomic_destroy(RwAtomic *a);
void      rw_atomic_set_geometry(RwAtomic *a, RwGeometry *geo);
void      rw_atomic_set_frame(RwAtomic *a, RwFrame *f);
void      rw_atomic_set_pipeline(RwAtomic *a, RwObjPipeline *p);
void      rw_atomic_set_render_cb(RwAtomic *a, void (*cb)(RwAtomic *));
void      rw_atomic_render(RwAtomic *a);

RwClump  *rw_clump_create(void);
void      rw_clump_destroy(RwClump *c);
RwFrame  *rw_clump_get_frame(RwClump *c);
void      rw_clump_set_frame(RwClump *c, RwFrame *f);
void      rw_clump_add_atomic(RwClump *c, RwAtomic *a);
void      rw_clump_remove_atomic(RwClump *c, RwAtomic *a);
void      rw_clump_add_light(RwClump *c, RwLight *l);
void      rw_clump_render(RwClump *c);

RwWorld *rw_world_create(void);
void     rw_world_destroy(RwWorld *w);
void     rw_world_add_clump(RwWorld *w, RwClump *c);
void     rw_world_remove_clump(RwWorld *w, RwClump *c);
void     rw_world_add_light(RwWorld *w, RwLight *l);
void     rw_world_remove_light(RwWorld *w, RwLight *l);
void     rw_world_render(RwWorld *w);
void     rw_world_enumerate_lights(RwWorld *w, RwAtomic *a, RwWorldLights *out);

RwCamera *rw_camera_create(void);
void      rw_camera_destroy(RwCamera *c);
void      rw_camera_set_frame(RwCamera *c, RwFrame *f);
void      rw_camera_begin_update(RwCamera *c);
void      rw_camera_end_update(RwCamera *c);
void      rw_camera_clear(RwCamera *c, uint32_t flags);
void      rw_camera_set_fov(RwCamera *c, float fov);
void      rw_camera_set_view_window(RwCamera *c, float vw);
void      rw_camera_set_near_far(RwCamera *c, float near, float far);
int       rw_camera_frustum_test_sphere(RwCamera *c, const RwSphere *s);

RwLight *rw_light_create(RwLightType type);
void     rw_light_destroy(RwLight *l);
void     rw_light_set_color(RwLight *l, RwRGBAf color);
void     rw_light_set_radius(RwLight *l, float radius);
void     rw_light_set_frame(RwLight *l, RwFrame *f);

void rw_im2d_render_primitive(RwPrimitiveType type, RwIm2DVertex *verts, int num_verts);
void rw_im2d_render_indexed(RwPrimitiveType type, RwIm2DVertex *verts, int num_verts,
                             uint16_t *indices, int num_indices);

void rw_im3d_transform(RwIm3DVertex *verts, int num_verts, RwMatrix *world, uint32_t flags);
void rw_im3d_render_primitive(RwPrimitiveType type);
void rw_im3d_render_indexed(RwPrimitiveType type, uint16_t *indices, int num_indices);
void rw_im3d_end(void);

RwSkin  *rw_skin_create(int num_bones, int num_verts, int num_weights);
void     rw_skin_destroy(RwSkin *skin);
void     rw_skin_set_data(RwSkin *skin, uint8_t *indices, float *weights, float *inv_matrices);
void     rw_skin_set_pipeline(RwAtomic *a);

RwHAnimHier *rw_hanim_create(int num_nodes, RwHAnimNodeInfo *nodes);
void         rw_hanim_destroy(RwHAnimHier *h);
void         rw_hanim_attach(RwHAnimHier *h, RwFrame *root);
void         rw_hanim_update_matrices(RwHAnimHier *h);
void         rw_hanim_interpolate(RwHAnimHier *h, float time);

void *rw_malloc(size_t size);
void *rw_realloc(void *ptr, size_t size);
void  rw_free(void *ptr);

#endif
