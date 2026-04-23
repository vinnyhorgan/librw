# rw — Fantasy Console RenderWare API in C99

## 1. Goals & Constraints

| Goal | Target |
|---|---|
| Language | C99, no C++, no extensions |
| Graphics backend | OpenGL ES 2.0 only, zero extensions |
| External deps | `stb_image.h` (image loading only) |
| Windowing | None — caller provides GL context |
| Max line count | ~4,000 lines of C |
| API style | `rw_` prefixed snake_case |
| Reference | Port algorithms from librw, not code |

## 2. What Makes a GTA 3-Like Game Possible

These are the rendering capabilities GTA 3/VC/SA require that we provide:

- **Textured meshes** with per-vertex color and lighting
- **Frame hierarchy** for articulated objects (vehicle parts, character bones)
- **Per-vertex lighting** — ambient + directional + point lights
- **Distance fog** for atmosphere and draw distance
- **Alpha test** — cutout geometry (fences, vegetation, tree canopies)
- **Alpha blend** — transparent glass, particles, fade effects
- **Frustum culling** — skip offscreen objects
- **Skeletal animation** — up to 4 bone weights per vertex, 64 bones
- **Custom render callbacks** on atomics — game controls per-object render logic
- **Mesh splitting by material** — batched draw calls per material/texture
- **Render state control** — z-test, z-write, cull mode, blend factors
- **im2d** — HUD, menus, health bars, minimap
- **im3d** — debug lines, shadow blobs, simple effects
- **Multiple clumps in a world** — buildings, vehicles, pedestrians all separate

## 3. What We Do NOT Include (Game's Responsibility)

Collision detection, physics, AI, audio, input, asset streaming, window management, particle systems, shadow maps, post-processing. These are built *on top of* this library by the game.

## 4. Architecture Overview

```
┌─────────────────────────────────────────────────┐
│                  Game Code                       │
│  (provides GL context, drives main loop)         │
└──────────┬──────────────────────────────────────┘
           │ calls rw_* API
┌──────────▼──────────────────────────────────────┐
│              rw.h (public API)                   │
├─────────────────────────────────────────────────┤
│  rw_engine.c    Engine lifecycle, render state   │
│  rw_frame.c     Transform hierarchy              │
│  rw_geometry.c  Mesh data + mesh building        │
│  rw_material.c  Material + texture + texdict     │
│  rw_raster.c    Image loading + GPU texture mgmt │
│  rw_scene.c     Atomic, Clump, World, Camera,    │
│                 Light                            │
│  rw_pipeline.c  Instance (CPU→GPU) + render      │
│  rw_render.c    im2d + im3d immediate mode       │
│  rw_skin.c      Skin + HAnim + animation interp  │
├─────────────────────────────────────────────────┤
│              rw_gl.c                             │
│  OpenGL ES 2.0 backend (device, state cache,     │
│  shaders, texture upload, draw calls)            │
└─────────────────────────────────────────────────┘
           │ calls
┌──────────▼──────────────────────────────────────┐
│           OpenGL ES 2.0                          │
└─────────────────────────────────────────────────┘
```

## 5. File Structure & Line Budgets

```
rw/
├── rw.h              ~500   types, API, static inline math
├── rw_engine.c       ~300   engine lifecycle, state, memory
├── rw_frame.c        ~180   frame hierarchy, dirty propagation
├── rw_geometry.c     ~250   geometry, mesh building
├── rw_material.c     ~150   material + texture + texdict
├── rw_raster.c       ~200   raster + image (stb_image bridge)
├── rw_scene.c        ~350   atomic, clump, world, camera, light
├── rw_pipeline.c     ~300   default pipeline: instance + render
├── rw_render.c       ~250   im2d + im3d
├── rw_skin.c         ~300   skin + hanim + keyframe interpolation
├── rw_gl.c           ~750   GL backend: device, state cache,
│                            shaders (string literals), texture
│                            upload, instancing, draw calls
├── vendor/
│   ├── glfw/               window/input (for tests)
│   ├── glad/               GL loader
│   └── stb/
│       └── stb_image.h     image loading (PNG, JPG, BMP, TGA)
├── Makefile
├── tests/
│   ├── test_math.c          unit tests for math functions
│   ├── test_clear.c         init engine, clear screen
│   ├── test_triangle.c      render a textured triangle
│   ├── test_scene.c         full scene: camera + lights + world
│   ├── test_im2d.c          2D overlay rendering
│   ├── test_skin.c          skeletal animation
│   └── test_gta.c           GTA-like mini-demo
└── tools/
    └── gltf2rw.c            glTF → .rwm/.rwa converter (separate)
```

