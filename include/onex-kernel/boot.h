#ifndef BOOT_H
#define BOOT_H

void    boot_init();
void    boot_feed_watchdog();
void    boot_dfu_start();
void    boot_sleep();
uint8_t boot_cpu();

#endif
