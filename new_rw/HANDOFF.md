# Handoff Prompt For Next Session

You are continuing work in `/home/dvh/Downloads/librw/new_rw` on `rw`, a compact C99 RenderWare-like rendering API for a fantasy console. The backend target is OpenGL ES 2.0 with no extensions. The public API uses `rw_` prefixed snake_case.

Everything outside `new_rw/` is the original C++ librw codebase and is reference only. Study algorithms there when useful, but do not copy code.

## First Actions

1. Work from `/home/dvh/Downloads/librw/new_rw` unless running repository-level git commands.
2. Run `git status --short` from `/home/dvh/Downloads/librw` before editing. The tree may contain uncommitted work from a previous agent or the user. Do not revert or overwrite unrelated changes.
3. Read `PLAN.md` for architecture, constraints, API shape, and intended implementation order.
4. Read `PROGRESS.md` for implementation status and changelog.
5. Read `Makefile` before running builds; it is the strict build/test entry point.
6. Inspect only the source files needed for the task you choose. Keep context small and avoid vendor code unless necessary.

## Verified State

The runtime has complete CPU-side core systems, a GLES2 backend, default and skin pipelines, immediate-mode infrastructure, GLES2 auto-mipmapping for valid power-of-two rasters, and a GTA-like offscreen integration demo. The current suite passes with strict C99 flags and GLFW offscreen GLES2 render tests.

Verify with:

```bash
make test
```

The last verified run, from `/home/dvh/Downloads/librw/new_rw`, passed all tests after adding `tests/test_gta.c`:

- `tests/test_math.c`
- `tests/test_frame.c`
- `tests/test_material.c`
- `tests/test_geometry.c`
- `tests/test_scene.c`
- `tests/test_skin.c`
- `tests/test_render.c`
- `tests/test_im2d.c`
- `tests/test_gta.c`

The strict build uses:

```text
cc -I. -Ivendor/glad/include -Ivendor/glfw/include -std=c99 -Wall -Wextra -Werror -pedantic -O2 ... -lm -lglfw -ldl
```

## Current Worktree Notes

If this handoff is used before the current changes are committed, expect exactly these intentional changes:

- `new_rw/tests/test_gta.c` - new offscreen GLES2 GTA-like integration demo.
- `new_rw/Makefile` - adds `gta` to `GL_TEST_NAMES` so `make test` builds and runs it.
- `new_rw/PROGRESS.md` - marks the GTA-like demo complete and records the passing verification.
- `new_rw/HANDOFF.md` - this updated next-session prompt.

Current expected repository-level status before committing:

```text
 M new_rw/HANDOFF.md
 M new_rw/Makefile
 M new_rw/PROGRESS.md
?? new_rw/tests/test_gta.c
```

## Current Implementation

- `rw.h` - public types, structs, enums, APIs, inline math, intrusive lists.
- `rw_engine.c` - engine lifecycle, render-state array, allocator wrappers, GL backend lifecycle wiring.
- `rw_frame.c` - frame hierarchy, dirty propagation, LTM sync.
- `rw_material.c` - materials, textures, texture dictionaries.
- `rw_raster.c` - CPU rasters/images, `stb_image` bridge, CPU image-to-raster copy, GL texture cleanup on destroy.
- `rw_geometry.c` - geometry allocation, mesh grouping, bounding spheres, GL instance cleanup on destroy.
- `rw_scene.c` - atomics, clumps, worlds, cameras, lights, light enumeration, CPU render dispatch, `rw_camera_clear` wired to `glClear`, atomic HAnim hierarchy attachment.
- `rw_skin.c` - skin data allocation, CPU HAnim attach/interpolate/update, `rw_skin_set_pipeline` wired to the skin pipeline.
- `rw_gl.c` - GLES2 state cache, shader compile/link, 8 shader permutations, device init/shutdown, texture upload/mipmap generation, cache-aware texture deletion, lighting upload, skin matrix upload, camera matrix upload.
- `rw_pipeline.c` - default instance callback and render callback for default and skinned geometry.
- `rw_render.c` - im2d/im3d immediate mode with dynamic VBOs.
- `rw_gl_internal.h` - shared internal GL backend declarations.

## Recent Completed Work

Do not redo these unless a regression is found.

- Skin rendering is wired through the default render path.
- `RwAtomic` has a borrowed `RwHAnimHier *hanim` pointer.
- `rw_atomic_set_hanim_hierarchy(RwAtomic *a, RwHAnimHier *h)` attaches the hierarchy to an atomic.
- `default_render()` calls `rw_gl_upload_skin_matrices(a->hanim, geo->skin, shader_idx)` when geometry is skinned and a hierarchy is attached.
- `tests/test_skin.c` covers the HAnim setter.
- `tests/test_render.c` renders default and skinned triangles in an offscreen GLFW GLES2 context and checks the center pixel.
- `tests/test_render.c` covers mipmapped texture sampler setup: valid power-of-two rasters generate mipmaps, while NPOT rasters fall back to non-mip filters without GL errors.
- `tests/test_im2d.c` renders primitive and indexed im2d quads in an offscreen GLFW GLES2 context and checks the center pixel.
- `tests/test_gta.c` renders a small GTA-like scene in an offscreen GLFW GLES2 context and checks scene/HUD pixels with no GL errors.
- `Makefile` groups GLFW-dependent render tests through `GL_TEST_NAMES := render im2d gta`.

## `tests/test_gta.c` Details

The GTA-like demo intentionally stays small and deterministic. It creates:

