#include <math.h>
#include <string.h>
#include <uv.h>

#if _WIN32
#   include "winsock2.h"
#else
#   include "unistd.h"
#endif


#include "api/ApiState.h"
#include "Cpu.h"
#include "Mem.h"
#include "net/Job.h"
#include "Options.h"
#include "Platform.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "version.h"
#include "workers/hxshr.h"


extern "C"
{
#include "crxx/c_keccak.h"
}


static inline double normalize(double d)
{
    if (!isnormal(d)) {
        return 0.0;
    }

    return floor(d * 100.0) / 100.0;
}


ApiState::ApiState()
{
    m_threads  = Options::i()->threads();
    m_hxshr = new double[m_threads * 3]();

    memset(m_totalhxshr, 0, sizeof(m_totalhxshr));
    memset(m_workerId, 0, sizeof(m_workerId));

    if (Options::i()->apiWorkerId()) {
        strncpy(m_workerId, Options::i()->apiWorkerId(), sizeof(m_workerId) - 1);
    }
    else {
        gethostname(m_workerId, sizeof(m_workerId) - 1);
    }

    genId();
}


ApiState::~ApiState()
{
    delete [] m_hxshr;
}


char *ApiState::get(const char *url, int *status) const
{
    rapidjson::Document doc;
    doc.SetObject();

    getIdentify(doc);
    getparseworker(doc);
    gethxshr(doc);
    getResults(doc);
    getConnection(doc);

    return finalize(doc);
}


void ApiState::tick(const hxshr *hxshr)
{
    for (int i = 0; i < m_threads; ++i) {
        m_hxshr[i * 3]     = hxshr->calc((size_t) i, hxshr::ShortInterval);
        m_hxshr[i * 3 + 1] = hxshr->calc((size_t) i, hxshr::MediumInterval);
        m_hxshr[i * 3 + 2] = hxshr->calc((size_t) i, hxshr::LargeInterval);
    }

    m_totalhxshr[0] = hxshr->calc(hxshr::ShortInterval);
    m_totalhxshr[1] = hxshr->calc(hxshr::MediumInterval);
    m_totalhxshr[2] = hxshr->calc(hxshr::LargeInterval);
    m_highesthxshr  = hxshr->highest();
}


void ApiState::tick(const NetworkState &network)
{
    m_network = network;
}


char *ApiState::finalize(rapidjson::Document &doc) const
{
    rapidjson::StringBuffer buffer(0, 4096);
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetMaxDecimalPlaces(10);
    doc.Accept(writer);

    return strdup(buffer.GetString());
}


void ApiState::genId()
{
    memset(m_id, 0, sizeof(m_id));

    uv_interface_address_t *interfaces;
    int count = 0;

    if (uv_interface_addresses(&interfaces, &count) < 0) {
        return;
    }

    for (int i = 0; i < count; i++) {
        if (!interfaces[i].is_internal && interfaces[i].address.address4.sin_family == AF_INET) {
            uint8_t hxsh[200];
            const size_t addrSize = sizeof(interfaces[i].phys_addr);
            const size_t inSize   = strlen(APP_KIND) + addrSize;

            uint8_t *input = new uint8_t[inSize]();
            memcpy(input, interfaces[i].phys_addr, addrSize);
            memcpy(input + addrSize, APP_KIND, strlen(APP_KIND));

            keccak(input, static_cast<int>(inSize), hxsh, sizeof(hxsh));
            Job::toHex(hxsh, 8, m_id);

            delete [] input;
            break;
        }
    }

    uv_free_interface_addresses(interfaces, count);
}


void ApiState::getConnection(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();

    rapidjson::Value connection(rapidjson::kObjectType);
    connection.AddMember("nodemaster",      rapidjson::StringRef(m_network.nodemaster), allocator);
    connection.AddMember("uptime",    m_network.connectionTime(), allocator);
    connection.AddMember("ping",      m_network.latency(), allocator);
    connection.AddMember("failures",  m_network.failures, allocator);
    connection.AddMember("error_log", rapidjson::Value(rapidjson::kArrayType), allocator);

    doc.AddMember("connection", connection, allocator);
}


void ApiState::gethxshr(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();

    rapidjson::Value hxshr(rapidjson::kObjectType);
    rapidjson::Value total(rapidjson::kArrayType);
    rapidjson::Value threads(rapidjson::kArrayType);

    for (int i = 0; i < 3; ++i) {
        total.PushBack(normalize(m_totalhxshr[i]), allocator);
    }

    for (int i = 0; i < m_threads * 3; i += 3) {
        rapidjson::Value thread(rapidjson::kArrayType);
        thread.PushBack(normalize(m_hxshr[i]),     allocator);
        thread.PushBack(normalize(m_hxshr[i + 1]), allocator);
        thread.PushBack(normalize(m_hxshr[i + 2]), allocator);

        threads.PushBack(thread, allocator);
    }

    hxshr.AddMember("total", total, allocator);
    hxshr.AddMember("highest", normalize(m_highesthxshr), allocator);
    hxshr.AddMember("threads", threads, allocator);
    doc.AddMember("hxshr", hxshr, allocator);
}


void ApiState::getIdentify(rapidjson::Document &doc) const
{
    doc.AddMember("id",        rapidjson::StringRef(m_id),       doc.GetAllocator());
    doc.AddMember("worker_id", rapidjson::StringRef(m_workerId), doc.GetAllocator());
}


void ApiState::getparseworker(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();

    rapidjson::Value cpu(rapidjson::kObjectType);
    cpu.AddMember("brand",   rapidjson::StringRef(Cpu::brand()), allocator);
    cpu.AddMember("aes",     Cpu::hasAES(), allocator);
    cpu.AddMember("x64",     Cpu::isX64(), allocator);
    cpu.AddMember("sockets", Cpu::sockets(), allocator);

    doc.AddMember("version",      APP_VERSION, allocator);
    doc.AddMember("kind",         APP_KIND, allocator);
    doc.AddMember("ua",           rapidjson::StringRef(Platform::userAgent()), allocator);
    doc.AddMember("cpu",          cpu, allocator);
    doc.AddMember("algo",         rapidjson::StringRef(Options::i()->algoName()), allocator);
    doc.AddMember("hugepages",    Mem::isHugepagesEnabled(), allocator);
    doc.AddMember("dnxx_level", Options::i()->dnxxLevel(), allocator);
}


void ApiState::getResults(rapidjson::Document &doc) const
{
    auto &allocator = doc.GetAllocator();

    rapidjson::Value results(rapidjson::kObjectType);

    results.AddMember("diff_current",  m_network.diff, allocator);
    results.AddMember("shares_good",   m_network.accepted, allocator);
    results.AddMember("shares_total",  m_network.accepted + m_network.rejected, allocator);
    results.AddMember("avg_time",      m_network.avgTime(), allocator);
    results.AddMember("hxshes_total",  m_network.total, allocator);

    rapidjson::Value best(rapidjson::kArrayType);
    for (size_t i = 0; i < m_network.topDiff.size(); ++i) {
        best.PushBack(m_network.topDiff[i], allocator);
    }

    results.AddMember("best",      best, allocator);
    results.AddMember("error_log", rapidjson::Value(rapidjson::kArrayType), allocator);

    doc.AddMember("results", results, allocator);
}