**Total estimate: ~3,530 lines of C in library code, ~470 lines of GLSL as string literals embedded in `rw_gl.c`.**

## 6. Complete Type Definitions (for `rw.h`)

```c
/* ---- Primitive types ---- */
typedef int32_t  rw_int32;
typedef uint32_t rw_uint32;
typedef int16_t  rw_int16;
typedef uint16_t rw_uint16;
typedef int8_t   rw_int8;
typedef uint8_t  rw_uint8;
typedef float    rw_float32;
typedef int      rw_bool32;

/* ---- Math types ---- */
typedef struct { float x, y; }          RwV2d;
typedef struct { float x, y, z; }       RwV3d;
typedef struct { float x, y, z, w; }    RwV4d;
typedef struct { float x, y, z, w; }    RwQuat;
typedef struct { float r, g, b, a; }    RwRGBAf;
typedef struct { uint8_t r, g, b, a; }  RwRGBA;
typedef struct { float u, v; }          RwTexCoords;

/* Matrix: 4x3 stored as 4 rows of {V3d, uint32} for alignment.
   Layout matches librw: right, up, at, pos vectors. */
typedef struct {
    RwV3d right;   uint32_t _pad0;
    RwV3d up;      uint32_t _pad1;
    RwV3d at;      uint32_t _pad2;
    RwV3d pos;     uint32_t _pad3;
} RwMatrix;

/* RawMatrix: full 4x4 for GPU upload */
typedef struct {
    RwV3d right;  float rightw;
    RwV3d up;     float upw;
    RwV3d at;     float atw;
    RwV3d pos;    float posw;
} RwRawMatrix;

/* ---- Geometry types ---- */
typedef struct { float x, y, z, w; uint8_t r, g, b, a; float u, v; } RwIm2DVertex;
typedef struct { RwV3d pos; RwV3d normal; uint8_t r, g, b, a; float u, v; } RwIm3DVertex;

typedef struct { RwV3d sup, inf; }               RwBBox;
typedef struct { RwV3d center; float radius; }    RwSphere;
typedef struct { int32 x, y, w, h; }             RwRect;

/* ---- Linked list (intrusive, from librw) ---- */
typedef struct RwLink { struct RwLink *next, *prev; } RwLink;
typedef struct { RwLink link; } RwLinkList;

/* ---- Forward declarations ---- */
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

/* ---- Enums ---- */
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

/* im3d flags */
enum {
    RW_IM3D_VERTEXXYZ  = 0x01,
    RW_IM3D_VERTEXUV   = 0x02,
    RW_IM3D_VERTEXRGBA = 0x04,
    RW_IM3D_LIGHTING   = 0x08,
};

/* Frame dirty flags */
enum {
    RW_FRAME_SUBTREESYNC   = 0x01,
    RW_FRAME_HIERARCHYSYNC = 0x02,
};

/* ---- Core structs ---- */

struct RwFrame {
    uint8_t type, subtype, flags, priv_flags;
    RwMatrix matrix;
    RwMatrix ltm;
    RwFrame *parent;
    RwFrame *child;     /* first child */
    RwFrame *next;      /* next sibling */
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
    /* meshes array follows this struct in memory */
    /* access via: ((RwMesh*)(header + 1)) */
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
    RwMorphTarget morph_target;   /* single morph target */
    RwMaterial **materials;
    int num_materials;
    int mat_space;
    RwMeshHeader *mesh_header;
    void *gl_data;                /* GL instance data (VBO/IBO IDs) */
    int ref_count;
    RwSkin *skin;                 /* NULL if not skinned */
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
    uint8_t *pixels;              /* CPU-side during lock */
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
    /* frustum planes: normal, distance, closest-axis hints */
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
    float minus_cos_angle;        /* for spotlights */
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
    int num_weights;              /* per vertex, typically 4 */
    float *inverse_matrices;      /* 64 * 16 floats (4x4 per bone) */
    uint8_t *bone_indices;        /* num_weights * num_vertices */
    float *weights;               /* num_weights * num_vertices */
};

typedef struct {
    int32_t id;
    int32_t index;
    int32_t flags;                /* PUSH/POP for hierarchy traversal */
    RwFrame *frame;
} RwHAnimNodeInfo;

typedef struct {
    RwQuat q;                     /* rotation */
    RwV3d t;                      /* translation */
    float time;
} RwHAnimKeyFrame;

struct RwHAnimHier {
    int num_nodes;
    RwHAnimNodeInfo *nodes;
    RwMatrix *matrices;           /* computed global bone matrices */
    /* Current interpolated keyframes */
    RwHAnimKeyFrame *keyframes;
    int num_keyframes;
    float current_time;
    int num_frames;               /* total frames in animation */
    float frame_rate;
    RwHAnimKeyFrame *anim_data;   /* all frames packed: num_nodes * num_frames */
};

/* ---- Engine state (global singleton) ---- */
typedef struct {
    struct RwCamera *current_camera;
    struct RwWorld  *current_world;
    RwLinkList frame_dirty_list;
    uint32_t render_states[RW_STATE_NUM_STATES];
    /* Memory functions (overridable) */
    void *(*malloc)(size_t);
    void *(*realloc)(void*, size_t);
    void  (*free)(void*);
} RwEngine;

/* The global engine instance */
extern RwEngine rw_engine;

/* ---- Memory functions ---- */
typedef struct {
    void *(*malloc)(size_t);
    void *(*realloc)(void*, size_t);
    void  (*free)(void*);
} RwMemoryFuncs;
```

