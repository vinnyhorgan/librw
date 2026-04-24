# rw — Implementation Progress

## Status: All core phases complete — im3d focused test added, all tests pass

## Phase Overview

| Phase | Files | Status | Notes |
|---|---|---|---|
| 1. Foundation | `rw.h`, `rw_engine.c` | Done | Types, math, engine lifecycle — strict C99 syntax check passes |
| 2. Frame hierarchy | `rw_frame.c` | Done | Transform tree, dirty propagation — math/frame tests pass |
| 3. GL backend core | `rw_gl.c`, `rw_gl_internal.h`, `rw_raster.c`, `rw_material.c` | Done | State cache, 8 shader permutations, texture upload/mipmaps, lighting, skin matrices |
| 4. Geometry + Pipeline | `rw_geometry.c`, `rw_pipeline.c` | Done | CPU geometry + GPU instance/render pipeline with interleaved VBO/IBO |
| 5. Scene graph | `rw_scene.c` | Done | CPU atomic/clump/world/light/camera, frustum planes, glClear wired |
| 6. Immediate mode | `rw_render.c` | Done | im2d + im3d with dynamic VBOs |
| 7. Animation | `rw_skin.c` | Done | CPU skin + HAnim, skin pipeline wired |
| 8. Polish | `test_render.c`, `test_im2d.c`, `test_im3d.c`, `test_gta.c`, Makefile | In progress | GL render tests, im3d coverage, and GTA-like integration demo added |

**Total: ~4,253 lines of C in library code (target ~4,000).**

## Phase 1: Foundation

### rw.h
- [x] Primitive types (int8–uint64, float32, bool32)
- [x] Math types (V2d, V3d, V4d, Quat, RGBA, RGBAf, TexCoords)
- [x] RwMatrix (4x3 aligned layout)
- [x] RwRawMatrix (4x4 for GPU)
- [x] Geometry vertex types (Im2DVertex, Im3DVertex)
- [x] BBox, Sphere, Rect
- [x] Intrusive linked list (RwLink, RwLinkList)
- [x] Forward declarations
- [x] All enums (LightType, CombineOp, PrimitiveType, Geo flags, Lock flags, RenderState, etc.)
- [x] Core structs (Frame, Geometry, Material, Raster, Image, Texture, TexDict, Atomic, Clump, Camera, Light, World, Pipeline, Skin, HAnimHier)
- [x] Engine global struct
- [x] All API function declarations
- [x] Static inline math (v3d ops, RenderWare-layout matrix ops, quaternion ops)

### rw_engine.c
- [x] rw_engine_init (memory setup, validates custom allocator table)
- [x] rw_engine_open (calls rw_gl_open)
- [x] rw_engine_start (calls rw_gl_start, compiles shaders)
- [x] rw_engine_stop (calls rw_gl_stop)
- [x] rw_engine_close (calls rw_gl_close)
- [x] rw_engine_term
- [x] rw_set_render_state / rw_get_render_state
- [x] Linked list helpers (init, add, remove)
- [x] Memory wrappers (rw_malloc, rw_free, etc.)

### Tests
- [x] test_math.c — foundation matrix/quaternion regression tests
- [ ] test_clear.c — engine lifecycle + screen clear

## Phase 2: Frame Hierarchy

### rw_frame.c
- [x] rw_frame_create
- [x] rw_frame_destroy
- [x] rw_frame_add_child
- [x] rw_frame_remove_child
- [x] rw_frame_get_ltm (dirty propagation)
- [x] rw_frame_set_matrix
- [x] rw_frame_translate
- [x] rw_frame_rotate
- [x] rw_frame_scale
- [x] rw_frame_sync_dirty (hierarchy LTM update)
- [x] updateObjects (mark dirty, add to dirty list)
- [x] syncHierarchyLTM (recursive LTM computation)

### Tests
- [x] test_frame.c — hierarchy creation, LTM computation

## Phase 3: GL Backend Core + Textures

