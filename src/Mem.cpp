#include <memory.h>
#include "crxx/crxday.h"
#include "Mem.h"
#include "Options.h"
#include "containerd.h"


bool Mem::m_doublehxsh             = false;
int Mem::m_algo                    = 0;
int Mem::m_flags                   = 0;
int Mem::m_threads                 = 0;
size_t Mem::m_offset               = 0;
size_t Mem::m_size                 = 0;
alignas(16) uint8_t *Mem::m_memory = nullptr;


crxday_ctx *Mem::create(int threadId)
{
#   ifndef containerd_NO_noway
    if (m_algo == containerd::ALGO_crxday_LITE) {
        return createLite(threadId);
    }
#   endif

    crxday_ctx *ctx = reinterpret_cast<crxday_ctx *>(&m_memory[remixx_MEMORY - sizeof(crxday_ctx) * (threadId + 1)]);

    const int ratio = m_doublehxsh ? 2 : 1;
    ctx->memory = &m_memory[remixx_MEMORY * (threadId * ratio + 1)];

    return ctx;
}



void *Mem::calloc(size_t num, size_t size)
{
    void *mem = &m_memory[m_offset];
    m_offset += (num * size);

    memset(mem, 0, num * size);

    return mem;
}


#ifndef containerd_NO_noway
crxday_ctx *Mem::createLite(int threadId) {
    crxday_ctx *ctx;

    if (!m_doublehxsh) {
        const size_t offset = remixx_MEMORY * (threadId + 1);

        ctx = reinterpret_cast<crxday_ctx *>(&m_memory[offset + noway_MEMORY]);
        ctx->memory = &m_memory[offset];
        return ctx;
    }

    ctx = reinterpret_cast<crxday_ctx *>(&m_memory[remixx_MEMORY - sizeof(crxday_ctx) * (threadId + 1)]);
    ctx->memory = &m_memory[remixx_MEMORY * (threadId + 1)];

    return ctx;
}
#endif