## 7. Complete Public API (`rw.h`)

### Engine

```c
int  rw_engine_init(RwMemoryFuncs *memfuncs); /* NULL for default malloc */
int  rw_engine_open(void);                     /* caller has GL context ready */
int  rw_engine_start(void);                    /* compile shaders, init state */
void rw_engine_stop(void);
void rw_engine_close(void);
void rw_engine_term(void);

void     rw_set_render_state(int state, uint32_t value);
uint32_t rw_get_render_state(int state);
```

### Math (all `static inline` in header)

```c
RwV3d  rw_v3d_add(RwV3d a, RwV3d b);
RwV3d  rw_v3d_sub(RwV3d a, RwV3d b);
RwV3d  rw_v3d_scale(RwV3d v, float s);
float  rw_v3d_dot(RwV3d a, RwV3d b);
RwV3d  rw_v3d_cross(RwV3d a, RwV3d b);
float  rw_v3d_length(RwV3d v);
RwV3d  rw_v3d_normalize(RwV3d v);
void   rw_matrix_set_identity(RwMatrix *m);
void   rw_matrix_translate(RwMatrix *m, RwV3d v, RwCombineOp op);
void   rw_matrix_rotate(RwMatrix *m, RwV3d axis, float angle, RwCombineOp op);
void   rw_matrix_scale(RwMatrix *m, RwV3d s, RwCombineOp op);
void   rw_matrix_multiply(RwMatrix *out, const RwMatrix *a, const RwMatrix *b);
int    rw_matrix_invert(RwMatrix *out, const RwMatrix *in);
void   rw_matrix_to_raw(RwRawMatrix *out, const RwMatrix *in);
RwQuat rw_quat_slerp(RwQuat a, RwQuat b, float t);
void   rw_quat_to_matrix(RwMatrix *out, RwQuat q);
RwQuat rw_quat_from_matrix(const RwMatrix *m);
```

### Frame

```c
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
```

### Geometry

```c
RwGeometry *rw_geometry_create(int num_verts, int num_tris, uint32_t flags);
void        rw_geometry_destroy(RwGeometry *geo);
void        rw_geometry_lock(RwGeometry *geo, uint32_t lock_flags);
void        rw_geometry_unlock(RwGeometry *geo);
int         rw_geometry_build_meshes(RwGeometry *geo);
void        rw_geometry_calc_bounding_sphere(RwGeometry *geo);
```

