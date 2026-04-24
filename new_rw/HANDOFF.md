# Handoff Prompt For Next LLM

You are continuing work in `/home/dvh/Downloads/librw/new_rw` on `rw`, a small C99 RenderWare-like rendering API for a fantasy console. The target backend is OpenGL ES 2.0 with no extensions. The public API is `rw_` prefixed snake_case. Everything outside `new_rw/` is the original C++ librw codebase and is reference only: study algorithms there when useful, but do not copy code.

Start by reading:
- `new_rw/PLAN.md` for the architecture/spec and intended implementation order.
- `new_rw/PROGRESS.md` for current status and changelog.
- `new_rw/Makefile` for the strict build/test entry point.

Then inspect only the source files needed for the task you choose.

## Current Verified State

The project has a complete CPU-side runtime, GL backend, default/skin pipelines, and immediate mode infrastructure. All current tests pass, including a GLFW offscreen GLES2 render test with center-pixel verification enabled.

Verify with:
```bash
cd /home/dvh/Downloads/librw/new_rw
make test
```

The last verified run passed all tests with strict C99 flags:
`cc -I. -Ivendor/glad/include -Ivendor/glfw/include -std=c99 -Wall -Wextra -Werror -pedantic -O2 ... -lm -lglfw -ldl`.

Current tests:
- `tests/test_math.c`
- `tests/test_frame.c`
- `tests/test_material.c`
- `tests/test_geometry.c`
- `tests/test_scene.c`
- `tests/test_skin.c`
- `tests/test_render.c`

## Implemented Source Files

- `rw.h` - public types, structs, enums, APIs, inline math, intrusive lists.
- `rw_engine.c` - engine lifecycle, render-state array, allocator wrappers, GL backend lifecycle wiring.
- `rw_frame.c` - frame hierarchy, dirty propagation, LTM sync.
- `rw_material.c` - materials, textures, texture dictionaries.
- `rw_raster.c` - CPU rasters/images, `stb_image` bridge, CPU image-to-raster copy, GL texture cleanup on destroy.
- `rw_geometry.c` - geometry allocation, mesh grouping, bounding spheres, GL instance cleanup on destroy.
- `rw_scene.c` - atomics, clumps, worlds, cameras, lights, light enumeration, CPU render dispatch, `rw_camera_clear` wired to `glClear`.
- `rw_skin.c` - skin data allocation and CPU HAnim attach/interpolate/update, `rw_skin_set_pipeline` wired to skin pipeline.
- `rw_gl.c` - GLES2 state cache, shader compile/link, 8 shader permutations, device init/shutdown, texture upload, cache-aware texture deletion, lighting upload, skin matrix upload, camera matrix upload.
- `rw_pipeline.c` - default instance callback and render callback.
- `rw_render.c` - im2d/im3d immediate mode with dynamic VBOs.
- `rw_gl_internal.h` - shared internal GL backend declarations.

## GL Backend Notes

- `rw_gl.c` is the only file that defines `GLAD_GLES2_IMPLEMENTATION`.
- Other GL-using files include `<glad/gles2.h>` for declarations and `rw_gl_internal.h` for internal backend prototypes/types.
- `RwGlState` caches blend, depth test/write, cull face, bound textures, and current program. Route GL state changes through cache helpers when helpers exist.
- `rw_gl_delete_texture(unsigned int *texid)` deletes textures and clears stale cached bindings. Use it instead of direct `glDeleteTextures` for raster-owned textures.
- Shader permutations are compiled at `rw_engine_start`: `default`, `default_dir`, `default_dir_point`, `skin`, `skin_dir_point`, `im2d`, `im3d`, `im3d_lit`.
- Attribute locations are fixed before link: `in_pos=0`, `in_normal=1`, `in_tex0=2`, `in_color=3`, `in_weights=4`, `in_indices=5`.
- Matrix upload convention is `RwMatrix` -> `RwRawMatrix` -> transpose -> `glUniformMatrix4fv`, converting RenderWare row-major layout to GL column-major expectations.

## Pipeline Notes

