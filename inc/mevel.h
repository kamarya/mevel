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


mevel_ctx_t*    mevel_ini();

/**
 * @brief mevel_rel releases the context
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
 * @brief mevel_add_fio() adds a file I/O event
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_fio(mevel_ctx_t*, mevel_cb_t, int fd, int evmask);

/**
 * @brief mevel_add_tot()
 *
 * @param timeout specifies the initial expiration of the timer
 * @param period specifies the repeated timer interval
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_tot(mevel_ctx_t*, mevel_cb_t, int timeout, int period);

/**
 * @brief mevel_add_tcp()
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_tcp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);

/**
 * @brief mevel_add_udp()
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_udp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);

/**
 * @brief mevel_add_sig()
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_add_sig(mevel_ctx_t*, mevel_cb_t, int, ...);

/**
 * @brief mevel_ini_fio() creates a file I/O event context
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_fio(mevel_ctx_t*, mevel_cb_t, int fd, int evmask);

/**
 * @brief mevel_ini_tot()
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_tot(mevel_ctx_t*, mevel_cb_t, int timeout, int period);

/**
 * @brief mevel_ini_tcp()
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_tcp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);

/**
 * @brief mevel_ini_udp()
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_udp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);

/**
 * @brief mevel_ini_sig()
 *
 * @return mevel_event_t*
 */
mevel_event_t*  mevel_ini_sig(mevel_ctx_t*, mevel_cb_t);

/**
 * @brief mevel_ini_sig_add()
 *
 * @return mevel_err_t
 */
mevel_err_t     mevel_ini_sig_add(mevel_event_t*, int);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
