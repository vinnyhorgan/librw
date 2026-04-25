#ifndef RW_EXAMPLE_COMMON_H
#define RW_EXAMPLE_COMMON_H

#include "../rw.h"
#include "../rw_gl_internal.h"
#include <GLFW/glfw3.h>

typedef struct ExApp {
    GLFWwindow *window;
    RwCamera *camera;
    RwFrame *camera_frame;
    int width;
    int height;
    int max_frames;
    int frame;
} ExApp;

int  ex_app_init(ExApp *app, const char *title, int width, int height);
void ex_app_begin_frame(ExApp *app);
int  ex_app_running(ExApp *app);
void ex_app_swap(ExApp *app);
void ex_app_shutdown(ExApp *app);

RwRaster *ex_make_checker_raster(int w, int h, RwRGBA a, RwRGBA b);
RwAtomic *ex_make_quad(float x0, float y0, float x1, float y1, float z,
                       RwRGBA color, RwRaster *raster, RwFrame **frame_out);
RwLight  *ex_make_light(RwLightType type, RwRGBAf color, RwV3d pos, float radius,
                        RwFrame **frame_out);

#endif
