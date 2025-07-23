
#include <onn.h>

list* persistence_init(properties* config);
list* persistence_reload();

char* persistence_get(char* uid);
void  persistence_put(char* uid, uint32_t ver, char* text);

void  persistence_show_db();