### rw_gl.c
- [x] GL state cache struct and flush functions (RwGlState: blend, depth, cull, texture, program)
- [x] Shader compilation from string literals (compile/link helpers with error logging)
- [x] Shader permutation management (8 permutations with #define prepending)
- [x] Shader source: default.vert, default.frag (as string literals)
- [x] Shader source: im2d.vert, im2d.frag, im3d.vert, im3d.frag, im3d_lit.vert
- [x] rw_gl_open (device init, state cache reset, default GL state)
- [x] rw_gl_start (compile all shaders, cache uniform locations)
- [x] rw_gl_stop / rw_gl_close (delete programs, reset state)
- [x] Camera matrix upload (proj + view uploaded to all programs)
- [x] Lighting upload (ambient, directional, point light uniform arrays)
- [x] Texture upload (glGenTextures + glTexImage2D for RGBA rasters)
- [x] Texture sampler state (filter/wrap mode mapping)
- [x] Auto-mipmap generation for mipmapped filters on valid power-of-two rasters, with non-mip fallback for invalid/NPOT rasters
- [x] Skin matrix upload (hierarchy × inverse_bind, transposed for GL)
- [x] Shader selection logic (based on light counts + skin flag)

### rw_raster.c
- [x] rw_raster_create
- [x] rw_raster_destroy
- [x] rw_raster_lock / rw_raster_unlock (CPU pixel storage only)
- [x] rw_image_load (stb_image bridge)
- [x] rw_image_destroy
- [x] rw_raster_from_image (CPU copy only; GL upload still pending)

### rw_material.c
- [x] rw_material_create / destroy
- [x] rw_material_set_texture / color / surface
- [x] rw_texture_create / destroy
- [x] rw_texture_set_filter / addressing
- [x] rw_texdict_create / destroy / add / find
- [x] tests/test_material.c — material defaults/refcounts, texdict lookup/ownership, image load, raster lock/image copy

## Phase 4: Geometry + Pipeline

### rw_geometry.c
- [x] rw_geometry_create
- [x] rw_geometry_destroy
- [x] rw_geometry_lock / unlock
- [x] rw_geometry_build_meshes (group triangles by material)
- [x] rw_geometry_calc_bounding_sphere
- [x] allocateData (vertex array allocation)
- [x] allocateMeshes (mesh header + mesh array)

### rw_pipeline.c
- [x] Default pipeline: instance callback (interleaved VBO packing)
- [x] Default pipeline: render callback (mesh loop with material/texture/light state)
- [x] Interleaved vertex buffer packing (pos3f + normal3f + tex2f + color4ub [+ weights4f + indices4ub])
- [x] GL instance: VBO/IBO creation and upload
- [x] GL render: bind buffers, set attrib pointers, iterate meshes
- [x] Material/texture state setting per mesh
- [x] Light uniform upload (via rw_gl_set_lights)
- [x] World matrix upload (transpose for column-major GL)
- [x] Fog data and alpha test uniform upload
- [x] Vertex attrib enable/disable per mesh render

### Tests
- [ ] test_triangle.c — single textured triangle renders
- [x] tests/test_geometry.c — CPU geometry allocation, mesh grouping, lock invalidation, bounding sphere

## Phase 5: Scene Graph

### rw_scene.c
- [x] rw_atomic_create / destroy / set_geometry / set_frame / set_pipeline / set_render_cb / set_hanim_hierarchy / render
- [x] rw_clump_create / destroy / get_frame / set_frame / add_atomic / remove_atomic / add_light / render
- [x] rw_world_create / destroy / add_clump / remove_clump / add_light / remove_light / render / enumerate_lights
- [x] rw_camera_create / destroy / set_frame / begin_update / end_update / clear / set_fov / set_view_window / set_near_far / frustum_test_sphere
- [x] rw_light_create / destroy / set_color / set_radius / set_frame
- [x] Camera: view matrix computation (inverse of frame LTM)
- [x] Camera: projection matrix computation
- [x] Camera: frustum plane extraction
- [x] World: light enumeration (ambient + directional + local)
- [x] World: render loop (iterate clumps → atomics → pipeline)
- [x] rw_camera_clear wired to glClear (color/depth/stencil)

### Tests
- [x] test_scene.c — CPU scene graph coverage for render dispatch, light enumeration, and frustum culling

## Phase 6: Immediate Mode

### rw_render.c
- [x] rw_im2d_render_primitive (dynamic VBO, pixel-to-NDC transform)
- [x] rw_im2d_render_indexed (dynamic VBO + IBO)
- [x] rw_im3d_transform (world matrix transform, stores vertex state)
- [x] rw_im3d_render_primitive (dynamic VBO, shader select by lighting flag)
- [x] rw_im3d_render_indexed (dynamic VBO + IBO)
- [x] rw_im3d_end (clears stored vertex state)

### Tests
- [x] test_im2d.c — primitive and indexed 2D quads render in an offscreen GLES2 context
- [x] test_im3d.c — unlit primitive and ambient-lit indexed 3D immediate geometry render in an offscreen GLES2 context
- [x] test_render.c — GL texture sampler coverage for POT mipmap generation and NPOT fallback

## Phase 7: Animation

### rw_skin.c
- [x] rw_skin_create / destroy
- [x] rw_skin_set_data (indices, weights, inverse matrices)
- [x] rw_skin_set_pipeline (wired to rw_gl_skin_pipeline)
- [x] rw_hanim_create / destroy
- [x] rw_hanim_attach (connect nodes to frames)
- [x] rw_hanim_interpolate (quaternion slerp + translation lerp)
- [x] rw_hanim_update_matrices (hierarchy traversal, PUSH/POP)
- [x] GL: skin vertex shader permutation (skin_dir_point)
- [x] GL: bone matrix upload (global × inverse_bind)
- [x] Pipeline: skinned atomics upload HAnim bone matrices before draw
- [x] GL: skinned vertex attrib setup (weights + indices)

### Tests
- [x] test_skin.c — CPU skin allocation and HAnim interpolation/update coverage

## Phase 8: Polish

- [x] Makefile (static library + all current CPU tests)
- [x] test_im3d.c — focused immediate-mode 3D render coverage
- [x] test_gta.c — mini GTA-like demo
- [ ] Final line count verification (~4,000 target)
- [ ] Code review pass (consistency, no leaks, no GL errors)

## Change Log

| Date | Phase | What Changed |
|---|---|---|
| 2026-04-23 | 1 | rw.h (~460 lines), rw_engine.c (~110 lines) — clean compile with -Wall -Wextra -Werror |
| 2026-04-24 | 1 | Corrected matrix/raw-matrix math to match RenderWare layout, validated custom memory callbacks, added `new_rw/.gitignore`, removed generated object from tracking, and added `tests/test_math.c` |
| 2026-04-24 | 2 | Added `rw_frame.c` frame hierarchy with dirty-list LTM sync and `tests/test_frame.c` coverage for hierarchy composition, get-ltm sync, and child detach |
| 2026-04-24 | 3 | Added CPU-side `rw_material.c`, `rw_raster.c`, and `tests/test_material.c`; GL upload/image loading remained pending |
| 2026-04-24 | 3 | Implemented `rw_image_load` with `stb_image.h` and project allocators; extended material test with runtime TGA loading coverage |
| 2026-04-24 | 4 | Added CPU-side `rw_geometry.c` and `tests/test_geometry.c`; geometry allocation, default material, trilist mesh grouping, polygon-lock mesh invalidation, and bounding sphere coverage pass |
| 2026-04-24 | 5 | Added CPU-side `rw_scene.c` and `tests/test_scene.c`; atomics/clumps/world render dispatch, light enumeration, camera update, and simple frustum sphere testing pass |
| 2026-04-24 | 5 | Added camera frustum plane extraction and switched sphere culling to stored planes; extended scene test coverage for extracted planes and moved cameras |
| 2026-04-24 | 7 | Added CPU-side `rw_skin.c` and `tests/test_skin.c`; skin allocation, HAnim node attachment, keyframe interpolation, and PUSH/POP matrix updates pass |
| 2026-04-24 | Audit/Build | Added `new_rw/Makefile` with strict C99 static-library/test build; fixed zero-axis rotation, near-linear quaternion slerp, matrix/header padding, geometry lock clearing, mesh index validation, and clump frame replacement ownership; all current CPU tests pass through `make test` and sanitized test builds |
| 2026-04-24 | 3/4 GL | Added `rw_gl.c` (~746 lines), `rw_pipeline.c` (~343 lines), `rw_gl_internal.h` (~88 lines); state cache, 7 GLSL ES 1.00 shader permutations, interleaved VBO/IBO packing, default render pipeline, texture upload, lighting, skin matrix upload, camera matrix upload; wired engine lifecycle to GL backend; all existing CPU tests still pass |
| 2026-04-24 | Fixes | Fixed duplicate `#version 100` in shader sources (build_shader prepends it), fixed `u_fog_data` precision mismatch between vertex/fragment shaders, fixed `rw_camera_clear` stub → glClear, fixed `rw_skin_set_pipeline` no-op → actual skin pipeline, fixed test_render.c double-free of raster |
| 2026-04-24 | 5/6/7 | Added `rw_render.c` (~260 lines) with im2d/im3d immediate mode; added `tests/test_render.c` GL render test with GLFW offscreen context; wired rw_camera_clear to glClear; wired rw_skin_set_pipeline; all tests pass |
| 2026-04-24 | Leak Fixes | Freed GL instance VBO/IBO data from `rw_geometry_destroy`; freed raster GL textures from `rw_raster_destroy` via a cache-aware texture delete helper; strict `make test` passes |
| 2026-04-24 | Skin GL | Added atomic HAnim hierarchy attachment and wired `default_render()` to upload skin bone matrices; extended `test_render.c` to render both default and skinned triangles with center-pixel verification; strict `make test` passes |
| 2026-04-24 | im2d GL | Added `tests/test_im2d.c` for offscreen GLES2 primitive and indexed im2d quad rendering; generalized Makefile GL tests; strict `make test` passes |
| 2026-04-24 | Texture GL | Added GLES2 `glGenerateMipmap` support for mipmapped texture filters on valid power-of-two rasters, NPOT/invalid fallback to non-mip filters, and GL sampler coverage in `test_render.c`; strict `make test` passes |
| 2026-04-24 | Demo/Test | Added `tests/test_gta.c`, a GLFW offscreen GTA-like integration demo covering a small world, ambient/point lights, fog state, skinned character rendering, and im2d HUD overlay; added it to `GL_TEST_NAMES`; strict `make test` passes |
| 2026-04-24 | im3d GL | Added `tests/test_im3d.c` covering unlit primitive and ambient-lit indexed im3d rendering in an offscreen GLFW GLES2 context; added it to `GL_TEST_NAMES`; strict `make test` passes |
