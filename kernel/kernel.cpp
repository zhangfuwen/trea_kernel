#include "kernel/kernel.h"
#include "arch/x86/paging.h"
#include "lib/debug.h"

Kernel::Kernel() : memory_manager() {
  debug_debug("Kernel::Kernel()");
}

void Kernel::init() {
  debug_debug("Kernel::init()");
    memory_manager.init();

}
Kernel Kernel::instance0;
