#ifndef __VERSION_H__
#define __VERSION_H__

#define APP_ID        "containerd"
#define APP_NAME      "containerd"
#define APP_DESC      ""
#define APP_VERSION   ""
#define APP_DOMAIN    ""
#define APP_SITE      ""
#define APP_COPYRIGHT ""
#define APP_KIND      ""

#define APP_VER_MAJOR  0
#define APP_VER_MINOR  0
#define APP_VER_BUILD  0
#define APP_VER_REV    0

#ifdef _MSC_VER
#   if (_MSC_VER >= 1910)
#       define MSVC_VERSION 2017
#   elif _MSC_VER == 1900
#       define MSVC_VERSION 2015
#   elif _MSC_VER == 1800
#       define MSVC_VERSION 2013
#   elif _MSC_VER == 1700
#       define MSVC_VERSION 2012
#   elif _MSC_VER == 1600
#       define MSVC_VERSION 2010
#   else
#       define MSVC_VERSION 0
#   endif
#endif

#endif
