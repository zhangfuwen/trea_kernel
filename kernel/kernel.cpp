#include "kernel/kernel.h"

#include <lib/serial.h>

#include "arch/x86/paging.h"
#include "lib/debug.h"

Kernel::Kernel() : memory_manager() {
  debug_debug("Kernel::Kernel()");
}

void Kernel::init() {
    serial_puts("kernel init\n");
    memory_manager.init();

}
Kernel Kernel::instance0;
