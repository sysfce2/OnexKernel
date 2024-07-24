
#include <linmath-plus.h>

#define MAX_PANELS 32 // TODO set src/ont/unix/onx.vert

extern mat4x4 proj_matrix;
extern mat4x4 view_l_matrix;
extern mat4x4 view_r_matrix;
extern mat4x4 model_matrix[MAX_PANELS];

extern VkDescriptorSetLayout descriptor_layout;

extern float aspect_ratio_proj;

void set_up_scene_begin(float** vertices);
void set_up_scene_end();

