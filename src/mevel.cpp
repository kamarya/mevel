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
#include <sys/signalfd.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdexcept>

#include <mevel.hpp>

namespace mevel
{

mevel::mevel()
: epollfd(0)
, running(0)
, eventmap()
{
    epollfd = epoll_create1(EPOLL_CLOEXEC);

    if (epollfd < 0)
    {
        throw exception("epoll_create1(EPOLL_CLOEXEC) failed.", MEVEL_ERR_CONSTRUCTOR);
    }
}

mevel::~mevel()
{
    if (epollfd > 0) ::close(epollfd);
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
            if (errno == EINTR) throw exception("main event loop hang up failure", MEVEL_ERR_HUP);
            else throw exception("main event loop wait failure", MEVEL_ERR_WAIT);
            running = 0x00;
            break;
        }
        else if (nfds == 0)
        {
            continue;
        }

        for (int indx = 0; indx < nfds; indx++)
        {
            int fd = (int)events[indx].data.fd;
            const mevent& ev = eventmap[fd];
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
                else if (ev.cb)
                {
                    if (ev.cb(ev, events[indx].events) != MEVEL_ERR_NONE)
                    {
                        del(ev);
                    }
                }
                else
                {
                    del(ev);
                }
            }
        }

    }
    return true;
}

void mevel::add(mevent ev)
{
    ev.event.data.fd = ev.fd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev.fd, &ev.event) < 0)
    {
        throw exception("add to the main event loop failed", MEVEL_ERR_ADD);
    }

    eventmap.insert(std::make_pair(ev.fd, ev));
}

void mevel::del(mevent ev)
{
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, ev.fd, &ev.event) < 0)
    {
        throw exception("delete from the main event loop failed", MEVEL_ERR_DEL);
    }

    if (eventmap.find(ev.fd) == eventmap.end())
    {
        throw exception("this event has not been registered", MEVEL_ERR_DEL);
    }

    if (eventmap.erase(ev.fd) != 1)
    {
        throw exception("this event erasure failed", MEVEL_ERR_DEL);
    }

    if (ev.fd > 0) ::close(ev.fd);
}

void mevel::add_fio(callback_t cb, int fd, int evmask)
{
    mevent              ev;
    ev.type             = MEVEL_TYPE_IO;
    ev.cb               = cb;
    ev.event.events     = evmask;
    ev.fd               = fd;

    add(ev);
}

void mevel::add_timer(callback_t cb, int timeout, int period)
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
        throw exception("adding timer failed", MEVEL_ERR_TIMER);
    }

    add(ev);
}

}
