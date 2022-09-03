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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdexcept>
#include <initializer_list>

#include <mevel.h>

namespace mevel
{

mevel::mevel()
: epollfd(0)
, running(0)
, eventmap()
, error_flag(MEVEL_ERR_NONE)
{
    epollfd = epoll_create1(EPOLL_CLOEXEC);

    if (epollfd < 0)
    {
        // we can not do anything hereafter; just throw and exception
        throw exception("epoll_create1(EPOLL_CLOEXEC) failed.", MEVEL_ERR_CONSTRUCTOR);
    }

    ev_signal.fd = -1;
}

mevel::~mevel() noexcept
{
    if (epollfd > 0) ::close(epollfd);
}

void mevel::clear_error_flag()
{
    error_flag = MEVEL_ERR_NONE;
}

error_en mevel::get_error_flag()
{
    return error_flag;
}

bool mevel::run()
{

    struct sockaddr_storage     peer;
    socklen_t                   plen;
	epoll_event                 events[MEVEL_MAX_EVENTS];

    int nfds            = 0;
    int timeout         = MEVEL_MAX_TIMEOUT;
	running             = 0xFF;

    while (running != 0x00)
    {

		nfds = epoll_wait(epollfd, events, MEVEL_MAX_EVENTS, timeout);

        if (nfds < 0)
        {
            if (errno == EINTR) error_flag = MEVEL_ERR_HUP;
            else error_flag = MEVEL_ERR_WAIT;
            running = 0x00;
            return false;
        }
        else if (nfds == 0)
        {
            continue;
        }

        for (int indx = 0; indx < nfds; indx++)
        {
            int fd = (int)events[indx].data.fd;
            mevent& ev = eventmap[fd];
            if (events[indx].events == 0) continue;
            if (ev.cb && ev.fd > 0)
            {
                if (ev.type == MEVEL_TYPE_ACC)
                {
                    fd = accept4(ev.fd, (struct sockaddr*)&peer, &plen, SOCK_NONBLOCK);
                    if (fd > 0)
                    {
                        add_fio(ev.cb, fd, ev.evmask);
                    }
                }
                if (ev.cb(ev, events[indx].events) != MEVEL_ERR_NONE)
                {
                    del(ev);
                }
            }
        }

    }
    return true;
}

bool mevel::add(mevent ev)
{
    clear_error_flag();

    if (ev.fd < 0 || eventmap.find(ev.fd) != eventmap.end())
    {
        error_flag = MEVEL_ERR_ADD;
        return false;
    }

    ev.event.data.fd = ev.fd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.fd, &ev.event) < 0)
    {
        error_flag = MEVEL_ERR_ADD;
        return false;
    }

    eventmap.insert(std::make_pair(ev.fd, ev));

    return true;
}

bool mevel::del(mevent& ev)
{
    clear_error_flag();

    if (eventmap.find(ev.fd) == eventmap.end() || eventmap.erase(ev.fd) != 1)
    {
        error_flag = MEVEL_ERR_DEL;
    }
    else if (epoll_ctl(epollfd, EPOLL_CTL_DEL, ev.fd, &ev.event) < 0)
    {
        error_flag = MEVEL_ERR_DEL;
    }
    return (error_flag == MEVEL_ERR_NONE);
}

bool mevel::add_fio(callback_t cb, int fd, int evmask)
{
    mevent              ev;
    ev.type             = MEVEL_TYPE_IO;
    ev.cb               = cb;
    ev.event.events     = evmask;
    ev.fd               = fd;

    return add(ev);
}

bool mevel::add_timer(callback_t cb, int timeout, int period)
{
    mevent              ev;
    ev.type             = MEVEL_TYPE_TIMER;
    ev.cb               = cb;
    ev.event.events     = MEVEL_EDGE | MEVEL_READ;
    ev.fd               = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (ev.fd < 0) throw exception("adding timer failed; file descriptor is negetive", MEVEL_ERR_TIMER);

    struct timespec     ts;
    struct itimerspec   itime;

    ts.tv_sec           = timeout / 1000;
    ts.tv_nsec          = (timeout % 1000) * 1000000;
    itime.it_value      = ts;

    ts.tv_sec           = period / 1000;
    ts.tv_nsec          = (period % 1000) * 1000000;
    itime.it_interval   = ts;

    if (timerfd_settime(ev.fd, 0, &itime, NULL) < 0)
    {
        ::close(ev.fd);
        ev.fd = -1;
        error_flag = MEVEL_ERR_TIMER;
        return false;
    }

    return add(ev);
}

