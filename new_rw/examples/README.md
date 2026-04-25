# rw Examples

These are visible GLFW demos for the current C99/GLES2 runtime. The primitive and lighting demos are C ports of the original librw `tools/` examples, adjusted for the smaller `new_rw` API.

Build them with:

```sh
make examples
```

Run them with:

```sh
build/bin/example_city_night
build/bin/example_im2d_primitives
build/bin/example_im3d_primitives
build/bin/example_lights_room
build/bin/example_immediate_playground
build/bin/example_skin_dancer
```

Set `RW_EXAMPLE_FRAMES=N` to run a demo hidden for `N` frames, useful for smoke checks:

```sh
RW_EXAMPLE_FRAMES=2 build/bin/example_city_night
```

## Demos

- `city_night` shows an Im2D-only neon skyline and moving train.
- `im2d_primitives` ports the original Im2D primitive test patterns: line list, polyline, tri list, tri strip, tri fan, and indexed triangles.
- `im3d_primitives` ports the original Im3D primitive test patterns into a simple rotating primitive gallery.
- `lights_room` ports the original lights-room idea with checker panels and a moving point light.
- `immediate_playground` shows a single clear Im3D indexed triangle fan.
- `skin_dancer` shows the skin pipeline, HAnim matrix updates, frame motion, fog, and point lighting.
