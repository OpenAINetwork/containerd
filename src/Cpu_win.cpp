#include <windows.h>


#include "Cpu.h"


void Cpu::init()
{
#   ifdef containerd_NO_LIBCPUID
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    m_totalThreads = sysinfo.dwNumberOfProcessors;
#   endif

    initCommon();
}


void Cpu::setAffinity(int id, uint64_t mask)
{
    if (id == -1) {
        SetProcessAffinityMask(GetCurrentProcess(), mask);
    }
    else {
        SetThreadAffinityMask(GetCurrentThread(), mask);
    }
}
