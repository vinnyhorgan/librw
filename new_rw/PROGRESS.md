# rw — Implementation Progress

## Status: Phase 7 CPU ANIMATION PROGRESS - strict Makefile build and CPU tests pass

## Phase Overview

| Phase | Files | Status | Notes |
|---|---|---|---|
| 1. Foundation | `rw.h`, `rw_engine.c` | Done | Types, math, engine lifecycle — strict C99 syntax check passes |
| 2. Frame hierarchy | `rw_frame.c` | Done | Transform tree, dirty propagation — math/frame tests pass |
| 3. GL backend core | `rw_gl.c` (partial), `rw_raster.c`, `rw_material.c` | In progress | CPU material/texture/texdict, raster/image loading done; GL upload/shaders not started |
| 4. Geometry + Pipeline | `rw_geometry.c`, `rw_pipeline.c` | In progress | CPU geometry allocation, mesh building, bounding sphere done; GPU instancing/render not started |
| 5. Scene graph | `rw_scene.c` | In progress | CPU atomic/clump/world/light/camera basics plus frustum planes done; GL clear/render integration pending |
| 6. Immediate mode | `rw_render.c` | Not started | im2d + im3d |
| 7. Animation | `rw_skin.c` | In progress | CPU skin allocation, HAnim attach/interpolate/update done; GL skin pipeline pending |
| 8. Polish | `test_gta.c`, Makefile | In progress | Strict Makefile added; integration demo not started |

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
- [x] rw_engine_open (lifecycle transition; GL device hooks come later)
- [x] rw_engine_start (default render-state setup; shader compile comes with GL backend)
- [x] rw_engine_stop
- [x] rw_engine_close
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

### rw_gl.c (partial)
- [ ] GL state cache struct and flush functions
- [ ] Shader compilation from string literals
- [ ] Shader permutation management
- [ ] Shader source: default.vert, default.frag (as string literals)
- [ ] Shader source: im2d.vert, im2d.frag
- [ ] rw_gl_init (compile all shaders, cache uniform locations)
- [ ] rw_gl_shutdown

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
- [ ] Default pipeline: instance callback
- [ ] Default pipeline: render callback
- [ ] Interleaved vertex buffer packing
- [ ] GL instance: VBO/IBO creation and upload
- [ ] GL render: bind buffers, set attrib pointers, iterate meshes
- [ ] Material/texture state setting per mesh
- [ ] Light uniform upload
- [ ] World matrix upload

### Tests
- [ ] test_triangle.c — single textured triangle renders
- [x] tests/test_geometry.c — CPU geometry allocation, mesh grouping, lock invalidation, bounding sphere

## Phase 5: Scene Graph

### rw_scene.c
- [x] rw_atomic_create / destroy / set_geometry / set_frame / set_pipeline / set_render_cb / render
- [x] rw_clump_create / destroy / get_frame / set_frame / add_atomic / remove_atomic / add_light / render
- [x] rw_world_create / destroy / add_clump / remove_clump / add_light / remove_light / render / enumerate_lights
- [x] rw_camera_create / destroy / set_frame / begin_update / end_update / clear / set_fov / set_view_window / set_near_far / frustum_test_sphere
- [x] rw_light_create / destroy / set_color / set_radius / set_frame
- [x] Camera: view matrix computation (inverse of frame LTM)
- [x] Camera: projection matrix computation
- [x] Camera: frustum plane extraction
- [x] World: light enumeration (ambient + directional + local)
- [x] World: render loop (iterate clumps → atomics → pipeline)

### Tests
- [x] test_scene.c — CPU scene graph coverage for render dispatch, light enumeration, and frustum culling

## Phase 6: Immediate Mode

### rw_render.c
- [ ] rw_im2d_render_primitive
- [ ] rw_im2d_render_indexed
- [ ] rw_im3d_transform
- [ ] rw_im3d_render_primitive
- [ ] rw_im3d_render_indexed
- [ ] rw_im3d_end
- [ ] im2d: dynamic VBO, shader, pixel-to-NDC transform
- [ ] im3d: dynamic VBO, shader, world transform

### Tests
- [ ] test_im2d.c — 2D overlays render on top of 3D scene

## Phase 7: Animation

### rw_skin.c
- [x] rw_skin_create / destroy
- [x] rw_skin_set_data (indices, weights, inverse matrices)
- [ ] rw_skin_set_pipeline
- [x] rw_hanim_create / destroy
- [x] rw_hanim_attach (connect nodes to frames)
- [x] rw_hanim_interpolate (quaternion slerp + translation lerp)
- [x] rw_hanim_update_matrices (hierarchy traversal, PUSH/POP)
- [ ] GL: skin vertex shader permutation
- [ ] GL: bone matrix upload (global × inverse_bind)
- [ ] GL: skinned vertex attrib setup (weights + indices)

### Tests
- [x] test_skin.c — CPU skin allocation and HAnim interpolation/update coverage

## Phase 8: Polish

- [x] Makefile (static library + all current CPU tests)
- [ ] test_gta.c — mini GTA-like demo
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
