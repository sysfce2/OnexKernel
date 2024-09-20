
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

#include "user-onl-vk.h"

#include <onn.h>

mat4x4 proj_matrix;
mat4x4 view_l_matrix;
mat4x4 view_r_matrix;
mat4x4 model_matrix[MAX_PANELS];
float  left_touch_vec[] = { 0.25, 0.75 };

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

// 1.75m height
// standing back 5m from origin
static vec3  body_pos = { 0, 1.75, -5.0 }; // 1.75 is still nose bridge pos
static float body_dir = 0.6852;
static float body_xv = 0.0f;
static float body_zv = 0.0f;

static float head_hor_dir=0;
static float head_ver_dir=0;

static const float eye_sep = 0.030; // eyes off-centre dist in m
static const float eye_con = 0.000; // angle of convergence rads

static vec3  up = { 0.0f, -1.0, 0.0 };

void set_proj_view() {

    body_pos[0] += body_xv;
    body_pos[2] += body_zv;

    #define VIEWPORT_FOV   21.5f
    #define VIEWPORT_NEAR   0.1f
    #define VIEWPORT_FAR  100.0f

    Mat4x4_perspective(proj_matrix,
                       (float)degreesToRadians(VIEWPORT_FOV),
                       onl_vk_aspect_ratio_proj,
                       VIEWPORT_NEAR, VIEWPORT_FAR);

    proj_matrix[1][1] *= -1;

    vec3 looking_at_l;
    vec3 looking_at_r;
    vec3 eye_l;
    vec3 eye_r;

    eye_l[0]=body_pos[0]-eye_sep;
    eye_l[1]=body_pos[1]        ;
    eye_l[2]=body_pos[2]        ;

    eye_r[0]=body_pos[0]+eye_sep;
    eye_r[1]=body_pos[1]        ;
    eye_r[2]=body_pos[2]        ;

    looking_at_l[0] = eye_l[0] + 100.0f * sin(body_dir + head_hor_dir + eye_con);
    looking_at_l[1] = eye_l[1] - 100.0f * sin(           head_ver_dir          );
    looking_at_l[2] = eye_l[2] + 100.0f * cos(body_dir + head_hor_dir + eye_con);

    looking_at_r[0] = eye_r[0] + 100.0f * sin(body_dir + head_hor_dir - eye_con);
    looking_at_r[1] = eye_r[1] - 100.0f * sin(           head_ver_dir          );
    looking_at_r[2] = eye_r[2] + 100.0f * cos(body_dir + head_hor_dir - eye_con);

    mat4x4_look_at(view_l_matrix, eye_l, looking_at_l, up);
    mat4x4_look_at(view_r_matrix, eye_r, looking_at_r, up);
}

static float dwell(float delta, float width){
  return delta > 0? max(delta - width, 0.0f):
                    min(delta + width, 0.0f);
}

static uint32_t x_on_press;
static uint32_t y_on_press;

static bool     head_moving=false;
static bool     body_moving=false;

