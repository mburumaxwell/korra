#ifndef SYS_REBOOT_H
#define SYS_REBOOT_H

#include <esp_system.h>

/** Reboot because something has failed. */
inline __attribute__((noreturn)) void sys_reboot() {
  esp_restart();
}

#endif // SYS_REBOOT_H
