# Handoff Prompt For Next LLM

You are continuing work in `/home/dvh/Downloads/librw/new_rw` on `rw`, a small C99 RenderWare-like rendering API for a fantasy console. Target backend is OpenGL ES 2.0 with no extensions. The public API is `rw_` prefixed snake_case. Everything outside `new_rw/` is the original C++ librw codebase and is reference only: study algorithms, do not copy code.

Read these first:
- `new_rw/PLAN.md` - full architecture/spec and intended implementation order.
- `new_rw/PROGRESS.md` - current status and changelog.
- `new_rw/Makefile` - current strict build/test entry point.

## Current State

The project has first-party CPU-side runtime code and tests for foundation, frames, materials/textures/rasters, geometry, scene graph/camera/lights, and skin/HAnim animation. GL backend, default pipeline, immediate mode, GL texture upload, and demos are not implemented yet.

Implemented first-party source files:
- `rw.h` - public types, structs, enums, APIs, inline math, intrusive lists.
- `rw_engine.c` - engine lifecycle, render-state array, allocator wrappers.
- `rw_frame.c` - frame hierarchy, dirty propagation, LTM sync.
- `rw_material.c` - materials, textures, texture dictionaries.
- `rw_raster.c` - CPU rasters/images, `stb_image` bridge, CPU image-to-raster copy.
- `rw_geometry.c` - geometry allocation, mesh grouping, bounding spheres.
- `rw_scene.c` - atomics, clumps, worlds, cameras, lights, light enumeration, CPU render dispatch.
- `rw_skin.c` - skin data allocation and CPU HAnim attach/interpolate/update.

Current tests:
- `tests/test_math.c`
- `tests/test_frame.c`
- `tests/test_material.c`
- `tests/test_geometry.c`
- `tests/test_scene.c`
- `tests/test_skin.c`

Build and verify with:
```bash
cd /home/dvh/Downloads/librw/new_rw
make test
```

The last verified run used the checked-in Makefile and passed all current tests with:
`cc -I. -std=c99 -Wall -Wextra -Werror -pedantic -O2 ... -lm`.

## Important Invariants

- Keep runtime code C99, not C++.
- Prefer small direct C functions over abstraction.
- Keep the runtime library windowing-free; GLFW/glad are for future tests only.
- Runtime dependency allowed by plan today: `vendor/stb/stb_image.h` for image loading.
- Matrix layout is RenderWare-style rows: `right`, `up`, `at`, `pos`, each padded to 16 bytes.
- Matrix multiplication uses row-vector composition. Child LTM is computed as `child->matrix * parent->ltm`.
- `RwFrame` hierarchy is first-child/next-sibling. Dirty roots are stored in `rw_engine.frame_dirty_list` through `RwFrame.in_dirty`.
- Intrusive links must be initialized before use and removed before re-linking.
- `RwGeometry` uses `uint16_t` triangle indices, so geometry creation rejects more than `UINT16_MAX` vertices.
- `RwMeshHeader` is padded to 16 bytes so the `RwMesh` array stored immediately after it remains pointer-aligned.
- Current mesh building supports triangle lists only; `RW_GEO_TRISTRIP` intentionally returns failure.
- `RwClump` owns its current frame. `rw_clump_set_frame` destroys the old owned frame and takes ownership of the new frame.
- `rw_skin_set_pipeline` is intentionally a no-op until a GL skin pipeline exists.

## Recent Audit Fixes

The latest audit added the Makefile and fixed these issues:
- `rw_geometry_unlock` now clears `geo->locked`.
- `rw_geometry_build_meshes` now rejects out-of-range material IDs and vertex indices before writing mesh indices.
- `rw_geometry_create` rejects vertex counts that cannot fit `uint16_t` mesh indices.
- `rw_matrix_rotate` treats a zero axis as identity for replace and no-op for concat.
- Matrix-producing helpers now zero padding fields to avoid carrying indeterminate data.
- `RwMeshHeader` now includes explicit padding so the trailing `RwMesh` array is aligned under UBSan/ASan.
- `rw_quat_slerp` now clamps dot products above 1 and linearly interpolates/normalizes very close quaternions instead of returning the start pose.
- `rw_clump_set_frame` now releases the previous owned frame instead of leaking it.
- Tests were extended to cover these regressions.

## Best Next Task

The best next implementation step is Phase 3/4 GL groundwork:
- Add `rw_gl.c` with GLES2 state cache, shader compile/link helpers, device init/shutdown hooks, and texture upload support.
- Add `rw_pipeline.c` default pipeline scaffolding for CPU-to-GPU instance data and mesh rendering.
- Wire `rw_engine_open/start/stop/close` to GL backend hooks once they exist.
- Add the first GL-context test only after the backend has enough surface area. Keep GL test link flags separate from the core library.

Useful librw references from the root codebase:
- `src/gl/gl3device.cpp` - state/device lifecycle concepts.
- `src/gl/gl3shader.cpp` - shader compile/link/permutation approach.
- `src/gl/gl3raster.cpp` - texture upload concepts.
- `src/gl/gl3pipe.cpp` - geometry instance buffer construction.
- `src/gl/gl3render.cpp` - default render callback and light upload flow.
- `src/geometry.cpp`, `src/frame.cpp`, `src/camera.cpp`, `src/world.cpp`, `src/skin.cpp`, `src/hanim.cpp` - CPU algorithms already partially ported.

## Do Not Do

- Do not copy C++ librw code verbatim.
- Do not introduce C++ or non-C99 constructs.
- Do not make the core library depend on GLFW.
- Do not implement broad compatibility layers unless a concrete shipped data/API need appears.
- Do not audit or rewrite `vendor/glfw`, `vendor/glad`, or `vendor/stb` unless the task is specifically about third-party code.

## Current Known Gaps

- No `rw_gl.c`, `rw_pipeline.c`, or `rw_render.c` yet.
- `rw_raster_from_image` is CPU copy only; no GL texture upload.
- `rw_camera_clear` is a stub until GL clear support exists.
- `rw_atomic_render` dispatches callbacks/pipeline only; no default pipeline exists.
- No `test_clear`, `test_triangle`, `test_im2d`, or `test_gta` yet.
- `RwHAnimHier.anim_data` is borrowed external storage; destroy does not free it.
- Vendor files are third-party dependencies and were not part of the first-party audit.
