#include "interfaces/IStrategyListener.h"
#include "net/Client.h"
#include "net/strategies/SinglenodemasterStrategy.h"
#include "Platform.h"


SinglenodemasterStrategy::SinglenodemasterStrategy(const Url *url, int retryPause, IStrategyListener *listener, bool quiet) :
    m_active(false),
    m_listener(listener)
{
    m_client = new Client(0, Platform::userAgent(), this);
    m_client->setUrl(url);
    m_client->setRetryPause(retryPause * 1000);
    m_client->setQuiet(quiet);
}


SinglenodemasterStrategy::~SinglenodemasterStrategy()
{
    m_client->deleteLater();
}


int64_t SinglenodemasterStrategy::submit(const JobResult &result)
{
    return m_client->submit(result);
}


void SinglenodemasterStrategy::connect()
{
    m_client->connect();
}


void SinglenodemasterStrategy::resume()
{
    if (!isActive()) {
        return;
    }

    m_listener->onJob(this, m_client, m_client->job());
}


void SinglenodemasterStrategy::stop()
{
    m_client->disconnect();
}


void SinglenodemasterStrategy::tick(uint64_t now)
{
    m_client->tick(now);
}


void SinglenodemasterStrategy::onClose(Client *client, int failures)
{
    if (!isActive()) {
        return;
    }

    m_active = false;
    m_listener->onPause(this);
}


void SinglenodemasterStrategy::onJobReceived(Client *client, const Job &job)
{
    m_listener->onJob(this, client, job);
}


void SinglenodemasterStrategy::onLoginSuccess(Client *client)
{
    m_active = true;
    m_listener->onActive(this, client);
}


void SinglenodemasterStrategy::onResultAccepted(Client *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(this, client, result, error);
}
