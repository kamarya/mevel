/*
 *   Copyright 2018 Behrooz Kamary
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef __MEVEL_H__
#define __MEVEL_H__

#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>

#include "types.h"
#include "queue.h"

#ifdef __cplusplus
#include <functional>
#include <unordered_map>
#include <exception>
#include <string>

extern "C" {
#endif

// TODO: consider EPOLLEXCLUSIVE and EPOLLONESHOT
// for multi-threaded solutions.

typedef struct {
    int             epollfd;    // epoll file descriptor
    char            running;    // atomic event loop state
    queue_ctx_t*    qctx;
} mevel_ctx_t;

typedef struct mevel_event {
    mevel_type_t    type;
    mevel_ctx_t*    ctx;
    epoll_event_t   event;
    sigset_t        smask;
    int             evmask;
    int             fd;
    mevel_err_t (*cb)(struct mevel_event*, int);
} mevel_event_t;

typedef mevel_err_t (mevel_cb_t)(mevel_event_t*, int);

/**
 * @brief mevel_ini initializes the context
 *
 */
mevel_ctx_t*    mevel_ini();

/**
 * @brief mevel_rel releases the context and its allocated resources
 *
 */
void            mevel_rel(mevel_ctx_t*);

/**
 * @brief mevel_run runs the event loop and block
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_run(mevel_ctx_t*);

/**
 * @brief mevel_add
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add(mevel_ctx_t*, mevel_event_t*);

/**
 * @brief mevel_del
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_del(mevel_ctx_t*, mevel_event_t*);

/**
 * @brief mevel_add_fio adds a file I/O event
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_fio(mevel_ctx_t*, mevel_cb_t, int fd, int evmask);

/**
 * @brief mevel_add_timer
 *
 * @param timeout specifies the initial expiration of the timer
 * @param period specifies the repeated timer interval
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_timer(mevel_ctx_t*, mevel_cb_t, int timeout, int period);

/**
 * @brief mevel_add_tcp
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_tcp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);

/**
 * @brief mevel_add_udp
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_udp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);

/**
 * @brief mevel_add_sig
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_sig(mevel_ctx_t*, mevel_cb_t, int, ...);

/**
 * @brief mevel_ini_fio creates a file I/O event context
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_fio(mevel_ctx_t*, mevel_cb_t, int fd, int evmask);

/**
 * @brief mevel_ini_timer
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_timer(mevel_ctx_t*, mevel_cb_t, int timeout, int period);

/**
 * @brief mevel_ini_tcp
 *
 * @param ctx context
 * @param cb callback function
 * @param stype
 * @param straddr
 * @param port
 * @param evmask
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_tcp(mevel_ctx_t* ctx, mevel_cb_t cb, int stype, const char* straddr, int port, int evmask);

/**
 * @brief mevel_ini_udp
 *
 * @param ctx context
 * @param cb callback function
 * @param stype
 * @param straddr
 * @param port
 * @param evmask
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_udp(mevel_ctx_t* ctx, mevel_cb_t cb, int stype, const char* straddr, int port, int evmask);

/**
 * @brief mevel_ini_sig
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_sig(mevel_ctx_t*, mevel_cb_t);

/**
 * @brief mevel_ini_sig_add
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_ini_sig_add(mevel_event_t*, int);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
namespace mevel
{

enum type_en
{
	MEVEL_TYPE_IO       = 100,
	MEVEL_TYPE_SIGNAL   = 101,
	MEVEL_TYPE_TIMER    = 102,
	MEVEL_TYPE_ACC      = 103,
};

enum error_en
{
    MEVEL_ERR_NONE,
    MEVEL_ERR_UNKNOWN,
    MEVEL_ERR_CONSTRUCTOR,
    MEVEL_ERR_NULL,
    MEVEL_ERR_ADD,
    MEVEL_ERR_DEL,
    MEVEL_ERR_HUP,
    MEVEL_ERR_WAIT,
    MEVEL_ERR_CLOSE,
    MEVEL_ERR_STOP,
    MEVEL_ERR_TIMER,
    MEVEL_ERR_SIGNAL,
    MEVEL_ERR_UDP,
    MEVEL_ERR_TCP,
    MEVEL_ERR_FIO
};

struct mevent;
using callback_t = std::function<error_en(const mevent&, int)>;

struct mevent
{
    type_en         type;
    epoll_event     event;
    sigset_t        smask;
    int             evmask;
    int             fd;
    callback_t      cb;
};

class mevel
{
private:

    int                                 epollfd;
    char                                running;
    std::unordered_map <int, mevent>    eventmap;
    error_en                            error_flag;
    mevent                              ev_signal;

    bool add(mevent ev);
    bool del(mevent& ev);

public:

    mevel();
    ~mevel();

    bool add_timer(callback_t cb, int timeout, int period);
    bool add_fio(callback_t cb, int fd, int evmask);
    bool add_tcp(callback_t cb, int stype, const char* straddr, int port, int evmask);
    bool add_udp(callback_t cb, int stype, const char* straddr, int port, int evmask);
    bool add_signal(callback_t cb, int sig);
    bool add_signal(callback_t cb, std::initializer_list<int> signums);

    void clear_error_flag();
    error_en get_error_flag();

    bool run();
};

class exception : public std::exception
{
private:
    std::string     message;
    error_en        err;

public:

    exception(const char* msg, error_en err = MEVEL_ERR_UNKNOWN)
    : message(msg)
    , err(err)
    {
        /* constructor */
    }

    const char* what()
    {
        return message.c_str();
    }

    error_en get_error()
    {
        return err;
    }
};

}
#endif

#endif