- A hidden GLFW OpenGL ES 2.0 context for testing only.
- A camera, world, and clump.
- Ground/building quads using prelit geometry and the default pipeline.
- Ambient and point lights.
- Fog render state.
- A skinned triangle character through `RwSkin`, `RwHAnimHier`, `rw_atomic_set_hanim_hierarchy`, and `rw_skin_set_pipeline`.
- An im2d HUD bar drawn after the world render.
- GL error checks plus center-scene and HUD pixel assertions.

Ownership notes in the test:

- Clump destruction destroys its atomics and their geometry.
- Atomic frames are parented under the clump frame so clump destruction also destroys them.
- `RwSkin` and `RwHAnimHier` remain borrowed/external and are destroyed explicitly after the clump.
- The point light owns no frame; the test stores `point_frame`, destroys the light, then destroys the frame.
- The animation keyframes are local stack data used only for interpolation; `hier->anim_data` is cleared before returning from `make_skinned_character`.

## GL Backend Notes

- `rw_gl.c` is the only file that defines `GLAD_GLES2_IMPLEMENTATION`.
- Other GL-using files include `<glad/gles2.h>` for declarations and `rw_gl_internal.h` for internal backend prototypes/types.
- `RwGlState` caches blend, depth test/write, cull face, bound textures, and current program. Route GL state changes through cache helpers when helpers exist.
- `rw_gl_delete_texture(unsigned int *texid)` deletes textures and clears stale cached bindings. Use it instead of direct `glDeleteTextures` for raster-owned textures.
- `rw_gl_set_texture_sampler()` generates GLES2 mipmaps once for mipmapped filters only when the raster has pixels and power-of-two dimensions. Invalid/NPOT rasters keep textures complete by falling back to non-mip min filters.
- `rw_gl_upload_raster()` deletes/recreates the GL texture and resets `has_mipmaps`; sampler setup is responsible for regenerating mip levels when a mipmapped filter is requested.
- Shader permutations are compiled at `rw_engine_start`: `default`, `default_dir`, `default_dir_point`, `skin`, `skin_dir_point`, `im2d`, `im3d`, `im3d_lit`.
- Attribute locations are fixed before link: `in_pos=0`, `in_normal=1`, `in_tex0=2`, `in_color=3`, `in_weights=4`, `in_indices=5`.
- Matrix upload convention is `RwMatrix` -> `RwRawMatrix` -> transpose -> `glUniformMatrix4fv`, converting RenderWare row-major layout to GL column-major expectations.
- `rw_im2d_render_*` maps pixel coordinates through the current camera framebuffer size when present, otherwise through the GL viewport.
- The default engine render state enables back-face culling with `RW_CULL_CCW`; immediate-mode tests that draw screen-space quads should disable culling with `rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE)` unless winding is deliberately tested.

## Pipeline Notes

- `default_instance()` computes an interleaved vertex stride from geometry flags, packs positions/normals/texcoords/colors/skin weights/bone indices into one VBO, packs per-mesh indices into one IBO, and stores `RwGlMeshData` in `geometry->gl_data`.
- Re-instancing calls `rw_gl_destroy_instance_data(geo)` before creating new GL buffers.
- `rw_geometry_destroy()` calls `rw_gl_destroy_instance_data(geo)` so VBO/IBO resources are not leaked.
- Bone indices are masked to `0x3F` while packing to stay within the 64-matrix shader limit.
- `default_render()` uploads the world matrix, enumerates lights, selects a shader, uploads skin bone matrices when available, flushes render state, configures vertex input, iterates meshes, uploads material uniforms, binds textures, and issues `glDrawElements`.

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
- `RwAtomic.hanim` is borrowed external storage; `rw_atomic_destroy()` does not destroy it.

## Best Next Tasks

1. **Polish/review pass**
   Check for leaks, GL errors, render-state inconsistencies, line-count drift, and missing coverage around alpha test, alpha blend, im3d rendering, camera clear behavior, fog behavior, and texture filters.

2. **Texture upload on raster creation/copy**
   `rw_raster_from_image()` currently performs CPU copy only. Texture upload happens lazily on first render. If adding eager upload, only do it when a GL context/backend is available; CPU-only tests must not require GL. Preserve lazy upload as the safe fallback.

3. **Optional focused tests**
   Consider small tests for alpha test/discard, alpha blending, im3d lit/unlit rendering, and `rw_camera_clear()` pixel behavior. Keep GLFW confined to tests.

## Current Known Gaps

- `rw_raster_from_image()` does CPU copy only; GL upload happens lazily in the render loop.
- Alpha test, alpha blend, im3d rendering, and camera clear behavior do not yet have focused GL pixel tests.
- Final line count target is approximate and currently over the original estimate.

## Do Not Do

- Do not copy C++ librw code verbatim.
- Do not introduce C++ or non-C99 constructs.
- Do not make the core runtime depend on GLFW.
- Do not make CPU-only tests require a GL context.
- Do not rewrite broad compatibility layers unless a concrete shipped data/API need appears.
- Do not audit or rewrite `vendor/glfw`, `vendor/glad`, or `vendor/stb` unless the task is specifically about third-party code.
- Do not undo unrelated user or agent changes in the worktree.

## Before Handing Off Again

- Run `make test` from `/home/dvh/Downloads/librw/new_rw`.
- Update `PROGRESS.md` with completed work and verification status.
- Update this file so it remains an accurate prompt for the next session.
- Mention intentional uncommitted files and any known test/environment caveats.
