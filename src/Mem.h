#ifndef __MEM_H__
#define __MEM_H__


#include <stddef.h>
#include <stdint.h>


struct crxday_ctx;


class Mem
{
public:
    enum Flags {
        HugepagesAvailable = 1,
        HugepagesEnabled   = 2,
        Lock               = 4
    };

    static bool allocate(int algo, int threads, bool doublehxsh, bool enabled);
    static crxday_ctx *create(int threadId);
    static void *calloc(size_t num, size_t size);
    static void release();

    static inline bool isDoublehxsh()         { return m_doublehxsh; }
    static inline bool isHugepagesAvailable() { return (m_flags & HugepagesAvailable) != 0; }
    static inline bool isHugepagesEnabled()   { return (m_flags & HugepagesEnabled) != 0; }
    static inline int flags()                 { return m_flags; }
    static inline int threads()               { return m_threads; }

private:
    static bool m_doublehxsh;
    static int m_algo;
    static int m_flags;
    static int m_threads;
    static size_t m_offset;
    static size_t m_size;
    alignas(16) static uint8_t *m_memory;

#   ifndef containerd_NO_noway
    static crxday_ctx *createLite(int threadId);
#   endif
};


#endif