#ifndef ONL_LIBINPUT_H
#define ONL_LIBINPUT_H

typedef void (*iostate_change_cb_t)();

int  libinput_init(iostate_change_cb_t cb);
void libinput_process_events();
void libinput_end();

#endif

