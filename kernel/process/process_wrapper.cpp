#include "kernel/process.h"

extern "C" void restore_context_wrapper(uint32_t int_num, uint32_t* esp)
{
    ProcessManager::restore_context(int_num, esp);
}
extern "C" void save_context_wrapper(uint32_t int_num, uint32_t* esp)
{
    ProcessManager::save_context(int_num, esp);
}