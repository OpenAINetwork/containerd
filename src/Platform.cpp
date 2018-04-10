#include <string.h>
#include <uv.h>


#include "Platform.h"


char *Platform::m_defaultConfigName = nullptr;
char *Platform::m_userAgent         = nullptr;


const char *Platform::defaultConfigName()
{
    size_t size = 520;

    if (m_defaultConfigName == nullptr) {
        m_defaultConfigName = new char[size];
    }

    if (uv_exepath(m_defaultConfigName, &size) < 0) {
        return nullptr;
    }

    if (size < 500) {
#       ifdef WIN32
        char *p = strrchr(m_defaultConfigName, '\\');
#       else
        char *p = strrchr(m_defaultConfigName, '/');
#       endif

        if (p) {
            strcpy(p + 1, "config.json");
            return m_defaultConfigName;
        }
    }

    return nullptr;
}
