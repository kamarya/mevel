/*
 *   Copyright 2018 Behrooz Kamary Aliabadi
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
void            mevel_rel(mevel_ctx_t*);
mevel_err_t     mevel_run(mevel_ctx_t*);
mevel_err_t     mevel_add(mevel_ctx_t*, mevel_event_t*);
mevel_err_t     mevel_del(mevel_ctx_t*, mevel_event_t*);

mevel_err_t     mevel_add_fio(mevel_ctx_t*, mevel_cb_t, int, int);
mevel_err_t     mevel_add_tot(mevel_ctx_t*, mevel_cb_t, int, int);
mevel_err_t     mevel_add_tcp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);
mevel_err_t     mevel_add_udp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);
mevel_err_t     mevel_add_sig(mevel_ctx_t*, mevel_cb_t, int, ...);

mevel_event_t*  mevel_ini_fio(mevel_ctx_t*, mevel_cb_t, int, int);
mevel_event_t*  mevel_ini_tot(mevel_ctx_t*, mevel_cb_t, int, int);
mevel_event_t*  mevel_ini_tcp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);
mevel_event_t*  mevel_ini_udp(mevel_ctx_t*, mevel_cb_t, int, const char*, int, int);
mevel_event_t*  mevel_ini_sig(mevel_ctx_t*, mevel_cb_t);
mevel_err_t     mevel_ini_sig_add(mevel_event_t*, int);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