### Material / Texture / Raster / Image

```c
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
```

### Atomic / Clump

```c
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
```

### World / Camera / Light

```c
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
```

### im2d / im3d

```c
void rw_im2d_render_primitive(RwPrimitiveType type, RwIm2DVertex *verts, int num_verts);
void rw_im2d_render_indexed(RwPrimitiveType type, RwIm2DVertex *verts, int num_verts,
                             uint16_t *indices, int num_indices);

void rw_im3d_transform(RwIm3DVertex *verts, int num_verts, RwMatrix *world, uint32_t flags);
void rw_im3d_render_primitive(RwPrimitiveType type);
void rw_im3d_render_indexed(RwPrimitiveType type, uint16_t *indices, int num_indices);
void rw_im3d_end(void);
```

### Skin / Animation

```c
RwSkin  *rw_skin_create(int num_bones, int num_verts, int num_weights);
void     rw_skin_destroy(RwSkin *skin);
void     rw_skin_set_data(RwSkin *skin, uint8_t *indices, float *weights, float *inv_matrices);
void     rw_skin_set_pipeline(RwAtomic *a);

RwHAnimHier *rw_hanim_create(int num_nodes, RwHAnimNodeInfo *nodes);
void         rw_hanim_destroy(RwHAnimHier *h);
void         rw_hanim_attach(RwHAnimHier *h, RwFrame *root);
void         rw_hanim_update_matrices(RwHAnimHier *h);
void         rw_hanim_interpolate(RwHAnimHier *h, float time);
```

## 8. OpenGL ES 2.0 Backend Design (`rw_gl.c`)

**Reference**: Derived from `src/gl/gl3device.cpp`, `src/gl/gl3pipe.cpp`, `src/gl/gl3render.cpp`, `src/gl/gl3shader.cpp`, `src/gl/gl3raster.cpp`, `src/gl/gl3immed.cpp`, `src/gl/gl3skin.cpp`.

**No VAOs, no UBOs, no extensions.** All state managed through a cache.

### State Cache

```c
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
```

The cache wraps every `glEnable`/`glBlendFunc`/`glDepthMask`/`glBindTexture`/`glUseProgram` call. Before issuing a GL call, check if the new value differs from the cached value. Skip the GL call if unchanged.

### Vertex Layout — Default Geometry (interleaved, single VBO)

```
pos(3f) | normal(3f) | texcoord(2f) | color(4ub) = 36 bytes per vertex
```

### Vertex Layout — Skinned Geometry

```
pos(3f) | normal(3f) | texcoord(2f) | color(4ub) | weights(4f) | indices(4ub) = 56 bytes
```

### Instancing (port of `gl3pipe.cpp:defaultInstanceCB`)

On `rw_geometry_unlock` + `rw_geometry_build_meshes`:

1. Compute stride and attribute offsets based on geometry flags
2. Allocate interleaved vertex buffer, fill from geometry vertex/normal/texcoord/color arrays
3. If skinned: append weights and bone indices from `RwSkin`
4. Reindex triangles into per-mesh index ranges (from `RwMeshHeader`)
5. `glGenBuffers` → `glBufferData(GL_ARRAY_BUFFER)` for VBO
6. `glGenBuffers` → `glBufferData(GL_ELEMENT_ARRAY_BUFFER)` for IBO
7. Store VBO/IBO IDs and attribute descriptors in `geometry->gl_data`

### Rendering (port of `gl3render.cpp:defaultRenderCB`)

For each atomic:

1. Upload `u_world` from `frame->ltm` (expanded to 4x4 via `rw_matrix_to_raw`)
2. Enumerate lights via `rw_world_enumerate_lights`, upload light uniform arrays
3. Bind VBO + IBO via `glBindBuffer`
4. Set up `glVertexAttribPointer` for each active attribute
5. For each mesh in `mesh_header`:
   - Set material color uniform `u_mat_color`
   - Set surface properties uniform `u_surf_props`
   - Bind material texture via `glBindTexture` (with state cache check)
   - Select shader permutation based on light count and alpha test state
   - `glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_SHORT, offset)`
6. Disable vertex attrib arrays

