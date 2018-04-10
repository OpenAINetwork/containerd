#include "interfaces/IStrategyListener.h"
#include "net/Client.h"
#include "net/strategies/FailoverStrategy.h"
#include "Platform.h"


FailoverStrategy::FailoverStrategy(const std::vector<Url*> &urls, int retryPause, int retries, IStrategyListener *listener, bool quiet) :
    m_quiet(quiet),
    m_retries(retries),
    m_retryPause(retryPause),
    m_active(-1),
    m_index(0),
    m_listener(listener)
{
    for (const Url *url : urls) {
        add(url);
    }
}


FailoverStrategy::~FailoverStrategy()
{
    for (Client *client : m_nodemasters) {
        client->deleteLater();
    }
}


int64_t FailoverStrategy::submit(const JobResult &result)
{
    if (m_active == -1) {
        return -1;
    }

    return m_nodemasters[m_active]->submit(result);
}


void FailoverStrategy::connect()
{
    m_nodemasters[m_index]->connect();
}


void FailoverStrategy::resume()
{
    if (!isActive()) {
        return;
    }

    m_listener->onJob(this, m_nodemasters[m_active],  m_nodemasters[m_active]->job());
}


void FailoverStrategy::stop()
{
    for (size_t i = 0; i < m_nodemasters.size(); ++i) {
        m_nodemasters[i]->disconnect();
    }

    m_index  = 0;
    m_active = -1;

    m_listener->onPause(this);
}


void FailoverStrategy::tick(uint64_t now)
{
    for (Client *client : m_nodemasters) {
        client->tick(now);
    }
}


void FailoverStrategy::onClose(Client *client, int failures)
{
    if (failures == -1) {
        return;
    }

    if (m_active == client->id()) {
        m_active = -1;
        m_listener->onPause(this);
    }

    if (m_index == 0 && failures < m_retries) {
        return;
    }

    if (m_index == client->id() && (m_nodemasters.size() - m_index) > 1) {
        m_nodemasters[++m_index]->connect();
    }
}


void FailoverStrategy::onJobReceived(Client *client, const Job &job)
{
    if (m_active == client->id()) {
        m_listener->onJob(this, client, job);
    }
}


void FailoverStrategy::onLoginSuccess(Client *client)
{
    int active = m_active;

    if (client->id() == 0 || !isActive()) {
        active = client->id();
    }

    for (size_t i = 1; i < m_nodemasters.size(); ++i) {
        if (active != static_cast<int>(i)) {
            m_nodemasters[i]->disconnect();
        }
    }

    if (active >= 0 && active != m_active) {
        m_index = m_active = active;
        m_listener->onActive(this, client);
    }
}


void FailoverStrategy::onResultAccepted(Client *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(this, client, result, error);
}


void FailoverStrategy::add(const Url *url)
{
    Client *client = new Client((int) m_nodemasters.size(), Platform::userAgent(), this);
    client->setUrl(url);
    client->setRetryPause(m_retryPause * 1000);
    client->setQuiet(m_quiet);

    m_nodemasters.push_back(client);
}
