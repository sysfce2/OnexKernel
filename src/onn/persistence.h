
#include <onn.h>

extern properties* objects_text; // XXX

void  persistence_init(char* filename);
bool  persistence_loop();
char* persistence_get(char* uid);
void  persistence_put(object* o);
void  persistence_flush();