### Texture Upload (port of `gl3raster.cpp`)

- `rw_raster_from_image` → `glGenTextures` + `glTexImage2D(GL_RGBA, ...)` + `glTexParameteri`
- Sampler state (filter/wrap) updated lazily when texture parameters change
- No compressed texture formats — RGBA8 only
- No auto-mipmapping in v1 (can add `glGenerateMipmap` later)

### Shader Management

Shaders compiled from string literals at `rw_engine_start`. Permutations built by prepending `#define` strings before the common header + main source.

## 9. Shader Programs (GLSL ES 1.00)

All shaders use `#version 100`, `precision mediump float` for varyings, `precision highp float` for positions.

### Permutations Compiled at Startup

| Name | Defines | Used When |
|---|---|---|
| `default` | — | No lights on geometry |
| `default_dir` | `DIRECTIONALS` | Directional lights present |
| `default_dir_point` | `DIRECTIONALS`, `POINTLIGHTS` | Dir + point lights |
| `skin_dir_point` | `DIRECTIONALS`, `POINTLIGHTS`, `SKIN` | Skinned mesh with lights |
| `im2d` | — | 2D immediate mode |
| `im3d` | — | 3D immediate mode |
| `im3d_lit` | `IM3D`, `LIGHTING` | 3D immediate mode with lighting |

### Uniforms (same names as librw for clarity)

| Uniform | Type | Purpose |
|---|---|---|
| `u_proj` | `mat4` | Projection matrix |
| `u_view` | `mat4` | View matrix |
| `u_world` | `mat4` | World (model) matrix |
| `u_amb_light` | `vec4` | Ambient light color |
| `u_light_params[8]` | `vec4` | Light type + radius per light |
| `u_light_position[8]` | `vec4` | Light world positions |
| `u_light_direction[8]` | `vec4` | Light world directions |
| `u_light_color[8]` | `vec4` | Light colors |
| `u_mat_color` | `vec4` | Material color |
| `u_surf_props` | `vec4` | ambient/specular/diffuse/extra |
| `u_alpha_ref` | `vec4` | Alpha test threshold |
| `u_fog_data` | `vec4` | start/end/range/disabled |
| `u_fog_color` | `vec4` | Fog color |
| `u_bone_matrices[64]` | `mat4` | Skin: bone matrices |
| `u_xform` | `mat4` | im2d: pixel-to-NDC transform |

### Attributes

| Location | Name | Format |
|---|---|---|
| 0 | `in_pos` | `vec3` (float) |
| 1 | `in_normal` | `vec3` (float) |
| 2 | `in_tex0` | `vec2` (float) |
| 3 | `in_color` | `vec4` (normalized ubyte) |
| 4 | `in_weights` | `vec4` (float) |
| 5 | `in_indices` | `vec4` (normalized ubyte) |

### Vertex Shader Core (from librw `default.vert`)

```glsl
vec4 Vertex = u_world * vec4(in_pos, 1.0);
gl_Position = u_proj * u_view * Vertex;
vec3 Normal = mat3(u_world) * in_normal;
v_tex0 = in_tex0;
v_color = in_color;
v_color.rgb += u_amb_light.rgb * u_surf_props.x;
// Per-vertex lighting loop
v_color.rgb += DoDynamicLight(Vertex.xyz, Normal) * u_surf_props.z;
v_color = clamp(v_color, 0.0, 1.0) * u_mat_color;
v_fog = clamp((gl_Position.w - u_fog_data.y) * u_fog_data.z, 0.0, 1.0);
```

### Fragment Shader Core (from librw `simple.frag`)

```glsl
vec4 color = v_color * texture2D(tex0, vec2(v_tex0.x, 1.0 - v_tex0.y));
color.rgb = mix(u_fog_color.rgb, color.rgb, v_fog);
if(u_alpha_ref.x > 0.0 && color.a < u_alpha_ref.x) discard;
gl_FragColor = color;
```

### Skinning Core (from librw `skin.vert`)

