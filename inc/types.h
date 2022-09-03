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

#ifndef __TYPES_H__
#define __TYPES_H__

#include <sys/epoll.h>

#define MEVEL_MAX_EVENTS    10
#define MEVEL_MAX_TIMEOUT   2000

#define MEVEL_NONE          0
#define MEVEL_ERROR         EPOLLERR
#define MEVEL_READ          EPOLLIN
#define MEVEL_WRITE         EPOLLOUT
#define MEVEL_PRI           EPOLLPRI
#define MEVEL_HUP           EPOLLHUP
#define MEVEL_RDHUP         EPOLLRDHUP
#define MEVEL_EDGE          EPOLLET
#define MEVEL_ONESHOT       EPOLLONESHOT

#define MEVEL_IPV6          AF_INET6
#define MEVEL_IPV4          AF_INET
#define MEVEL_UNIX          AF_UNIX

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epoll_event epoll_event_t;

typedef enum {
	MEVEL_TYPE_IO       = 100,
	MEVEL_TYPE_SIGNAL   = 101,
	MEVEL_TYPE_TIMER    = 102,
	MEVEL_TYPE_ACC      = 103,
} mevel_type_t;

typedef enum {
    MEVEL_ERR_NONE,
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
} mevel_err_t;

#ifdef __cplusplus
} // extern "C"
#endif


#endif
