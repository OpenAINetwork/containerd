#ifndef __CLIENT_H__
#define __CLIENT_H__


#include <map>
#include <uv.h>
#include <vector>
#include "net/Id.h"
#include "net/Job.h"
#include "net/SubmitResult.h"
#include "net/Url.h"
#include "rapidjson/fwd.h"


class IClientListener;
class JobResult;


class Client
{
public:
    enum SocketState {
        UnconnectedState,
        HostLookupState,
        ConnectingState,
        ConnectedState,
        ClosingState
    };

    constexpr static int kResponseTimeout  = 20 * 1000;
    constexpr static int kKeepAliveTimeout = 60 * 1000;

    Client(int id, const char *agent, IClientListener *listener);

    bool disconnect();
    int64_t submit(const JobResult &result);
    void connect();
    void connect(const Url *url);
    void deleteLater();
    void setUrl(const Url *url);
    void tick(uint64_t now);

    inline bool isReady() const              { return m_state == ConnectedState && m_failures == 0; }
    inline const char *host() const          { return m_url.host(); }
    inline const char *ip() const            { return m_ip; }
    inline const Job &job() const            { return m_job; }
    inline int id() const                    { return m_id; }
    inline SocketState state() const         { return m_state; }
    inline uint16_t port() const             { return m_url.port(); }
    inline void setQuiet(bool quiet)         { m_quiet = quiet; }
    inline void setRetryPause(int ms)        { m_retryPause = ms; }

private:
    ~Client();

    bool close();
    bool isCriticalError(const char *message);
    bool parseJob(const rapidjson::Value &params, int *code);
    bool parseLogin(const rapidjson::Value &result, int *code);
    int resolve(const char *host);
    int64_t send(size_t size);
    void connect(const std::vector<addrinfo*> &ipv4, const std::vector<addrinfo*> &ipv6);
    void connect(sockaddr *addr);
    void login();
    void onClose();
    void parse(char *line, size_t len);
    void parseExtensions(const rapidjson::Value &value);
    void parseNotification(const char *method, const rapidjson::Value &params, const rapidjson::Value &error);
    void parseResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error);
    void ping();
    void reconnect();
    void setState(SocketState state);
    void startTimeout();

    static void onAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
    static void onClose(uv_handle_t *handle);
    static void onConnect(uv_connect_t *req, int status);
    static void onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
    static void onResolved(uv_getaddrinfo_t *req, int status, struct addrinfo *res);

    static inline Client *getClient(void *data) { return static_cast<Client*>(data); }

    addrinfo m_hints;
    bool m_ipv6;
    bool m_nhserver;
    bool m_quiet;
    char m_buf[2048];
    char m_ip[46];
    char m_sendBuf[768];
    const char *m_agent;
    IClientListener *m_listener;
    int m_id;
    int m_retryPause;
    int64_t m_failures;
    Job m_job;
    size_t m_recvBufPos;
    SocketState m_state;
    static int64_t m_sequence;
    std::map<int64_t, SubmitResult> m_results;
    uint64_t m_expire;
    uint64_t m_jobs;
    Url m_url;
    uv_buf_t m_recvBuf;
    uv_getaddrinfo_t m_resolver;
    uv_stream_t *m_stream;
    uv_tcp_t *m_socket;
    containerd::Id m_rpcId;

#   ifndef containerd_PROXY_PROJECT
    uv_timer_t m_keepAliveTimer;
#   endif
};


#endif