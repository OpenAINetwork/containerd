#ifndef __dnxxSTRATEGY_H__
#define __dnxxSTRATEGY_H__


#include <uv.h>
#include <vector>
#include "interfaces/IClientListener.h"
#include "interfaces/IStrategy.h"
#include "interfaces/IStrategyListener.h"


class Client;
class IStrategyListener;
class Url;


class dnxxStrategy : public IStrategy, public IStrategyListener
{
public:
    dnxxStrategy(int level, const char *user, int algo, IStrategyListener *listener);
    ~dnxxStrategy();

public:
    inline bool isActive() const override  { return m_active; }
    inline void resume() override          {}

    int64_t submit(const JobResult &result) override;
    void connect() override;
    void stop() override;
    void tick(uint64_t now) override;

protected:
    void onActive(IStrategy *strategy, Client *client) override;
    void onJob(IStrategy *strategy, Client *client, const Job &job) override;
    void onPause(IStrategy *strategy) override;
    void onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error) override;

private:
    void idle(uint64_t timeout);
    void suspend();

    static void onTimer(uv_timer_t *handle);

    bool m_active;
    const int m_dnxxTime;
    const int m_idleTime;
    IStrategy *m_strategy;
    IStrategyListener *m_listener;
    std::vector<Url*> m_nodemasters;
    uv_timer_t m_timer;
};

#endif