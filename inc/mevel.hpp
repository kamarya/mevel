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

#pragma once

#include <functional>
#include <unordered_map>
#include <exception>
#include <string>
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