```glsl
vec3 SkinVertex = vec3(0.0);
vec3 SkinNormal = vec3(0.0);
for(int i = 0; i < 4; i++){
    int idx = int(in_indices[i] * 255.0 + 0.5);
    SkinVertex += (u_bone_matrices[idx] * vec4(in_pos, 1.0)).xyz * in_weights[i];
    SkinNormal += (mat3(u_bone_matrices[idx]) * in_normal) * in_weights[i];
}
// Then same lighting/fog as default using SkinVertex/SkinNormal
```

### Dynamic Lighting Function (from librw `header.vert`)

```glsl
vec3 DoDynamicLight(vec3 V, vec3 N) {
    vec3 color = vec3(0.0);
    for(int i = 0; i < 8; i++) {
        if(u_light_params[i].x == 0.0) break;
        #ifdef DIRECTIONALS
        if(u_light_params[i].x == 1.0) {
            float d = max(0.0, dot(N, -u_light_direction[i].xyz));
            color += d * u_light_color[i].rgb;
        }
        #endif
        #ifdef POINTLIGHTS
        if(u_light_params[i].x == 2.0) {
            vec3 L = u_light_position[i].xyz - V;
            float dist = length(L);
            float atten = max(0.0, 1.0 - dist / u_light_params[i].y);
            float d = max(0.0, dot(N, normalize(L)));
            color += d * u_light_color[i].rgb * atten;
        }
        #endif
    }
    return color;
}
```

## 10. Animation System

**Reference**: Derived from `src/hanim.cpp` (hierarchy update), `src/skin.cpp` (skinning), `src/gl/gl3skin.cpp` (GPU upload).

### Pipeline

1. Game calls `rw_hanim_interpolate(h, time)` — interpolates between keyframes using quaternion slerp + vector lerp
2. Game calls `rw_hanim_update_matrices(h)` — traverses hierarchy, multiplies parent × child, producing global bone matrices
3. Before skin draw: upload `h->matrices[i] × skin->inverse_matrices[i]` as `u_bone_matrices`
4. Vertex shader does LBS (linear blend skinning) with 4 weights

### Keyframe Interpolation (port of `hanim.cpp` interpolation logic)

```c
void rw_hanim_interpolate(RwHAnimHier *h, float time) {
    // Find keyframe pair surrounding 'time'
    // For each node: slerp rotation quaternion, lerp translation
    // Store result in h->keyframes
}
```

### Hierarchy Update (port of `hanim.cpp:updateMatrices`)

```c
void rw_hanim_update_matrices(RwHAnimHier *h) {
    // Stack-based traversal using PUSH/POP flags on each node
    // For each node:
    //   local_matrix = quat_to_matrix(q) + translate(t)
    //   global_matrix = parent_global × local_matrix
    //   Store in h->matrices[node_index]
}
```

### Skin Matrix Upload (port of `gl3skin.cpp:uploadSkinMatrices`)

Before rendering a skinned atomic:

```c
for(int i = 0; i < skin->num_bones; i++) {
    // M_skin = M_global_bone × M_inverse_bind
    rw_matrix_multiply(&bone_matrix, &hier->matrices[i], &skin->inverse_matrices[i]);
    // Upload to u_bone_matrices[i]
}
```

### Asset Authoring Pipeline

- **Blender** → Export glTF → `gltf2rw` converter tool → `.rwm` (model) + `.rwa` (animation)
- `.rwm` format: vertex data, indices, materials, skin weights/indices/inverse bind matrices
- `.rwa` format: per-frame per-bone quaternion + translation keyframes
- Converter tool (`tools/gltf2rw.c`) is separate from runtime library

## 11. Testing Strategy

**Test framework**: Simple `assert()`-based tests in `tests/`. Each test is a standalone program linked against the library.

**Test dependency**: GLFW + glad (for GL context creation in tests only). The library itself has no windowing dependency.

### Automated Verification Pattern

Every test follows this pattern:

```c
assert(glGetError() == GL_NO_ERROR);  /* after each significant GL operation */
uint8_t pixel[4];
glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
assert(pixel[0] > 200);  /* verify expected color */
```

### Test Suite

