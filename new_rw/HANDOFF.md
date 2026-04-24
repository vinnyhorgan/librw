# rw — Phase 2 Complete, Continuing Implementation

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

## What's Next: Phase 3 — GL Backend Core + Textures

Start `rw_gl.c` (partial), `rw_raster.c`, and `rw_material.c` per `PLAN.md` and `PROGRESS.md`.

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
