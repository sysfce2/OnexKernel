#ifndef ONP_H
#define ONP_H

#include <onf.h>

void onp_init();
void onp_loop();
void onp_send_observe(char* uid, char* to);
void onp_send_object(object* o, char* to);

#endif