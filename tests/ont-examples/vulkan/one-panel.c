
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdbool.h>

#include <linmath-plus.h>
#include <mathlib.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include <onl.h>
#include <onn.h>

#include "user-onx-vk.h"

extern bool prepared; // FIXME

mat4x4 proj_matrix;
mat4x4 view_l_matrix;
mat4x4 view_r_matrix;
mat4x4 model_matrix[MAX_PANELS];

uint32_t num_panels;

typedef struct panel {

  vec3  dimensions;
  vec3  position;
  vec3  rotation;

} panel;

panel the_cube ={
 .dimensions = { 0.4f, 0.4f, 0.4f },
 .position   = { 0.0f, 0.4f, 0.0f },
 .rotation   = { 0.0f, 0.0f, 0.0f },
};

static float    vertex_buffer_data[MAX_PANELS*6*6*3];
static uint32_t vertex_buffer_end=0;

static float    uv_buffer_data[MAX_PANELS*6*6*2];
static uint32_t uv_buffer_end=0;

static void make_box(vec3 dimensions){

  float w=dimensions[0];
  float h=dimensions[1];
  float d=dimensions[2]*2;

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

static void add_cube(panel* panel, int p){

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

void one_panel_render() {

  if(!prepared){
    log_write("one_panel_render() called when not prepared!\n");
    return;
  }

  set_up_scene_begin(&vertices);

  vertex_buffer_end = 0;
  uv_buffer_end = 0;

  num_panels = 0;
  add_cube(&the_cube, num_panels++);

  for(unsigned int i = 0; i < num_panels * 6*6; i++) {
    *(vertices+i*5+0) = vertex_buffer_data[i*3+0];
    *(vertices+i*5+1) = vertex_buffer_data[i*3+1];
    *(vertices+i*5+2) = vertex_buffer_data[i*3+2];
    *(vertices+i*5+3) = uv_buffer_data[i*2+0];
    *(vertices+i*5+4) = uv_buffer_data[i*2+1];
  }
  set_up_scene_end();
}

// -----------------------------------------------------------

static const float eye_sep = 0.020; // eyes off-centre dist in m
static const float eye_con = 0.020; // angle of convergence rads

// 1.75m height
// standing back 5m from origin
static vec3  eye_l = { -eye_sep, 1.75, -5.0 };
static vec3  eye_r = {  eye_sep, 1.75, -5.0 };
static vec3  up = { 0.0f, -1.0, 0.0 };

static float eye_dir=0;
static float head_hor_dir=0;
static float head_ver_dir=0;

void set_mvp_uniforms() {

    #define VIEWPORT_FOV   43.0f
    #define VIEWPORT_NEAR   0.1f
    #define VIEWPORT_FAR  100.0f

    float ar = aspect_ratio / (multiview? 2.0f: 1.0f);

    Mat4x4_perspective(proj_matrix,
                       (float)degreesToRadians(VIEWPORT_FOV),
                       ar,
                       VIEWPORT_NEAR, VIEWPORT_FAR);

    proj_matrix[1][1] *= -1;

    vec3 looking_at_l;
    vec3 looking_at_r;

    looking_at_l[0] = eye_l[0] + 100.0f * sin(eye_dir + eye_con + head_hor_dir);
    looking_at_l[1] = eye_l[1] - 100.0f * sin(                    head_ver_dir);
    looking_at_l[2] = eye_l[2] + 100.0f * cos(eye_dir + eye_con + head_hor_dir);

    looking_at_r[0] = eye_r[0] + 100.0f * sin(eye_dir - eye_con + head_hor_dir);
    looking_at_r[1] = eye_r[1] - 100.0f * sin(                    head_ver_dir);
    looking_at_r[2] = eye_r[2] + 100.0f * cos(eye_dir - eye_con + head_hor_dir);

    mat4x4_look_at(view_l_matrix, eye_l, looking_at_l, up);
    mat4x4_look_at(view_r_matrix, eye_r, looking_at_r, up);
}

static bool     head_moving=false;
static bool     body_moving=false;
static uint32_t x_on_press;
static uint32_t y_on_press;

static float dwell(float delta, float width){
  return delta > 0? max(delta - width, 0.0f):
                    min(delta + width, 0.0f);
}

void onx_iostate_changed() {
  /*
  log_write("onx_iostate_changed [%d,%d] @(%d %d) buttons=(%d %d %d) key=%d\n",
           io.swap_width, io.swap_height,
           io.mouse_x, io.mouse_y,
           io.left_pressed, io.middle_pressed, io.right_pressed,
           io.key);
  */
  bool bottom_left = io.mouse_x < io.swap_width / 3 && io.mouse_y > io.swap_height / 2;

  if(io.left_pressed && !body_moving && bottom_left){
    body_moving=true;

    x_on_press = io.mouse_x;
    y_on_press = io.mouse_y;
  }
  else
  if(io.left_pressed && body_moving){

    float delta_x =  0.00007f * ((int32_t)io.mouse_x - (int32_t)x_on_press);
    float delta_y = -0.00007f * ((int32_t)io.mouse_y - (int32_t)y_on_press);

    delta_x = dwell(delta_x, 0.0015f);
    delta_y = dwell(delta_y, 0.0015f);

    eye_dir += 0.5f* delta_x;

    eye_l[0] += 4.0f * delta_y * sin(eye_dir);
    eye_l[2] += 4.0f * delta_y * cos(eye_dir);
    eye_r[0] += 4.0f * delta_y * sin(eye_dir);
    eye_r[2] += 4.0f * delta_y * cos(eye_dir);
  }
  else
  if(!io.left_pressed && body_moving){
    body_moving=false;
  }
  else
  if(io.left_pressed && !head_moving){

    head_moving=true;

    x_on_press = io.mouse_x;
    y_on_press = io.mouse_y;
  }
  else
  if(io.left_pressed && head_moving){

    float delta_x = 0.00007f * ((int32_t)io.mouse_x - (int32_t)x_on_press);
    float delta_y = 0.00007f * ((int32_t)io.mouse_y - (int32_t)y_on_press);

    head_hor_dir = 35.0f*dwell(delta_x, 0.0015f);
    head_ver_dir = 35.0f*dwell(delta_y, 0.0015f);
  }
  else
  if(!io.left_pressed && head_moving){

    head_moving=false;

    head_hor_dir=0;
    head_ver_dir=0;
  }
}

// ---------------------------------

void onx_init(){
  one_panel_render(); // static scene! this can be done per frame
}

// ---------------------------------

static void show_matrix(mat4x4 m){
  log_write("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++) log_write("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  log_write("\\---------------------/\n");
}

// --------------------------------------------------------------------------------------




