# rw — Phase 7 CPU Animation Started

## Project

`rw` is a C99 reimplementation of a RenderWare-like 3D rendering API for a fantasy console, targeting OpenGL ES 2.0 with no extensions. Lives in `new_rw/`. Everything outside `new_rw/` is the original librw C++ codebase — reference only, port algorithms from it, do not copy code.

Full spec is in `new_rw/PLAN.md` (~1064 lines). Progress tracking in `new_rw/PROGRESS.md`.

## What's Done

**Phase 1 (Foundation) — COMPLETE.** Compiles clean with `gcc -std=c99 -Wall -Wextra -Werror`.

### `new_rw/rw.h` (~980 lines)
- All primitive types (rw_int8 through rw_bool32)
- Math types: RwV2d, RwV3d, RwV4d, RwQuat, RwRGBAf, RwRGBA, RwTexCoords
- RwMatrix (4x3 with pad uint32 per row), RwRawMatrix (4x4 for GPU)
- Vertex types: RwIm2DVertex, RwIm3DVertex
- BBox, Sphere, Rect
- Intrusive linked list: RwLink, RwLinkList with init/add/append/remove + RW_LINK_DATA/RW_FORLIST macros
- All forward declarations, all enums (LightType, CombineOp, PrimitiveType, Geo flags, Lock flags, RenderState, Cull, Blend, Tex filter/wrap, Camera clear, im3d flags, Frame dirty flags)
- All core structs: Frame, Triangle, Mesh, MeshHeader, MorphTarget, Geometry, Material, GlRaster, Raster, Image, Texture, TexDict, Atomic, Clump, Camera, Light, WorldLights, World, ObjPipeline, Skin, HAnimNodeInfo, HAnimKeyFrame, HAnimHier, Engine
- All API function declarations for every module (engine, frame, geometry, material, texture, raster, image, texdict, atomic, clump, world, camera, light, im2d, im3d, skin, hanim)
- Static inline math:
  - V3d: add, sub, scale, dot, cross, length, normalize, lerp
  - Matrix: set_identity, multiply using RenderWare's row-vector/layout convention, invert (general cofactor), translate, rotate (axis-angle), scale — all with CombineOp support
  - RawMatrix: explicit field-wise multiply and transpose matching RenderWare layout
  - Quaternion: mult (Hamilton), slerp, to_matrix, from_matrix (trace method, 4 cases)

### `new_rw/rw_engine.c` (~110 lines)
- State machine: RW_ENGINE_DEAD → INITIALIZED → OPENED → STARTED
- rw_engine_init: validates state, sets memory funcs (default: malloc/realloc/free), rejects incomplete custom allocator tables
- rw_engine_open/close: state transitions (device hooks stubbed for later)
- rw_engine_start/stop: sets default render states, state transitions
- rw_engine_term: clears engine struct, returns to DEAD
- rw_set_render_state / rw_get_render_state
- rw_malloc / rw_realloc / rw_free wrappers through engine function pointers

### `new_rw/tests/test_math.c`
- Regression coverage for RenderWare-layout matrix multiplication
- Verifies replace-mode translate/scale reset the whole matrix
- Verifies matrix inverse round-trip against identity

### `new_rw/.gitignore`
- Ignores local build artifacts (`*.o`, libraries, root-level test executables, build dirs)
- `new_rw/rw_engine.o` is generated and should remain untracked

## Phase 2 — Frame Hierarchy Complete

Implemented `new_rw/rw_frame.c` (~184 lines). Reference: `src/frame.cpp`.

### Implemented functions:
- `rw_frame_create` — allocate, zero, identity local/LTM, init root and dirty link
- `rw_frame_destroy` — detach from parent, remove from dirty list if needed, recursively destroy children, free
- `rw_frame_add_child` — append to parent's child list, set parent/root pointers, merge dirty roots
- `rw_frame_remove_child` — unlink from sibling chain, make detached subtree its own root, mark old/new roots dirty
- `rw_frame_get_ltm` — sync dirty hierarchy immediately and remove root from dirty list before returning `&frame->ltm`
- `rw_frame_set_matrix` — copy matrix, mark subtree dirty
- `rw_frame_translate/rotate/scale` — apply transform with `RwCombineOp`, mark dirty
- `rw_frame_sync_dirty` — drain `rw_engine.frame_dirty_list`, recursively recompute LTMs

