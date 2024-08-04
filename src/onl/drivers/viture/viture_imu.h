#ifndef VITURE_H
#define VITURE_H

typedef void (*head_rotations_cb_t)(uint32_t ts, float yaw, float pitch, float roll);

void viture_init(head_rotations_cb_t cb);
void viture_end();

#endif