- `default_instance()` computes an interleaved vertex stride from geometry flags, packs positions/normals/texcoords/colors/skin weights/bone indices into one VBO, packs per-mesh indices into one IBO, and stores `RwGlMeshData` in `geometry->gl_data`.
- Re-instancing calls `rw_gl_destroy_instance_data(geo)` before creating new GL buffers.
- `rw_geometry_destroy()` now also calls `rw_gl_destroy_instance_data(geo)` so VBO/IBO resources are not leaked.
- Bone indices are masked to `0x3F` while packing to stay within the 64-matrix shader limit.
- `default_render()` uploads the world matrix, enumerates lights, selects a shader, flushes render state, configures vertex input, iterates meshes, uploads material uniforms, binds textures, and issues `glDrawElements`.
- `default_render()` currently does not upload skin bone matrices before drawing skinned geometry. This is the highest-priority functional gap.

## Important Invariants

- Keep runtime code C99, not C++.
- Prefer small direct C functions over new abstraction.
- Keep the runtime library windowing-free; GLFW is for tests only.
- Runtime dependencies currently allowed: `vendor/stb/stb_image.h` for image loading and `vendor/glad/` for GL loading.
- Matrix layout is RenderWare-style rows: `right`, `up`, `at`, `pos`, each padded to 16 bytes.
- Matrix multiplication uses row-vector composition. Child LTM is computed as `child->matrix * parent->ltm`.
- `RwFrame` hierarchy is first-child/next-sibling. Dirty roots are stored in `rw_engine.frame_dirty_list` through `RwFrame.in_dirty`.
- Intrusive links must be initialized before use and removed before re-linking.
- `RwGeometry` uses `uint16_t` triangle indices; geometry creation rejects more than `UINT16_MAX` vertices.
- `RwMeshHeader` is padded to 16 bytes so the `RwMesh` array stored immediately after it remains pointer-aligned.
- Current mesh building supports triangle lists only; `RW_GEO_TRISTRIP` intentionally returns failure.
- `RwClump` owns its current frame. `rw_clump_set_frame` destroys the old owned frame and takes ownership of the new frame.
- `RwHAnimHier.anim_data` is borrowed external storage; `rw_hanim_destroy()` does not free it.

## Best Next Tasks

1. **Wire skin bone upload in `default_render()`**
   The skin shader path exists and `rw_gl_upload_skin_matrices(RwHAnimHier *hier, RwSkin *skin, int shader_idx)` exists, but `default_render()` never calls it. Make a minimal API/storage choice so an `RwAtomic` can reference its `RwHAnimHier` (for example a dedicated `RwHAnimHier *hanim` field plus setter, or a generic `void *user_data` only if that is clearly better). When `data->has_skin` is true and the hierarchy is available, call `rw_gl_upload_skin_matrices()` after shader selection/use and before the mesh draw loop. Add focused coverage if practical; otherwise preserve existing tests and note the residual test gap.

2. **Texture upload on raster creation/copy**
   `rw_raster_from_image()` currently does CPU copy only. Texture upload happens lazily on first render. Consider eager upload only if a GL context/backend is available; avoid making CPU-only tests require a GL context.

3. **Auto-mipmapping**
   Add `glGenerateMipmap` support for mipmapped texture filters. Keep GLES2/no-extension constraints in mind and avoid generating mipmaps for invalid dimensions or unavailable texture data.

4. **Add `tests/test_im2d.c`**
   Cover 2D overlay rendering with the immediate mode API.

5. **Add `tests/test_gta.c` mini demo**
   Exercise a tiny GTA-like scene: ground plane, building boxes, animated character, follow camera, fog, and HUD overlay.

## Do Not Do

- Do not copy C++ librw code verbatim.
- Do not introduce C++ or non-C99 constructs.
- Do not make the core runtime depend on GLFW.
- Do not rewrite broad compatibility layers unless a concrete shipped data/API need appears.
- Do not audit or rewrite `vendor/glfw`, `vendor/glad`, or `vendor/stb` unless the task is specifically about third-party code.
- Do not undo unrelated user or agent changes in the worktree.

## Current Known Gaps

- Skin bone matrices are not uploaded by `default_render()` yet.
- `rw_raster_from_image()` does CPU copy only; GL upload happens lazily in the render loop.
- No auto-mipmapping yet.
- No im2d render test yet.
- No GTA-like integration demo yet.
