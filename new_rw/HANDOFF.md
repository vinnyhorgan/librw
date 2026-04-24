# Handoff Prompt For Next LLM

You are continuing work in `/home/dvh/Downloads/librw/new_rw` on `rw`, a small C99 RenderWare-like rendering API for a fantasy console. Target backend is OpenGL ES 2.0 with no extensions. The public API is `rw_` prefixed snake_case. Everything outside `new_rw/` is the original C++ librw codebase and is reference only: study algorithms, do not copy code.

Read these first:
- `new_rw/PLAN.md` - full architecture/spec and intended implementation order.
- `new_rw/PROGRESS.md` - current status and changelog.
- `new_rw/Makefile` - current strict build/test entry point.

## Current State

The project has complete CPU-side runtime and GL backend infrastructure.

Implemented first-party source files:
- `rw.h` (~1027 lines) - public types, structs, enums, APIs, inline math, intrusive lists.
- `rw_engine.c` (~130 lines) - engine lifecycle, render-state array, allocator wrappers. Wires to GL backend on open/start/stop/close.
- `rw_frame.c` - frame hierarchy, dirty propagation, LTM sync.
- `rw_material.c` - materials, textures, texture dictionaries.
- `rw_raster.c` - CPU rasters/images, `stb_image` bridge, CPU image-to-raster copy.
- `rw_geometry.c` - geometry allocation, mesh grouping, bounding spheres.
- `rw_scene.c` - atomics, clumps, worlds, cameras, lights, light enumeration, CPU render dispatch.
- `rw_skin.c` - skin data allocation and CPU HAnim attach/interpolate/update.
- `rw_gl.c` (~746 lines) - GLES2 state cache, shader compile/link, 7 shader permutations (GLSL ES 1.00), device init/shutdown, texture upload, lighting upload, skin matrix upload, camera matrix upload.
- `rw_pipeline.c` (~343 lines) - default instance callback (interleaved VBO/IBO packing), default render callback (world matrix → lights → shader select → mesh loop).
- `rw_gl_internal.h` (~88 lines) - shared internal header for GL backend types/enums/prototypes.

Current tests (all pass):
- `tests/test_math.c`, `tests/test_frame.c`, `tests/test_material.c`, `tests/test_geometry.c`, `tests/test_scene.c`, `tests/test_skin.c`

Build and verify with:
```bash
cd /home/dvh/Downloads/librw/new_rw
make test
```

The last verified run used the checked-in Makefile and passed all current tests with:
`cc -I. -Ivendor/glad/include -std=c99 -Wall -Wextra -Werror -pedantic -O2 ... -lm`.

## GL Backend Architecture

`rw_gl.c` defines `GLAD_GLES2_IMPLEMENTATION` — this is the ONLY file that does so. Other GL-using files include `<glad/gles2.h>` for declarations only.

### State Cache (`RwGlState`)
Tracks blend, depth test/write, cull face, bound textures, current program. Every GL state change goes through cache checks to skip redundant calls.

### Shader System
- 7 permutations compiled at `rw_engine_start` via `compile_all_shaders()`.
- Permutations: `default`, `default_dir`, `default_dir_point`, `skin_dir_point`, `im2d`, `im3d`, `im3d_lit`.
- Built by prepending `#define` strings before shared vertex/fragment source.
- Attribute locations bound via `glBindAttribLocation` before linking: in_pos=0, in_normal=1, in_tex0=2, in_color=3, in_weights=4, in_indices=5.
- Uniform locations cached per-program in `RwShaderUniforms` structs.

### Pipeline (`rw_pipeline.c`)
- `default_instance()`: Computes interleaved stride from geometry flags, packs positions/normals/texcoords/colors/weights/indices into single VBO, packs per-mesh indices into IBO. Stores `RwGlMeshData` in `geometry->gl_data`.
- `default_render()`: Uploads world matrix, enumerates lights, selects shader permutation, flushes render state, sets up vertex input, iterates meshes uploading material uniforms + binding textures + issuing `glDrawElements`.
- Default and skin pipelines are static `RwObjPipeline` structs returned by `rw_gl_default_pipeline()` / `rw_gl_skin_pipeline()`.

### Matrix Upload Convention
RwMatrix → RwRawMatrix (via `rw_matrix_to_raw`) → transpose (via `rw_raw_matrix_transpose`) → cast to `float*` for `glUniformMatrix4fv`. This converts row-major RenderWare layout to column-major GL.

## Important Invariants

- Keep runtime code C99, not C++.
- Prefer small direct C functions over abstraction.
- Keep the runtime library windowing-free; GLFW/glad are for tests only.
- Runtime dependency allowed by plan today: `vendor/stb/stb_image.h` for image loading, `vendor/glad/` for GL loading.
- Matrix layout is RenderWare-style rows: `right`, `up`, `at`, `pos`, each padded to 16 bytes.
- Matrix multiplication uses row-vector composition. Child LTM is computed as `child->matrix * parent->ltm`.
- `RwFrame` hierarchy is first-child/next-sibling. Dirty roots are stored in `rw_engine.frame_dirty_list` through `RwFrame.in_dirty`.
- Intrusive links must be initialized before use and removed before re-linking.
- `RwGeometry` uses `uint16_t` triangle indices, so geometry creation rejects more than `UINT16_MAX` vertices.
- `RwMeshHeader` is padded to 16 bytes so the `RwMesh` array stored immediately after it remains pointer-aligned.
- Current mesh building supports triangle lists only; `RW_GEO_TRISTRIP` intentionally returns failure.
- `RwClump` owns its current frame. `rw_clump_set_frame` destroys the old owned frame and takes ownership of the new frame.

## Best Next Task

The best next implementation steps:
1. **GL render test**: Create `test_render.c` using GLFW to create a GL context, load glad, init engine, create a camera with a triangle geometry, render a frame, and verify GL output (readpixels or just no crash).
2. **Wire `rw_camera_clear`**: Implement `glClear` with color/depth/stencil flags from the camera clear mask.
3. **Wire `rw_skin_set_pipeline`**: Change from no-op to actually setting the skin pipeline.
4. **Immediate mode**: Implement `rw_im2d_render_primitive`, `rw_im2d_render_indexed`, `rw_im3d_transform/render/end` with dynamic VBOs.
5. **Texture upload on raster creation**: Hook `rw_raster_from_image` to call `rw_gl_upload_raster` when GL context is available.

## Do Not Do

- Do not copy C++ librw code verbatim.
- Do not introduce C++ or non-C99 constructs.
- Do not make the core library depend on GLFW.
- Do not implement broad compatibility layers unless a concrete shipped data/API need appears.
- Do not audit or rewrite `vendor/glfw`, `vendor/glad`, or `vendor/stb` unless the task is specifically about third-party code.

## Current Known Gaps

- No GL render tests yet. Need a GLFW-based test that exercises the full pipeline.
- `rw_camera_clear` is still a stub — needs `glClear`.
- `rw_skin_set_pipeline` is still a no-op.
- Immediate mode (`rw_im2d_*`, `rw_im3d_*`) is declared but not implemented.
- `rw_raster_from_image` does CPU copy only; GL upload happens lazily in the render loop.
- No auto-mipmapping (`glGenerateMipmap`) yet.
- `RwHAnimHier.anim_data` is borrowed external storage; destroy does not free it.
- The skin render path in `default_render` doesn't yet call `rw_gl_upload_skin_matrices` — it needs the HAnimHier accessible from the atomic. A convention or user-data field is needed.
