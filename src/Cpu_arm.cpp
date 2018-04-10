#include <string.h>


#include "Cpu.h"


char Cpu::m_brand[64]   = { 0 };
int Cpu::m_flags        = 0;
int Cpu::m_l2_cache     = 0;
int Cpu::m_l3_cache     = 0;
int Cpu::m_sockets      = 1;
int Cpu::m_totalCores   = 0;
int Cpu::m_totalThreads = 0;


int Cpu::optimalThreadsCount(int algo, bool doublehxsh, int maxCpuUsage)
{
    return m_totalThreads;
}


void Cpu::initCommon()
{
    memcpy(m_brand, "Unknown", 7);

#   if defined(containerd_ARMv8)
    m_flags |= X86_64;
    m_flags |= AES;
#   endif
}
