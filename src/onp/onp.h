#ifndef ONP_H
#define ONP_H

#include <onn.h>

void onp_init(properties* config);
bool onp_loop();
void onp_send_observe(char* uid, char* devices);
void onp_send_object(object* o, char* devices);

#endif
