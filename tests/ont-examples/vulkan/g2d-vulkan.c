
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <user-onx-vk.h>
#include <linmath-plus.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onn.h>

#include <onex-kernel/log.h>

#include <mathlib.h>

// ---------------------------------

uint32_t num_panels;

mat4x4 model_matrix[MAX_PANELS];

typedef struct panel {

  vec3  dimensions;
  vec3  position;
  vec3  rotation;

} panel;

panel welcome_banner ={
 .dimensions = { 1.0f, 0.4f, 0.03f },
 .position   = { 3.0f, 2.0f, -2.0f },
 .rotation   = { 0.0f, 0.0f, 0.0f },
};

static float    vertex_buffer_data[MAX_PANELS*6*6*3];
static uint32_t vertex_buffer_end=0;

static float    uv_buffer_data[MAX_PANELS*6*6*2];
static uint32_t uv_buffer_end=0;

static void make_box(vec3 dimensions){

  float w=dimensions[0];
  float h=dimensions[1];
  float d=dimensions[2];

  float verts[6*6*3] = {

    -w, -h,  d,  // -X side
    -w, -h,  0,
    -w,  h,  0,

    -w,  h,  0,
    -w,  h,  d,
    -w, -h,  d,

    -w, -h,  d,  // -Z side
     w,  h,  d,
     w, -h,  d,

    -w, -h,  d,
    -w,  h,  d,
     w,  h,  d,

    -w, -h,  d,  // -Y side
     w, -h,  d,
     w, -h,  0,

    -w, -h,  d,
     w, -h,  0,
    -w, -h,  0,

    -w,  h,  d,  // +Y side
    -w,  h,  0,
     w,  h,  0,

    -w,  h,  d,
     w,  h,  0,
     w,  h,  d,

     w,  h,  d,  // +X side
     w,  h,  0,
     w, -h,  0,

     w, -h,  0,
     w, -h,  d,
     w,  h,  d,

    -w,  h,  0,  // +Z side
    -w, -h,  0,
     w,  h,  0,

    -w, -h,  0,
     w, -h,  0,
     w,  h,  0,
  };

  memcpy((void*)vertex_buffer_data + vertex_buffer_end, (const void*)verts, sizeof(verts));

  vertex_buffer_end += sizeof(verts);

  float uvs[6*6*2] = {

    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,

    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,

    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,

    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
  };

  memcpy((void*)uv_buffer_data + uv_buffer_end, (const void*)uvs, sizeof(uvs));

  uv_buffer_end += sizeof(uvs);
}

static void add_panel(panel* panel, int p){

    make_box(panel->dimensions);

    mat4x4_translation(model_matrix[p], panel->position[0],
                                        panel->position[1],
                                        panel->position[2]);
    mat4x4 mm;
    mat4x4_rotate_X(mm, model_matrix[p], (float)degreesToRadians(panel->rotation[0]));
    mat4x4_rotate_Y(model_matrix[p], mm, (float)degreesToRadians(panel->rotation[1]));
    mat4x4_rotate_Z(mm, model_matrix[p], (float)degreesToRadians(panel->rotation[2]));
    mat4x4_orthonormalize(model_matrix[p], mm);
}

static float* vertices;

// -----------------------------------------

void g2d_init() {
}

bool g2d_clear_screen(uint8_t colour) {

  if(!prepared) return false;

  set_up_scene_begin(&vertices);

  vertex_buffer_end = 0;
  uv_buffer_end = 0;

  num_panels = 0;

  return true;
}

void g2d_render() {

  if(!prepared){
    log_write("g2d_render() called when not prepared!\n");
    return;
  }

  add_panel(&welcome_banner, num_panels++);

  for(unsigned int i = 0; i < num_panels * 6*6; i++) {
    *(vertices+i*5+0) = vertex_buffer_data[i*3+0];
    *(vertices+i*5+1) = vertex_buffer_data[i*3+1];
    *(vertices+i*5+2) = vertex_buffer_data[i*3+2];
    *(vertices+i*5+3) = uv_buffer_data[i*2+0];
    *(vertices+i*5+4) = uv_buffer_data[i*2+1];
  }
  set_up_scene_end();
}




