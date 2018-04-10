#include <string.h>


#include "net/Job.h"


static inline unsigned char hf_hex2bin(char c, bool &err)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 0xA;
    }
    else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 0xA;
    }

    err = true;
    return 0;
}


static inline char hf_bin2hex(unsigned char c)
{
    if (c <= 0x9) {
        return '0' + c;
    }

    return 'a' - 0xA + c;
}


Job::Job() :
    m_nhserver(false),
    m_cnxx(),
    m_algo(containerd::ALGO_crxday),
    m_nodemasterId(-2),
    m_threadId(-1),
    m_variant(containerd::VARIANT_AUTO),
    m_size(0),
    m_diff(0),
    m_target(0),
    m_blob()
{
}


Job::Job(int nodemasterId, bool nhserver, int algo, int variant) :
    m_nhserver(nhserver),
    m_cnxx(),
    m_algo(algo),
    m_nodemasterId(nodemasterId),
    m_threadId(-1),
    m_variant(variant),
    m_size(0),
    m_diff(0),
    m_target(0),
    m_blob()
{
}


Job::~Job()
{
}


bool Job::setBlob(const char *blob)
{
    if (!blob) {
        return false;
    }

    m_size = strlen(blob);
    if (m_size % 2 != 0) {
        return false;
    }

    m_size /= 2;
    if (m_size < 76 || m_size >= sizeof(m_blob)) {
        return false;
    }

    if (!fromHex(blob, (int) m_size * 2, m_blob)) {
        return false;
    }

    if (*nonce() != 0 && !m_nhserver) {
        m_nhserver = true;
    }

    return true;
}


bool Job::setTarget(const char *target)
{
    if (!target) {
        return false;
    }

    const size_t len = strlen(target);

    if (len <= 8) {
        uint32_t tmp = 0;
        char str[8];
        memcpy(str, target, len);

        if (!fromHex(str, 8, reinterpret_cast<unsigned char*>(&tmp)) || tmp == 0) {
            return false;
        }

        m_target = 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / static_cast<uint64_t>(tmp));
    }
    else if (len <= 16) {
        m_target = 0;
        char str[16];
        memcpy(str, target, len);

        if (!fromHex(str, 16, reinterpret_cast<unsigned char*>(&m_target)) || m_target == 0) {
            return false;
        }
    }
    else {
        return false;
    }

    m_diff = toDiff(m_target);
    return true;
}


void Job::setcnxx(const char *cnxx)
{
    if (!cnxx || strlen(cnxx) > 4) {
        memset(m_cnxx, 0, sizeof(m_cnxx));
        return;
    }

    strncpy(m_cnxx, cnxx, sizeof(m_cnxx));
    m_algo = strcmp(m_cnxx, "noway") == 0 ? containerd::ALGO_crxday_LITE : containerd::ALGO_crxday;
}


void Job::setVariant(int variant)
{
    switch (variant) {
    case containerd::VARIANT_AUTO:
    case containerd::VARIANT_NONE:
    case containerd::VARIANT_V1:
        m_variant = variant;
        break;

    default:
        break;
    }
}


bool Job::fromHex(const char* in, unsigned int len, unsigned char* out)
{
    bool error = false;
    for (unsigned int i = 0; i < len; i += 2) {
        out[i / 2] = (hf_hex2bin(in[i], error) << 4) | hf_hex2bin(in[i + 1], error);

        if (error) {
            return false;
        }
    }
    return true;
}


void Job::toHex(const unsigned char* in, unsigned int len, char* out)
{
    for (unsigned int i = 0; i < len; i++) {
        out[i * 2] = hf_bin2hex((in[i] & 0xF0) >> 4);
        out[i * 2 + 1] = hf_bin2hex(in[i] & 0x0F);
    }
}


bool Job::operator==(const Job &other) const
{
    return m_id == other.m_id && memcmp(m_blob, other.m_blob, sizeof(m_blob)) == 0;
}


bool Job::operator!=(const Job &other) const
{
    return m_id != other.m_id || memcmp(m_blob, other.m_blob, sizeof(m_blob)) != 0;
}