### Key algorithms from librw reference:
- **updateObjects**: When a frame's local matrix changes, mark the hierarchy root with RW_FRAME_HIERARCHYSYNC and add that root to engine's frame_dirty_list if not already present; mark the changed frame with RW_FRAME_SUBTREESYNC.
- **syncHierarchyLTM**: Recursive — if this frame or an ancestor has SUBTREESYNC, LTM = frame->matrix × parent->ltm using `rw_matrix_multiply(&ltm, &frame->matrix, &parent->ltm)`. Root LTM is its local matrix. Clear flags after update.

### Key details:
- Frame hierarchy uses first-child/next-sibling tree (not a binary tree)
- Root frame's LTM = its local matrix (no parent to combine with)
- Dirty propagation uses the engine's `frame_dirty_list` (intrusive linked list via `in_dirty` link)
- Frame root pointer should point to the root of the hierarchy

### Tests
- `tests/test_math.c` passes before and after Phase 2.
- `tests/test_frame.c` covers hierarchy creation, required child LTM composition order, `rw_frame_get_ltm` dirty sync, and child detach/new-root behavior.

### Verified commands
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_math.c rw_engine.c -lm -o test_math && ./test_math`
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_frame.c rw_engine.c rw_frame.c -lm -o test_frame && ./test_frame`

## Phase 3 Progress

Added `new_rw/rw_material.c`, `new_rw/rw_raster.c`, and `new_rw/tests/test_material.c`.

Implemented CPU-side resource functions:
- `rw_material_create/destroy`, `rw_material_set_texture/color/surface`
- `rw_texture_create/destroy`, `rw_texture_set_filter/addressing`
- `rw_texdict_create/destroy/add/find`
- `rw_raster_create/destroy/lock/unlock`
- `rw_image_load` through `stb_image.h`, forcing RGBA8 output and using the engine memory callbacks
- `rw_image_destroy`
- `rw_raster_from_image` as a CPU pixel copy only
- `tests/test_material.c` writes a tiny uncompressed TGA fixture at runtime and verifies loaded RGBA pixels

Important ownership behavior:
- Materials start white with ambient/specular/diffuse set to `1.0f` and `ref_count = 1`.
- `rw_material_set_texture` increments the new texture refcount and destroys/releases the old one.
- `rw_texture_destroy` decrements refcount, unlinks from its texdict at zero, destroys owned raster, and frees.
- `rw_texdict_destroy` unlinks and destroys all contained textures.
- Texture filter/addressing is packed as `VVVVUUUU FFFFFFFF`, matching the planned struct field.

Still pending in Phase 3:
- `rw_raster_from_image` does not upload GL texture data yet.
- `rw_gl.c` state cache, shader compilation, shader permutations, and GL lifecycle hooks are not started.

