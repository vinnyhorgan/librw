# Handoff Prompt For Next LLM

You are continuing work in `/home/dvh/Downloads/librw/new_rw` on `rw`, a small C99 RenderWare-like rendering API for a fantasy console. Target backend is OpenGL ES 2.0 with no extensions. The public API is `rw_` prefixed snake_case. Everything outside `new_rw/` is the original C++ librw codebase and is reference only: study algorithms, do not copy code.

Read these first:
- `new_rw/PLAN.md` - full architecture/spec and intended implementation order.
- `new_rw/PROGRESS.md` - current status and changelog.
- `new_rw/Makefile` - current strict build/test entry point.

## Current State

The project has complete CPU-side runtime, GL backend, and immediate mode infrastructure. All existing tests pass including a GL render test.

Implemented first-party source files:
- `rw.h` (~1027 lines) - public types, structs, enums, APIs, inline math, intrusive lists.
- `rw_engine.c` (~130 lines) - engine lifecycle, render-state array, allocator wrappers. Wires to GL backend on open/start/stop/close.
- `rw_frame.c` - frame hierarchy, dirty propagation, LTM sync.
- `rw_material.c` - materials, textures, texture dictionaries.
- `rw_raster.c` - CPU rasters/images, `stb_image` bridge, CPU image-to-raster copy.
- `rw_geometry.c` - geometry allocation, mesh grouping, bounding spheres.
- `rw_scene.c` - atomics, clumps, worlds, cameras, lights, light enumeration, CPU render dispatch, `rw_camera_clear` wired to glClear.
- `rw_skin.c` - skin data allocation and CPU HAnim attach/interpolate/update, `rw_skin_set_pipeline` wired to skin pipeline.
- `rw_gl.c` (~750 lines) - GLES2 state cache, shader compile/link, 8 shader permutations (GLSL ES 1.00), device init/shutdown, texture upload, lighting upload, skin matrix upload, camera matrix upload.
- `rw_pipeline.c` (~350 lines) - default instance callback (interleaved VBO/IBO packing), default render callback (world matrix → lights → shader select → mesh loop).
- `rw_render.c` (~260 lines) - im2d/im3d immediate mode with dynamic VBOs.
- `rw_gl_internal.h` (~90 lines) - shared internal header for GL backend types/enums/prototypes.

Current tests (all pass):
- `tests/test_math.c`, `tests/test_frame.c`, `tests/test_material.c`, `tests/test_geometry.c`, `tests/test_scene.c`, `tests/test_skin.c`, `tests/test_render.c`

Build and verify with:
```bash
cd /home/dvh/Downloads/librw/new_rw
make test
```

The last verified run used the checked-in Makefile and passed all current tests with:
`cc -I. -Ivendor/glad/include -Ivendor/glfw/include -std=c99 -Wall -Wextra -Werror -pedantic -O2 ... -lm -lglfw -ldl`.

## GL Backend Architecture

`rw_gl.c` defines `GLAD_GLES2_IMPLEMENTATION` — this is the ONLY file that does so. Other GL-using files (`rw_pipeline.c`, `rw_scene.c`, `rw_skin.c`, `rw_render.c`) include `<glad/gles2.h>` for declarations only and `rw_gl_internal.h` for internal types/prototypes.

### State Cache (`RwGlState`)
Tracks blend, depth test/write, cull face, bound textures, current program. Every GL state change goes through cache checks to skip redundant calls.

### Shader System
- 8 permutations compiled at `rw_engine_start` via `compile_all_shaders()`.
- Permutations: `default`, `default_dir`, `default_dir_point`, `skin`, `skin_dir_point`, `im2d`, `im3d`, `im3d_lit`.
- Built by prepending `#define` strings before shared vertex/fragment source.
- Attribute locations bound via `glBindAttribLocation` before linking: in_pos=0, in_normal=1, in_tex0=2, in_color=3, in_weights=4, in_indices=5.
- Uniform locations cached per-program in `RwShaderUniforms` structs.

### Pipeline (`rw_pipeline.c`)
- `default_instance()`: Computes interleaved stride from geometry flags, packs positions/normals/texcoords/colors/weights/indices into single VBO, packs per-mesh indices into IBO. Stores `RwGlMeshData` in `geometry->gl_data`. Re-instance calls `rw_gl_destroy_instance_data` to clean up old GPU buffers. Bone indices are masked to `0x3F` at pack time to stay within the 64-matrix shader limit.
- `default_render()`: Uploads world matrix, enumerates lights, selects shader permutation, flushes render state, sets up vertex input, iterates meshes uploading material uniforms + binding textures + issuing `glDrawElements`. Early-returns if `mesh_header` is NULL.
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

## Best Next Tasks (Prioritized)

1. **Fix GL resource leaks** — `rw_geometry_destroy` should call `rw_gl_destroy_instance_data(geo)` to free VBO/IBO. `rw_raster_destroy` should call `glDeleteTextures` if `r->gl.texid` is set. These are real leaks that will accumulate during normal use.
2. **Fix render test pixel verification** — The GL render test passes (no errors, no crash) but the center pixel is black. The triangle isn't rendering correctly — likely the camera projection matrix needs fixing (currently orthographic-style, not perspective). Once the projection is correct, the pixel assertion in `test_render.c` can be re-enabled.
3. **Wire skin bone upload in `default_render`** — The skin render path in `default_render()` doesn't yet call `rw_gl_upload_skin_matrices`. It needs the `RwHAnimHier` accessible from the atomic. Add `void *user_data` to `RwAtomic` or a dedicated `RwHAnimHier *hanim` field, then call `rw_gl_upload_skin_matrices(hier, geo->skin, shader_idx)` before the mesh loop when `data->has_skin`.
4. **Texture upload on raster creation** — Hook `rw_raster_from_image` to call `rw_gl_upload_raster` when a GL context is available, so textures are uploaded eagerly rather than lazily during the first render.
5. **Auto-mipmapping** — Add `glGenerateMipmap` support for texture mipmaps.
6. **`test_im2d.c`** — Test 2D overlay rendering with im2d functions.
7. **`test_gta.c`** — Mini GTA-like demo: ground plane, building boxes, animated character, camera follow, fog, HUD overlay.

## Do Not Do

- Do not copy C++ librw code verbatim.
- Do not introduce C++ or non-C99 constructs.
- Do not make the core library depend on GLFW.
- Do not implement broad compatibility layers unless a concrete shipped data/API need appears.
- Do not audit or rewrite `vendor/glfw`, `vendor/glad`, or `vendor/stb` unless the task is specifically about third-party code.

## Current Known Gaps

- **GL resource leaks**: `rw_geometry_destroy` doesn't call `rw_gl_destroy_instance_data`; `rw_raster_destroy` doesn't call `glDeleteTextures`.
- **Render test pixel is black**: camera projection matrix is orthographic-style, not perspective; triangle may not be in clip volume.
- **Skin bone matrices not uploaded**: `default_render()` selects the skin shader but never calls `rw_gl_upload_skin_matrices`.
- `rw_raster_from_image` does CPU copy only; GL upload happens lazily in the render loop.
- No auto-mipmapping (`glGenerateMipmap`) yet.
- `RwHAnimHier.anim_data` is borrowed external storage; destroy does not free it.