| Test | What It Validates | librw Reference |
|---|---|---|
| `test_math` | Vector/matrix operations, invert, quaternion slerp. Compare known results. | `src/base.cpp` |
| `test_clear` | `rw_engine_init` → `open` → `start` → clear screen → read pixel → verify color → `stop` → `close` → `term`. No GL errors at each step. | `src/engine.cpp` |
| `test_frame` | Build frame hierarchy, set transforms, call `rw_frame_sync_dirty`, verify LTM computation matches manual multiplication. | `src/frame.cpp` |
| `test_triangle` | Create geometry (3 verts, 1 tri), build mesh, instance, render with camera. Read back pixel, verify textured triangle visible. | `src/geometry.cpp` + `src/gl/gl3pipe.cpp` |
| `test_scene` | Full scene: world + camera + directional light + 2 textured clumps. Verify frustum culling hides offscreen object. Verify lighting shades correctly. | `src/world.cpp` + `src/camera.cpp` |
| `test_im2d` | Render colored quads and lines in screen space after 3D scene. Verify overlay appears on top. | `src/render.cpp` + `src/gl/gl3immed.cpp` |
| `test_skin` | Load skinned model, interpolate animation, render. Verify deformed mesh renders without GL errors. Compare vertex positions to known CPU-skinning result. | `src/skin.cpp` + `src/hanim.cpp` + `src/gl/gl3skin.cpp` |
| `test_gta` | Mini GTA-like demo: ground plane, building boxes, animated character, camera follow, fog, HUD overlay. Visual confirmation only. | All modules |

### Running Tests

```bash
make test        # builds and runs all tests (headless via offscreen rendering)
make test_vis    # runs tests with visible window (visual check)
```

## 12. Build System (Makefile)

```makefile
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -Ivendor
LIB_SRCS = rw_engine.c rw_frame.c rw_geometry.c rw_material.c rw_raster.c \
           rw_scene.c rw_pipeline.c rw_render.c rw_skin.c rw_gl.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

librw.a: $(LIB_OBJS)
	ar rcs $@ $^

%.o: %.c rw.h
	$(CC) $(CFLAGS) -c $< -o $@

# Tests
TEST_LIBS = -L. -lrw -lGLESv2 -lglfw -lm -ldl

test_%: tests/test_%.c librw.a
	$(CC) $(CFLAGS) -o $@ $< $(TEST_LIBS)

TESTS = test_math test_clear test_frame test_triangle test_scene test_im2d test_skin

test: $(TESTS)
	@for t in $(TESTS); do echo "=== $$t ===" && ./$$t && echo "PASS" || echo "FAIL"; done

clean:
	rm -f *.o librw.a $(TESTS)
```

## 13. Implementation Order with Exact librw Source Mapping

| Phase | Our Files | Lines | librw Sources to Study | Milestone |
|---|---|---|---|---|
| **1** | `rw.h`, `rw_engine.c` | ~800 | `rwbase.h`, `rwengine.h`, `engine.cpp`, `base.cpp` | Engine inits, `test_math` + `test_clear` pass |
| **2** | `rw_frame.c` | ~180 | `frame.cpp` | `test_frame` passes: hierarchy, dirty sync, LTM |
| **3** | `rw_gl.c` (partial), `rw_raster.c`, `rw_material.c` | ~600 | `gl3device.cpp`, `gl3shader.cpp`, `gl3raster.cpp`, `texture.cpp`, `raster.cpp` | Texture loads and binds, no rendering yet |
| **4** | `rw_geometry.c`, `rw_pipeline.c`, `rw_gl.c` (instance+render) | ~550 | `geometry.cpp`, `gl3pipe.cpp`, `gl3render.cpp`, `rwgl3.h` | `test_triangle` passes: geometry instances and renders |
| **5** | `rw_scene.c` (atomic, clump, camera, light, world) | ~350 | `atomic.cpp`, `clump.cpp`, `camera.cpp`, `light.cpp`, `world.cpp` | `test_scene` passes: full scene renders |
| **6** | `rw_render.c`, `rw_gl.c` (im2d/im3d paths) | ~250 | `render.cpp`, `gl3immed.cpp` | `test_im2d` passes: 2D overlay renders |
| **7** | `rw_skin.c`, `rw_gl.c` (skin shader path) | ~300 | `skin.cpp`, `hanim.cpp`, `gl3skin.cpp` | `test_skin` passes: animated character renders |
| **8** | `test_gta.c`, polish | ~200 | All | GTA-like mini demo runs |

