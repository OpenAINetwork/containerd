#ifndef __JOBRESULT_H__
#define __JOBRESULT_H__


#include <memory.h>
#include <stdint.h>
#include "Job.h"


class JobResult
{
public:
    inline JobResult() : nodemasterId(0), diff(0), nonce(0) {}
    inline JobResult(int nodemasterId, const containerd::Id &jobId, uint32_t nonce, const uint8_t *result, uint32_t diff) :
        nodemasterId(nodemasterId),
        diff(diff),
        nonce(nonce),
        jobId(jobId)
    {
        memcpy(this->result, result, sizeof(this->result));
    }


    inline JobResult(const Job &job) : nodemasterId(0), diff(0), nonce(0)
    {
        jobId  = job.id();
        nodemasterId = job.nodemasterId();
        diff   = job.diff();
        nonce  = *job.nonce();
    }


    inline JobResult &operator=(const Job &job) {
        jobId  = job.id();
        nodemasterId = job.nodemasterId();
        diff   = job.diff();

        return *this;
    }


    inline uint64_t actualDiff() const
    {
        return Job::toDiff(reinterpret_cast<const uint64_t*>(result)[3]);
    }


    int nodemasterId;
    uint32_t diff;
    uint32_t nonce;
    uint8_t result[32];
    containerd::Id jobId;
};

#endif