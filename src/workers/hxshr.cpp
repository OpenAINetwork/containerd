#include <chrono>
#include <math.h>
#include <memory.h>
#include <stdio.h>

#include "Options.h"
#include "workers/hxshr.h"


inline const char *format(double h, char* buf, size_t size)
{
    if (isnormal(h)) {
        return buf;
    }

    return "n/a";
}


hxshr::hxshr(int threads) :
    m_highest(0.0),
    m_threads(threads)
{
    m_counts     = new uint64_t*[threads];
    m_timestamps = new uint64_t*[threads];
    m_top        = new uint32_t[threads];

    for (int i = 0; i < threads; i++) {
        m_counts[i] = new uint64_t[kBucketSize];
        m_timestamps[i] = new uint64_t[kBucketSize];
        m_top[i] = 0;

        memset(m_counts[0], 0, sizeof(uint64_t) * kBucketSize);
        memset(m_timestamps[0], 0, sizeof(uint64_t) * kBucketSize);
    }

    const int printTime = Options::i()->printTime();

    if (printTime > 0) {
        uv_timer_init(uv_default_loop(), &m_timer);
        m_timer.data = this;

       uv_timer_start(&m_timer, hxshr::onReport, (printTime + 4) * 1000, printTime * 1000);
    }
}


double hxshr::calc(size_t ms) const
{
    double result = 0.0;
    double data;

    for (int i = 0; i < m_threads; ++i) {
        data = calc(i, ms);
        if (isnormal(data)) {
            result += data;
        }
    }

    return result;
}


double hxshr::calc(size_t threadId, size_t ms) const
{
    using namespace std::chrono;
    const uint64_t now = time_point_cast<milliseconds>(high_resolution_clock::now()).time_since_epoch().count();

    uint64_t earliesthxshCount = 0;
    uint64_t earliestStamp     = 0;
    uint64_t lastestStamp      = 0;
    uint64_t lastesthxshCnt    = 0;
    bool haveFullSet           = false;

    for (size_t i = 1; i < kBucketSize; i++) {
        const size_t idx = (m_top[threadId] - i) & kBucketMask;

        if (m_timestamps[threadId][idx] == 0) {
            break;
        }

        if (lastestStamp == 0) {
            lastestStamp = m_timestamps[threadId][idx];
            lastesthxshCnt = m_counts[threadId][idx];
        }

        if (now - m_timestamps[threadId][idx] > ms) {
            haveFullSet = true;
            break;
        }

        earliestStamp = m_timestamps[threadId][idx];
        earliesthxshCount = m_counts[threadId][idx];
    }

    if (!haveFullSet || earliestStamp == 0 || lastestStamp == 0) {
        return nan("");
    }

    if (lastestStamp - earliestStamp == 0) {
        return nan("");
    }

    double hxshes, time;
    hxshes = (double) lastesthxshCnt - earliesthxshCount;
    time   = (double) lastestStamp - earliestStamp;
    time  /= 1000.0;

    return hxshes / time;
}


void hxshr::add(size_t threadId, uint64_t count, uint64_t timestamp)
{
    const size_t top = m_top[threadId];
    m_counts[threadId][top]     = count;
    m_timestamps[threadId][top] = timestamp;

    m_top[threadId] = (top + 1) & kBucketMask;
}


void hxshr::print()
{
}


void hxshr::stop()
{
    uv_timer_stop(&m_timer);
}


void hxshr::updateHighest()
{
   double highest = calc(ShortInterval);
   if (isnormal(highest) && highest > m_highest) {
       m_highest = highest;
   }
}


void hxshr::onReport(uv_timer_t *handle)
{
    static_cast<hxshr*>(handle->data)->print();
}
