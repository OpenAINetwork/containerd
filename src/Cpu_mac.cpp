#include <pthread.h>
#include <sched.h>
#include <unistd.h>


#include "Cpu.h"


void Cpu::init()
{
#   ifdef containerd_NO_LIBCPUID
    m_totalThreads = sysconf(_SC_NPROCESSORS_CONF);
#   endif

    initCommon();
}


void Cpu::setAffinity(int id, uint64_t mask)
{
}
