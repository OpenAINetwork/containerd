#ifndef __NETWORK_H__
#define __NETWORK_H__


#include <vector>
#include <uv.h>
#include "api/NetworkState.h"
#include "interfaces/IJobResultListener.h"
#include "interfaces/IStrategyListener.h"


class IStrategy;
class Options;
class Url;


class Network : public IJobResultListener, public IStrategyListener
{
public:
  Network(const Options *options);
  ~Network();

  void connect();
  void stop();

protected:
  void onActive(IStrategy *strategy, Client *client) override;
  void onJob(IStrategy *strategy, Client *client, const Job &job) override;
  void onJobResult(const JobResult &result) override;
  void onPause(IStrategy *strategy) override;
  void onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error) override;

private:
  constexpr static int kTickInterval = 1 * 1000;

  void setJob(Client *client, const Job &job, bool dnxx);
  void tick();

  static void onTick(uv_timer_t *handle);

  const Options *m_options;
  IStrategy *m_dnxx;
  IStrategy *m_strategy;
  NetworkState m_state;
  uv_timer_t m_timer;
};


#endif