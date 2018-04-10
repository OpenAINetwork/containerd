#ifndef __JOB_H__
#define __JOB_H__


#include <stddef.h>
#include <stdint.h>
#include "net/Id.h"
#include "containerd.h"


class Job
{
public:
    Job();
    Job(int nodemasterId, bool nhserver, int algo, int variant);
    ~Job();

    bool setBlob(const char *blob);
    bool setTarget(const char *target);
    void setcnxx(const char *cnxx);
    void setVariant(int variant);

    inline bool isnhserver() const         { return m_nhserver; }
    inline bool isValid() const            { return m_size > 0 && m_diff > 0; }
    inline bool setId(const char *id)      { return m_id.setId(id); }
    inline const char *cnxx() const        { return m_cnxx; }
    inline const uint32_t *nonce() const   { return reinterpret_cast<const uint32_t*>(m_blob + 39); }
    inline const uint8_t *blob() const     { return m_blob; }
    inline const containerd::Id &id() const     { return m_id; }
    inline int nodemasterId() const              { return m_nodemasterId; }
    inline int threadId() const            { return m_threadId; }
    inline int variant() const             { return (m_variant == containerd::VARIANT_AUTO ? (m_blob[0] > 6 ? 1 : 0) : m_variant); }
    inline size_t size() const             { return m_size; }
    inline uint32_t *nonce()               { return reinterpret_cast<uint32_t*>(m_blob + 39); }
    inline uint32_t diff() const           { return (uint32_t) m_diff; }
    inline uint64_t target() const         { return m_target; }
    inline void setnhserver(bool nhserver) { m_nhserver = nhserver; }
    inline void setnodemasterId(int nodemasterId)      { m_nodemasterId = nodemasterId; }
    inline void setThreadId(int threadId)  { m_threadId = threadId; }

    static bool fromHex(const char* in, unsigned int len, unsigned char* out);
    static inline uint32_t *nonce(uint8_t *blob)   { return reinterpret_cast<uint32_t*>(blob + 39); }
    static inline uint64_t toDiff(uint64_t target) { return 0xFFFFFFFFFFFFFFFFULL / target; }
    static void toHex(const unsigned char* in, unsigned int len, char* out);

    bool operator==(const Job &other) const;
    bool operator!=(const Job &other) const;

private:
    bool m_nhserver;
    char m_cnxx[5];
    int m_algo;
    int m_nodemasterId;
    int m_threadId;
    int m_variant;
    size_t m_size;
    uint64_t m_diff;
    uint64_t m_target;
    uint8_t m_blob[96];
    containerd::Id m_id;
};

#endif