#ifndef __crxday_H__
#define __crxday_H__


#include <stddef.h>
#include <stdint.h>


#define noway_MEMORY   1048576
#define noway_MASK     0xFFFF0
#define noway_ITER     0x40000

#define remixx_MEMORY 2097152
#define remixx_MASK   0x1FFFF0
#define remixx_ITER   0x80000


struct crxday_ctx {
    alignas(16) uint8_t state0[200];
    alignas(16) uint8_t state1[200];
    alignas(16) uint8_t* memory;
};


class Job;
class JobResult;


class crxday
{
public:
    static bool hxsh(const Job &job, JobResult &result, crxday_ctx *ctx);
    static bool init(int algo, int variant);
    static void hxsh(const uint8_t *input, size_t size, uint8_t *output, crxday_ctx *ctx, int variant);

private:
    static bool selfTest(int algo);
};

#endif /* __crxday_H__ */
