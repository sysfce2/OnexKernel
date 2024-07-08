// ------- common/shared between user and vk code -------------

#include <pthread.h>
#include <stdbool.h>

#include "onl/onl.h"
#include <linmath-plus.h>

extern bool prepared;

#define MAX_PANELS 32 // TODO set src/ont/unix/onx.vert
extern mat4x4 proj_matrix;
extern mat4x4 view_matrix;
extern mat4x4 model_matrix[MAX_PANELS];

void set_up_scene_begin(void** vertices);
void set_up_scene_end();

void set_mvp_uniforms();

