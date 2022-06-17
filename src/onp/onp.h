#ifndef ONP_H
#define ONP_H

#include <onn.h>

void onp_init();
bool onp_loop();
void onp_send_observe(char* uid, char* channel);
void onp_send_object(object* o, char* channel);

#endif
