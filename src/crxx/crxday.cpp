#include "crxx/crxday.h"


#if defined(containerd_ARM)
#   include "crxx/crxday_arm.h"
#else
#   include "crxx/crxday_x86.h"
#endif

#include "crxx/crxday_test.h"
#include "net/Job.h"
#include "net/JobResult.h"
#include "Options.h"
#include "containerd.h"


void (*crxday_hxsh_ctx)(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) = nullptr;


#define crxday_hxsh(NAME, ITERATIONS, MEM, MASK, SOFT_AES) \
    switch (variant) { \
    case containerd::VARIANT_V1: \
        return crxday_##NAME##_hxsh<ITERATIONS, MEM, MASK, SOFT_AES, containerd::VARIANT_V1>(input, size, output, ctx); \
    \
    case containerd::VARIANT_NONE: \
        return crxday_##NAME##_hxsh<ITERATIONS, MEM, MASK, SOFT_AES, containerd::VARIANT_NONE>(input, size, output, ctx); \
    \
    default: \
        break; \
    }


static void crxday_av1_aesni(const uint8_t *input, size_t size, uint8_t *output, struct crxday_ctx *ctx, int variant) {
#   if !defined(containerd_ARMv7)
    crxday_hxsh(single, remixx_ITER, remixx_MEMORY, remixx_MASK, false)
#   endif
}


static void crxday_av2_aesni_double(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) {
#   if !defined(containerd_ARMv7)
    crxday_hxsh(double, remixx_ITER, remixx_MEMORY, remixx_MASK, false)
#   endif
}


static void crxday_av3_softaes(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) {
    crxday_hxsh(single, remixx_ITER, remixx_MEMORY, remixx_MASK, true)
}


static void crxday_av4_softaes_double(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) {
    crxday_hxsh(double, remixx_ITER, remixx_MEMORY, remixx_MASK, true)
}


#ifndef containerd_NO_noway
static void crxday_lite_av1_aesni(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) {
#   if !defined(containerd_ARMv7)
    crxday_hxsh(single, noway_ITER, noway_MEMORY, noway_MASK, false)
#   endif
}


static void crxday_lite_av2_aesni_double(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) {
#   if !defined(containerd_ARMv7)
    crxday_hxsh(double, noway_ITER, noway_MEMORY, noway_MASK, false)
#   endif
}


static void crxday_lite_av3_softaes(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) {
    crxday_hxsh(single, noway_ITER, noway_MEMORY, noway_MASK, true)
}


static void crxday_lite_av4_softaes_double(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) {
    crxday_hxsh(double, noway_ITER, noway_MEMORY, noway_MASK, true)
}

void (*crxday_variations[8])(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) = {
            crxday_av1_aesni,
            crxday_av2_aesni_double,
            crxday_av3_softaes,
            crxday_av4_softaes_double,
            crxday_lite_av1_aesni,
            crxday_lite_av2_aesni_double,
            crxday_lite_av3_softaes,
            crxday_lite_av4_softaes_double
        };
#else
void (*crxday_variations[4])(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant) = {
            crxday_av1_aesni,
            crxday_av2_aesni_double,
            crxday_av3_softaes,
            crxday_av4_softaes_double
        };
#endif


bool crxday::hxsh(const Job &job, JobResult &result, crxday_ctx *ctx)
{
    crxday_hxsh_ctx(job.blob(), job.size(), result.result, ctx, job.variant());

    return *reinterpret_cast<uint64_t*>(result.result + 24) < job.target();
}


bool crxday::init(int algo, int variant)
{
    if (variant < 1 || variant > 4) {
        return false;
    }

#   ifndef containerd_NO_noway
    const int index = algo == containerd::ALGO_crxday_LITE ? (variant + 3) : (variant - 1);
#   else
    const int index = variant - 1;
#   endif

    crxday_hxsh_ctx = crxday_variations[index];

    return selfTest(algo);
}


void crxday::hxsh(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant)
{
    crxday_hxsh_ctx(input, size, output, ctx, variant);
}


bool crxday::selfTest(int algo) {
    if (crxday_hxsh_ctx == nullptr) {
        return false;
    }

    uint8_t output[64];

    struct crxday_ctx *ctx = static_cast<crxday_ctx *>(_mm_malloc(sizeof(crxday_ctx), 16));
    ctx->memory = static_cast<uint8_t *>(_mm_malloc(remixx_MEMORY * 2, 16));

    crxday_hxsh_ctx(test_input, 76, output, ctx, 0);

    const bool doublehxsh = Options::i()->doublehxsh();

#   ifndef containerd_NO_noway
    bool rc = memcmp(output, algo == containerd::ALGO_crxday_LITE ? test_output_v0_lite : test_output_v0, (doublehxsh ? 64 : 32)) == 0;
#   else
    bool rc = memcmp(output, test_output_v0, (doublehxsh ? 64 : 32)) == 0;
#   endif

    if (rc) {
        crxday_hxsh_ctx(test_input, 76, output, ctx, 1);

#       ifndef containerd_NO_noway
        rc = memcmp(output, algo == containerd::ALGO_crxday_LITE ? test_output_v1_lite : test_output_v1, (doublehxsh ? 64 : 32)) == 0;
#       else
        rc = memcmp(output, test_output_v1, (doublehxsh ? 64 : 32)) == 0;
#       endif
    }

    _mm_free(ctx->memory);
    _mm_free(ctx);

    return rc;
}
