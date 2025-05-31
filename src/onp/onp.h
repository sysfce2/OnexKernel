#ifndef ONP_H
#define ONP_H

#include <onn.h>

typedef struct {
  char* uid;
  char* dev;
} observe;

void onp_init(properties* config);
bool onp_loop();
void onp_send_observe(char* uid, char* devices);
void onp_send_object( char* uid, char* devices);
void onn_recv_observe(observe* obs);
void onn_recv_object(object* n);

observe* observe_from_text(char* u);
void     observe_free(observe* obs);

#endif
