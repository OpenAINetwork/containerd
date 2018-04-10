#ifndef __crxday_remixx_H__
#define __crxday_remixx_H__


// VARIANT ALTERATIONS
#ifndef containerd_ARM
#   define VARIANT1_INIT(part) \
    uint64_t tweak1_2_##part = 0; \
    if (VARIANT > 0) { \
        tweak1_2_##part = (*reinterpret_cast<const uint64_t*>(input + 35 + part * size) ^ \
                          *(reinterpret_cast<const uint64_t*>(ctx->state##part) + 24)); \
    }
#else
#   define VARIANT1_INIT(part) \
    uint64_t tweak1_2_##part = 0; \
    if (VARIANT > 0) { \
        volatile const uint64_t a = *reinterpret_cast<const uint64_t*>(input + 35 + part * size); \
        volatile const uint64_t b = *(reinterpret_cast<const uint64_t*>(ctx->state##part) + 24); \
        tweak1_2_##part = a ^ b; \
    }
#endif

#define VARIANT1_1(p) \
    if (VARIANT > 0) { \
        const uint8_t tmp = reinterpret_cast<const uint8_t*>(p)[11]; \
        static const uint32_t table = 0x75310; \
        const uint8_t index = (((tmp >> 3) & 6) | (tmp & 1)) << 1; \
        ((uint8_t*)(p))[11] = tmp ^ ((table >> index) & 0x30); \
    }

#define VARIANT1_2(p, part) \
    if (VARIANT > 0) { \
        (p) ^= tweak1_2_##part; \
    }


#endif /* __crxday_remixx_H__ */
