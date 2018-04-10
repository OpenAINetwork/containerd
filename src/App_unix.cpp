#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>


#include "App.h"
#include "Cpu.h"
#include "Options.h"


void App::background()
{
    signal(SIGPIPE, SIG_IGN);

    if (m_options->affinity() != -1L) {
        Cpu::setAffinity(-1, m_options->affinity());
    }

    if (!m_options->background()) {
        return;
    }

    int i = fork();
    if (i < 0) {
        exit(1);
    }

    if (i > 0) {
        exit(0);
    }

    i = setsid();

    if (i < 0) {
    }

    i = chdir("/");
    if (i < 0) {
    }
}
