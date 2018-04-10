#ifdef __FreeBSD__
#   include <sys/types.h>
#   include <sys/param.h>
#   include <sys/cpuset.h>
#   include <pthread_np.h>
#endif


#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>


#include "Cpu.h"


#ifdef __FreeBSD__
typedef cpuset_t cpu_set_t;
#endif


void Cpu::init()
{
#   ifdef containerd_NO_LIBCPUID
    m_totalThreads = sysconf(_SC_NPROCESSORS_CONF);
#   endif

    initCommon();
}


void Cpu::setAffinity(int id, uint64_t mask)
{
    cpu_set_t set;
    CPU_ZERO(&set);

    for (int i = 0; i < m_totalThreads; i++) {
        if (mask & (1UL << i)) {
            CPU_SET(i, &set);
        }
    }

    if (id == -1) {
#       ifndef __FreeBSD__
        sched_setaffinity(0, sizeof(&set), &set);
#       endif
    } else {
#       ifndef __ANDROID__
        pthread_setaffinity_np(pthread_self(), sizeof(&set), &set);
#       else
        sched_setaffinity(gettid(), sizeof(&set), &set);
#       endif
    }
}