Verified commands:
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_math.c rw_engine.c -lm -o test_math && ./test_math`
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_frame.c rw_engine.c rw_frame.c -lm -o test_frame && ./test_frame`
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_material.c rw_engine.c rw_material.c rw_raster.c -lm -o test_material && ./test_material`

## Phase 4 Progress

Added `new_rw/rw_geometry.c` and `new_rw/tests/test_geometry.c`.

Implemented CPU-side geometry functions:
- `rw_geometry_create/destroy`
- `rw_geometry_lock/unlock`
- `rw_geometry_build_meshes` for triangle lists grouped by `mat_id`
- `rw_geometry_calc_bounding_sphere`

Important behavior:
- Geometries start with `ref_count = 1` and one default white material so `mat_id = 0` triangles are immediately valid.
- Vertex arrays are allocated based on geometry flags: positions always, normals for `RW_GEO_NORMALS`, colors for `RW_GEO_PRELIT`, texcoords for `RW_GEO_TEXTURED`.
- Mesh headers are a single allocation containing `RwMeshHeader`, `RwMesh[num_materials]`, and contiguous `uint16_t` indices.
- `rw_geometry_lock(..., RW_LOCK_POLYGONS)` frees the existing mesh header; `rw_geometry_unlock` rebuilds it when absent.
- Bounding sphere follows the librw reference: compute AABB min/max, center at `(min + max) * 0.5`, radius as distance from center to max.

Verified commands:
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_math.c rw_engine.c -lm -o test_math && ./test_math`
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_frame.c rw_engine.c rw_frame.c -lm -o test_frame && ./test_frame`
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_material.c rw_engine.c rw_material.c rw_raster.c -lm -o test_material && ./test_material`
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_geometry.c rw_engine.c rw_material.c rw_raster.c rw_geometry.c -lm -o test_geometry && ./test_geometry`

## Phase 5 Progress

Added `new_rw/rw_scene.c` and `new_rw/tests/test_scene.c`.

Implemented CPU-side scene functions:
- `rw_atomic_create/destroy/set_geometry/set_frame/set_pipeline/set_render_cb/render`
- `rw_clump_create/destroy/get_frame/set_frame/add_atomic/remove_atomic/add_light/render`
- `rw_world_create/destroy/add_clump/remove_clump/add_light/remove_light/render/enumerate_lights`
- `rw_camera_create/destroy/set_frame/begin_update/end_update/clear/set_fov/set_view_window/set_near_far/frustum_test_sphere`
- `rw_light_create/destroy/set_color/set_radius/set_frame`

Important behavior:
- Atomics retain assigned geometry with `ref_count` and release it on replacement/destruction.
- World render iterates clumps, clumps iterate atomics, and atomics invoke render callbacks before pipeline render callbacks.
- World light enumeration accumulates ambient lights, collects directional lights, and includes point/spot lights whose radius intersects an atomic bounding sphere.
- Camera begin update computes view matrix by inverting the attached frame LTM, updates a GLES-style projection raw matrix, and extracts six normalized frustum planes in world space.
- Frustum sphere testing uses the stored camera planes and supports moved camera frames.
- `RwLight` now has an `in_clump` link so clump-local light lists do not conflict with world light lists.

Verified commands:
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_scene.c rw_engine.c rw_frame.c rw_material.c rw_raster.c rw_geometry.c rw_scene.c -lm -o test_scene && ./test_scene`

## What's Next: Phase 4 GL Prerequisites

Next useful progress is GL-facing Phase 4/3 work: `rw_pipeline.c` instance-data scaffolding plus `rw_gl.c` state/shader/device hooks. Full triangle rendering still requires GL backend state/shaders and pipeline render paths.

## Phase 7 CPU Progress

Added `new_rw/rw_skin.c` and `new_rw/tests/test_skin.c`.

Implemented CPU-side animation functions:
- `rw_skin_create/destroy/set_data`
- `rw_hanim_create/destroy/attach/interpolate/update_matrices`

Important behavior:
- Skins allocate inverse bind matrices as `num_bones * 16` floats and per-vertex bone indices/weights as `num_verts * num_weights` entries.
- Added `RW_HANIM_PUSH` and `RW_HANIM_POP` flags in `rw.h` for stack-based hierarchy traversal.
- `rw_hanim_interpolate` treats `anim_data` as frame-major packed keyframes: `num_frames * num_nodes`.
- `rw_hanim_update_matrices` builds local matrices from interpolated quaternion + translation, accumulates globals through the PUSH/POP stack, writes `h->matrices[index]`, and mirrors local transforms onto attached frames.
- `rw_skin_set_pipeline` remains a no-op until the GL skin pipeline exists.

Verified commands:
- `gcc -std=c99 -Wall -Wextra -Werror -I. tests/test_skin.c rw_engine.c rw_frame.c rw_skin.c -lm -o test_skin && ./test_skin`

## Build Notes
- `gcc -std=c99 -Wall -Wextra -Werror`
- Include path: `-Ivendor` (for glad, stb)
- GL loader: `vendor/glad/include/glad/gles2.h` (header-only GLES 2.0 loader)
- Tests will need GLFW for GL context (not needed for frame.c itself, only for test_frame)

## Architecture Reminders
- All functions prefixed `rw_`, snake_case
- No comments in code unless asked
- C99 only, no C++ features
- Matrix layout: right, up, at, pos vectors (NOT x/y/z rows)
- Follow existing code style in rw.h and rw_engine.c