void ont_vk_iostate_changed() {

#define LOG_IO
#ifdef LOG_IO
  log_write("D-pad=(%d %d %d %d) head=(%f %f %f) "
            "joy 1=(%f %f) joy 2=(%f %f) touch=(%d %d) "
            "mouse pos=(%d %d %d) mouse buttons=(%d %d %d) key=%d\n",
                   io.d_pad_left, io.d_pad_right, io.d_pad_up, io.d_pad_down,
                   io.yaw, io.pitch, io.roll,
                   io.joy_1_lr, io.joy_1_ud, io.joy_2_lr, io.joy_2_ud,
                   io.touch_x, io.touch_y,
                   io.mouse_x, io.mouse_y, io.mouse_scroll,
                   io.mouse_left, io.mouse_middle, io.mouse_right,
                   io.key);
#endif

  float sd = sin(body_dir);
  float cd = cos(body_dir);
  float sp = 0.02f;

  body_xv = 0.0f; body_zv = 0.0f;

  if(io.d_pad_up){
    body_xv +=  sp * sd;
    body_zv +=  sp * cd;
  }
  if(io.d_pad_down){
    body_xv += -sp * sd;
    body_zv += -sp * cd;
  }
  if(io.d_pad_right){
    body_xv +=  sp * cd;
    body_zv += -sp * sd;
  }
  if(io.d_pad_left){
    body_xv += -sp * cd;
    body_zv +=  sp * sd;
  }

  float bd = 3.1415926/200 * io.joy_2_lr;
  body_dir += bd;

  bool bottom_left = io.touch_x < swap_width / 3 && io.touch_y > swap_height / 2;

  left_touch_vec[0]=0.25;
  left_touch_vec[1]=0.75;

  if(io.touch_x==0 && io.touch_y==0) {
    body_moving=false;
    head_moving=false;
  }
  else
  if(bottom_left || body_moving){

    left_touch_vec[0] = (float)io.touch_x / (float)swap_width * onl_vk_aspect_ratio_proj;
    left_touch_vec[1] = (float)io.touch_y / (float)swap_height;

    if(!body_moving){
      body_moving = true;
    }
    float x =  (float)io.touch_x                  / (swap_width  / 3);
    float y = ((float)io.touch_y-(swap_height/2)) / (swap_height / 2);

    if(x>0.333 && x< 0.666 && y < 0.333){
      body_xv +=  sp * sd;
      body_zv +=  sp * cd;
    }
    if(x>0.333 && x< 0.666 && y > 0.666){
      body_xv += -sp * sd;
      body_zv += -sp * cd;
    }
    if(x>0.666 && y > 0.333 && y < 0.666){
      body_xv +=  sp * cd;
      body_zv += -sp * sd;
    }
    if(x<0.333 && y > 0.333 && y < 0.666){
      body_xv += -sp * cd;
      body_zv +=  sp * sd;
    }
  }
  else
  if(io.touch_x && io.touch_y){
    if(!head_moving){
      head_moving=true;
      x_on_press=io.touch_x;
      y_on_press=io.touch_y;
    }
    float mx=(float)((int16_t)io.touch_x-(int16_t)x_on_press);
    float my=(float)((int16_t)io.touch_y-(int16_t)y_on_press);
    float dx=mx/swap_width *3.1415926/64;
    float dy=my/swap_height*3.1415926/64;
    printf("mx=%f my=%f dx=%f dy=%f\n", mx, my, dx, dy);
    head_hor_dir+=dx;
    head_ver_dir+=dy;
  }
  head_hor_dir+=io.yaw;
  head_ver_dir+=io.pitch;

/*
  bool bottom_left = io.mouse_x < swap_width / 3 && io.mouse_y > swap_height / 2;

  if(io.mouse_left && !body_moving && bottom_left){
    body_moving=true;

    x_on_press = io.mouse_x;
    y_on_press = io.mouse_y;
  }
  else
  if(io.mouse_left && body_moving){

    float delta_x =  0.00007f * ((int32_t)io.mouse_x - (int32_t)x_on_press);
    float delta_y = -0.00007f * ((int32_t)io.mouse_y - (int32_t)y_on_press);

    delta_x = dwell(delta_x, 0.0015f);
    delta_y = dwell(delta_y, 0.0015f);

    body_dir += 0.5f* delta_x;

    eye_l[0] += 4.0f * delta_y * sin(body_dir);
    eye_l[2] += 4.0f * delta_y * cos(body_dir);
    eye_r[0] += 4.0f * delta_y * sin(body_dir);
    eye_r[2] += 4.0f * delta_y * cos(body_dir);
  }
  else
  if(!io.mouse_left && body_moving){
    body_moving=false;
  }
  else
  if(io.mouse_left && !head_moving){

    head_moving=true;

    x_on_press = io.mouse_x;
    y_on_press = io.mouse_y;
  }
  else
  if(io.mouse_left && head_moving){

    float delta_x = 0.00007f * ((int32_t)io.mouse_x - (int32_t)x_on_press);
    float delta_y = 0.00007f * ((int32_t)io.mouse_y - (int32_t)y_on_press);

    head_hor_dir = 35.0f*dwell(delta_x, 0.0015f);
    head_ver_dir = 35.0f*dwell(delta_y, 0.0015f);
  }
  else
  if(!io.mouse_left && head_moving){

    head_moving=false;

    head_hor_dir=0;
    head_ver_dir=0;
  }
*/
}

// ---------------------------------

void ont_vk_init(){
  one_panel_render(); // static scene! this can be done per frame
}

// ---------------------------------

static void show_matrix(mat4x4 m){
  log_write("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++) log_write("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  log_write("\\---------------------/\n");
}

// --------------------------------------------------------------------------------------




