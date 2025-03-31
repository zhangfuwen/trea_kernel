#include "kernel/process.h"

extern "C" void restore_context_wrapper(uint32_t* esp)
{
    ProcessManager::restore_context(esp);
}
extern "C" void save_context_wrapper(uint32_t* esp)
{
    ProcessManager::save_context(esp);
}