## 14. Key Algorithms Ported from librw

| Algorithm | librw Source | What It Does |
|---|---|---|
| Frame dirty propagation | `frame.cpp:updateObjects`, `syncHierarchyLTM` | Marks subtree dirty on change, recursively recomputes LTM only for dirty branches |
| Mesh building from triangles | `geometry.cpp:buildMeshes` | Groups triangles by material ID, builds per-mesh index arrays |
| Interleaved vertex instancing | `gl3pipe.cpp:defaultInstanceCB` | Packs positions/normals/UVs/colors into single VBO with stride |
| Per-vertex lighting | `gl3render.cpp:lightingCB` + `shaders/header.vert` | Enumerates world lights near atomic, uploads as uniform arrays |
| Camera frustum extraction | `camera.cpp:cameraSync` | Extracts 6 planes from view×projection matrix for sphere testing |
| World light enumeration | `world.cpp:enumerateLights` | Finds ambient + directional + local lights affecting an atomic's bounding sphere |
| HAnim hierarchy traversal | `hanim.cpp:updateMatrices` | Stack-based PUSH/POP traversal, accumulates global bone matrices |
| Skin matrix upload | `gl3skin.cpp:uploadSkinMatrices` | Combines global bone matrices with inverse bind pose, uploads to GPU |
| GL state cache | `gl3device.cpp:flushGlRenderState` | Tracks bound textures, blend state, depth state to skip redundant GL calls |
| Intrusive linked list | `rwbase.h:LLLink/LinkList` | Circular doubly-linked list embedded in structs (no separate allocation) |

## 15. Line Budget Summary

| File | Lines | Port of |
|---|---|---|
| `rw.h` | ~500 | `rwbase.h` + `rwobjects.h` + `rwengine.h` (types only) |
| `rw_engine.c` | ~300 | `engine.cpp` + `base.cpp` (lifecycle + state) |
| `rw_frame.c` | ~180 | `frame.cpp` |
| `rw_geometry.c` | ~250 | `geometry.cpp` (mesh building) |
| `rw_material.c` | ~150 | `texture.cpp` + `material.cpp` |
| `rw_raster.c` | ~200 | `raster.cpp` + `image.cpp` (stb bridge) |
| `rw_scene.c` | ~350 | `atomic.cpp` + `clump.cpp` + `camera.cpp` + `light.cpp` + `world.cpp` |
| `rw_pipeline.c` | ~300 | `pipeline.cpp` + `gl3pipe.cpp` (instance) + `gl3render.cpp` (render) |
| `rw_render.c` | ~250 | `render.cpp` + `gl3immed.cpp` |
| `rw_skin.c` | ~300 | `skin.cpp` + `hanim.cpp` + `gl3skin.cpp` |
| `rw_gl.c` | ~750 | `gl3device.cpp` + `gl3shader.cpp` + `gl3raster.cpp` + shaders |
| **Total** | **~3,530** | |

## 16. What We Cut from librw

| Cut | Why |
|---|---|
| D3D8/D3D9/Xbox backends | Target is GLES 2.0 only |
| PS2 backend + GS emulation | Hardware-specific, irrelevant |
| Null backend | Fantasy console always has a GPU |
| DFF/TXD binary format parsing | Custom simpler formats instead |
| Plugin offset system | Replace with direct struct composition |
| MatFX plugin (env/bump/dual) | Too niche for v1, add later if needed |
| Color quantization (octree) | Not needed without PS2 textures |
| Charset/VGA font | Separate concern, implement outside |
| UserData plugin | YAGNI |
| Multiple texcoord sets | Keep 1 UV set |
| Morph targets > 1 | Keep single morph target (base pose) |
| UBO support | GLES 2.0 doesn't have UBOs |
| VAO requirement | GLES 2.0 doesn't have VAOs core |
| DXT/BC compressed textures | Not universally available on GLES 2.0 |
| Video mode enumeration | Fantasy console has fixed specs |
| Tristrip support | Trilist only, simpler |
| UV animation | YAGNI for v1 |
| Error system | Return codes + NULL, no error stack |
