// ------- common/shared between user and vk code -------------

#include <linmath-plus.h>

#include <onl-vk.h>

#define MAX_PANELS 32 // TODO set src/ont/unix/onx.vert

extern mat4x4 proj_matrix;
extern mat4x4 view_l_matrix;
extern mat4x4 view_r_matrix;
extern mat4x4 model_matrix[MAX_PANELS];
extern float  left_touch_vec[2];

void set_proj_view();
bool set_up_scene_begin(float** vertices);
void set_up_scene_end();

