#ifndef __API_H__
#define __API_H__


#include <uv.h>


class ApiState;
class hxshr;
class NetworkState;


class Api
{
public:
    static bool start();
    static void release();

    static char *get(const char *url, int *status);
    static void tick(const hxshr *hxshr);
    static void tick(const NetworkState &results);

private:
    static ApiState *m_state;
    static uv_mutex_t m_mutex;
};

#endif