bool mevel::add_signal(callback_t cb, int signum)
{
    error_flag = MEVEL_ERR_SIGNAL;
    if (!cb) return false;

    if (ev_signal.fd < 0)
    {
        sigemptyset(&ev_signal.smask);

        ev_signal.type            = MEVEL_TYPE_SIGNAL;
        ev_signal.event.events    = MEVEL_READ;

        if (sigprocmask(SIG_BLOCK, &ev_signal.smask, NULL) == 0)
        {
            ev_signal.fd          = signalfd(-1, &ev_signal.smask, SFD_NONBLOCK | SFD_CLOEXEC);
            if (ev_signal.fd <= 0)
            {
                ev_signal.fd = -1;
                return false;
            }
        }
        else return false;
    }

    if (eventmap.find(ev_signal.fd) != eventmap.end())
    {
        if (!del(ev_signal)) return false;
    }

    sigaddset(&ev_signal.smask, signum);

    if (sigprocmask(SIG_BLOCK, &ev_signal.smask, NULL) == 0)
    {
        if (signalfd(ev_signal.fd, &ev_signal.smask, SFD_NONBLOCK | SFD_CLOEXEC) != ev_signal.fd)
        {
            return false;
        }
    }
    else return false;

    ev_signal.cb = cb;


    clear_error_flag();
    return add(ev_signal);
}

bool mevel::add_signal(callback_t cb, std::initializer_list<int> signums)
{
    error_flag = MEVEL_ERR_SIGNAL;
    if (!cb) return false;

    for (auto elem : signums)
    {
        if (!add_signal(cb, elem)) return false;
    }

    clear_error_flag();
    return true;
}

bool mevel::add_udp(callback_t cb, int stype, const char* straddr, int port, int evmask)
{
    error_flag          = MEVEL_ERR_UDP;
    mevent              ev;
    ev.type             = MEVEL_TYPE_IO;
    ev.cb               = cb;
    ev.event.events     = evmask;

    if (stype == MEVEL_IPV4 || stype == MEVEL_IPV6)
    {
        struct sockaddr_in  baddr;
        memset(&baddr, 0x00, sizeof(struct sockaddr_in));
        baddr.sin_family    =   stype;
        baddr.sin_port      =   htons(port);
        if (inet_pton(stype, straddr, &baddr.sin_addr) != 1)
        {
            return false;
        }

        ev.fd               =   socket(stype, SOCK_DGRAM, 0);

        if (bind(ev.fd, (struct sockaddr*) &baddr, sizeof(struct sockaddr_in)) < 0)
        {
            if (ev.fd > 0) ::close(ev.fd);
            return false;
        }
    }
    else if (stype == MEVEL_UNIX)
    {
        struct sockaddr_un  baddr;
        baddr.sun_family = AF_UNIX;
        strncpy(baddr.sun_path, straddr, sizeof(baddr.sun_path) - 1);
        ev.fd = socket(AF_UNIX, SOCK_DGRAM, 0);

        if (bind(ev.fd, (struct sockaddr*) &baddr, sizeof(struct sockaddr_un)) < 0)
        {
            if (ev.fd > 0) ::close(ev.fd);
            return false;
        }
    }
    else
    {
        return false;
    }

    clear_error_flag();
    return add(ev);
}

bool mevel::add_tcp(callback_t cb, int stype, const char* straddr, int port, int evmask)
{
    error_flag         = MEVEL_ERR_TCP;
    mevent              ev;
    ev.type            = MEVEL_TYPE_ACC;
    ev.cb              = cb;
    ev.event.events    = MEVEL_READ;
    ev.evmask          = evmask;

    if (stype == MEVEL_IPV4 || stype == MEVEL_IPV6)
    {
        struct sockaddr_in  baddr;
        memset(&baddr, 0x00, sizeof(struct sockaddr_in));
        baddr.sin_family    =   stype;
        baddr.sin_port      =   htons(port);
        if (inet_pton(stype, straddr, &baddr.sin_addr) != 1)
        {
            return false;
        }

        ev.fd               =   socket(stype, SOCK_STREAM, 0);

        if (bind(ev.fd, (struct sockaddr*) &baddr, sizeof(struct sockaddr_in)) < 0)
        {
            if (ev.fd > 0) ::close(ev.fd);
            return false;
        }
    }
    else if (stype == MEVEL_UNIX)
    {
        struct sockaddr_un  baddr;
        baddr.sun_family = AF_UNIX;
        strncpy(baddr.sun_path, straddr, sizeof(baddr.sun_path) - 1);
        ev.fd = socket(AF_UNIX, SOCK_STREAM, 0);

        if (bind(ev.fd, (struct sockaddr*) &baddr, sizeof(struct sockaddr_un)) < 0)
        {
            if (ev.fd > 0) ::close(ev.fd);
            return false;
        }
    }
    else
    {
        return false;
    }


    int flags = 1;

#ifdef O_NONBLOCK
    flags = fcntl(ev.fd, F_GETFL, 0);
    if (flags < 0)
    {
        close(ev.fd);
        return false;
    }

    if (fcntl(ev.fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        close(ev.fd);
        return false;
    }
#else // for those who do not support POSIX
    if (ioctl(ev.fd, FIONBIO, &flags) < 0)
    {
        ::close(ev.fd);
        return NULL;
    }
#endif

    if (listen(ev.fd, MEVEL_MAX_EVENTS) < 0)
    {
        ::close(ev.fd);
    }

    clear_error_flag();
    return add(ev);
}

}
