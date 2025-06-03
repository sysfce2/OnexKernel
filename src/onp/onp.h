#ifndef ONP_H
#define ONP_H

#include <onn.h>

void onp_init(properties* config);
bool onp_loop();
void onp_send_observe(char* uid, char* devices);
void onp_send_object( char* uid, char* devices);
void onn_recv_observe(observe obs);
void onn_recv_object(object* n);

object* object_from_text(char* text, bool need_uid_ver, uint8_t max_size);

#endif
