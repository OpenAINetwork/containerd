#ifndef __ISTRATEGYLISTENER_H__
#define __ISTRATEGYLISTENER_H__


#include <stdint.h>


class Client;
class IStrategy;
class Job;
class SubmitResult;


class IStrategyListener
{
public:
    virtual ~IStrategyListener() {}

    virtual void onActive(IStrategy *strategy, Client *client)                                                        = 0;
    virtual void onJob(IStrategy *strategy, Client *client, const Job &job)                                           = 0;
    virtual void onPause(IStrategy *strategy)                                                                         = 0;
    virtual void onResultAccepted(IStrategy *strategy, Client *client, const SubmitResult &result, const char *error) = 0;
};


#endif