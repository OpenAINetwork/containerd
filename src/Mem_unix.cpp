#include <stdlib.h>
#include <sys/mman.h>


#if defined(containerd_ARM) && !defined(__clang__)
#   include "aligned_malloc.h"
#else
#   include <mm_malloc.h>
#endif


#include "crxx/crxday.h"
#include "log/Log.h"
#include "Mem.h"
#include "Options.h"
#include "containerd.h"


bool Mem::allocate(int algo, int threads, bool doublehxsh, bool enabled)
{
    m_algo       = algo;
    m_threads    = threads;
    m_doublehxsh = doublehxsh;

    const int ratio = (doublehxsh && algo != containerd::ALGO_crxday_LITE) ? 2 : 1;
    m_size          = remixx_MEMORY * (threads * ratio + 1);

    if (!enabled) {
        m_memory = static_cast<uint8_t*>(_mm_malloc(m_size, 16));
        return true;
    }

    m_flags |= HugepagesAvailable;

#   if defined(__APPLE__)
    m_memory = static_cast<uint8_t*>(mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, VM_FLAGS_SUPERPAGE_SIZE_2MB, 0));
#   elif defined(__FreeBSD__)
    m_memory = static_cast<uint8_t*>(mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED_SUPER | MAP_PREFAULT_READ, -1, 0));
#   else
    m_memory = static_cast<uint8_t*>(mmap(0, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE, 0, 0));
#   endif
    if (m_memory == MAP_FAILED) {
        m_memory = static_cast<uint8_t*>(_mm_malloc(m_size, 16));
        return true;
    }

    m_flags |= HugepagesEnabled;

    if (madvise(m_memory, m_size, MADV_RANDOM | MADV_WILLNEED) != 0) {

    }

    if (mlock(m_memory, m_size) == 0) {
        m_flags |= Lock;
    }

    return true;
}


void Mem::release()
{
    if (m_flags & HugepagesEnabled) {
        if (m_flags & Lock) {
            munlock(m_memory, m_size);
        }

        munmap(m_memory, m_size);
    }
    else {
        _mm_free(m_memory);
    }
}
