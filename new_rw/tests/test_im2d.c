#include "../rw.h"
#include <GLFW/glfw3.h>
#include <glad/gles2.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void expect_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  uint8_t p[4];
  glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
  fprintf(stderr, "pixel(%d,%d): R=%d G=%d B=%d A=%d\n", x, y, p[0], p[1], p[2],
          p[3]);
  assert(p[0] == r);
  assert(p[1] == g);
  assert(p[2] == b);
}

static void test_im2d_quads(void) {
  GLFWwindow *win;
  GLenum err;
  int w = 128, h = 128;
  RwIm2DVertex red[6] = {
      {32.0f, 32.0f, 0.0f, 1.0f, 255, 0, 0, 255, 0.0f, 0.0f},
      {96.0f, 32.0f, 0.0f, 1.0f, 255, 0, 0, 255, 1.0f, 0.0f},
      {32.0f, 96.0f, 0.0f, 1.0f, 255, 0, 0, 255, 0.0f, 1.0f},
      {96.0f, 32.0f, 0.0f, 1.0f, 255, 0, 0, 255, 1.0f, 0.0f},
      {96.0f, 96.0f, 0.0f, 1.0f, 255, 0, 0, 255, 1.0f, 1.0f},
      {32.0f, 96.0f, 0.0f, 1.0f, 255, 0, 0, 255, 0.0f, 1.0f},
  };
  RwIm2DVertex green[4] = {
      {48.0f, 48.0f, 0.0f, 1.0f, 0, 255, 0, 255, 0.0f, 0.0f},
      {80.0f, 48.0f, 0.0f, 1.0f, 0, 255, 0, 255, 1.0f, 0.0f},
      {80.0f, 80.0f, 0.0f, 1.0f, 0, 255, 0, 255, 1.0f, 1.0f},
      {48.0f, 80.0f, 0.0f, 1.0f, 0, 255, 0, 255, 0.0f, 1.0f},
  };
  RwIm2DVertex white[4] = {
      {48.0f, 48.0f, 0.0f, 1.0f, 255, 255, 255, 255, 0.0f, 0.0f},
      {80.0f, 48.0f, 0.0f, 1.0f, 255, 255, 255, 255, 1.0f, 0.0f},
      {80.0f, 80.0f, 0.0f, 1.0f, 255, 255, 255, 255, 1.0f, 1.0f},
      {48.0f, 80.0f, 0.0f, 1.0f, 255, 255, 255, 255, 0.0f, 1.0f},
  };
  uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
  RwRaster *tex_raster;
  uint8_t *tex_pixels;

  win = glfwCreateWindow(w, h, "test_im2d", NULL, NULL);
  assert(win);
  glfwMakeContextCurrent(win);
  gladLoadGLES2(glfwGetProcAddress);

  assert(rw_engine_init(NULL));
  assert(rw_engine_open());
  assert(rw_engine_start());

  glViewport(0, 0, w, h);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  rw_set_render_state(RW_STATE_CULLMODE, RW_CULL_NONE);
  rw_im2d_render_primitive(RW_PRIM_TRILIST, red, 6);
  expect_pixel(w / 2, h / 2, 255, 0, 0);

  rw_im2d_render_indexed(RW_PRIM_TRILIST, green, 4, indices, 6);
  expect_pixel(w / 2, h / 2, 0, 255, 0);

  tex_raster = rw_raster_create(1, 1, 32, 0);
  assert(tex_raster);
  tex_pixels = rw_raster_lock(tex_raster);
  assert(tex_pixels);
  tex_pixels[0] = 0; tex_pixels[1] = 0; tex_pixels[2] = 255; tex_pixels[3] = 255;
  rw_raster_unlock(tex_raster);
  rw_set_render_state_ptr(RW_STATE_TEXTURERASTER, tex_raster);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  rw_im2d_render_indexed(RW_PRIM_TRILIST, white, 4, indices, 6);
  expect_pixel(w / 2, h / 2, 0, 0, 255);

  tex_pixels = rw_raster_lock(tex_raster);
  tex_pixels[0] = 255; tex_pixels[1] = 0; tex_pixels[2] = 0; tex_pixels[3] = 255;
  rw_raster_unlock(tex_raster);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  rw_im2d_render_indexed(RW_PRIM_TRILIST, white, 4, indices, 6);
  expect_pixel(w / 2, h / 2, 255, 0, 0);
  rw_set_render_state_ptr(RW_STATE_TEXTURERASTER, NULL);
  rw_raster_destroy(tex_raster);

  err = glGetError();
  fprintf(stderr, "GL error after im2d render: 0x%x\n", err);
  assert(err == GL_NO_ERROR);

  rw_engine_stop();
  rw_engine_close();
  rw_engine_term();

  glfwDestroyWindow(win);
  fprintf(stderr, "test_im2d_quads: PASS\n");
}

int main(void) {
  assert(glfwInit());
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  test_im2d_quads();

  glfwTerminate();
  return 0;
}
