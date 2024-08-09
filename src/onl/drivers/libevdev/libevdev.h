#ifndef ONL_LIBEVDEV_H
#define ONL_LIBEVDEV_H

typedef void (*iostate_change_cb_t)();

int  libevdev_init(iostate_change_cb_t cb);
void libevdev_process_events();
void libevdev_end();

#endif

