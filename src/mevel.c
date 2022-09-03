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

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/un.h>

#include "mevel.h"


mevel_ctx_t* mevel_ini()
{
    mevel_ctx_t* ctx = (mevel_ctx_t*) malloc(sizeof(mevel_ctx_t));

    if (ctx)
    {
	    ctx->epollfd = epoll_create1(EPOLL_CLOEXEC);

	    if (ctx->epollfd < 0)
	    {
	        free(ctx);
	        ctx = NULL;
	    }
    }

    if (ctx)
    {
        ctx->qctx = queue_ini();
        if (ctx->qctx == NULL)
        {
	        free(ctx);
	        ctx = NULL;
        }
    }


    return ctx;
}

void    mevel_rel(mevel_ctx_t* ctx)
{

    if (ctx)
    {
        queue_rel_ptr(ctx->qctx);

        if (ctx->epollfd > 0) close(ctx->epollfd);
        free(ctx);
        ctx = NULL;
    }
}

mevel_err_t mevel_run(mevel_ctx_t* ctx)
{

    if (ctx == NULL) return MEVEL_ERR_NULL;

    struct sockaddr_storage     peer;
    socklen_t                   plen;

    mevel_err_t     ret = MEVEL_ERR_NONE;
    int nfds            = 0;
    int timeout         = MEVEL_MAX_TIMEOUT;
	epoll_event_t  events[MEVEL_MAX_EVENTS];

	ctx->running = 0xFF;

    while (ctx->running)
    {

		nfds = epoll_wait(ctx->epollfd, events, MEVEL_MAX_EVENTS, timeout);

        if (nfds < 0)
        {
            if (errno == EINTR) ret = MEVEL_ERR_HUP;
            else ret = MEVEL_ERR_WAIT;
            ctx->running = 0x00;
            break;
        }
        else if (nfds == 0)
        {
            continue;
        }

        for (int indx = 0; indx < nfds; indx++)
        {
            mevel_event_t* ev = (mevel_event_t*)events[indx].data.ptr;
            if (ev == NULL || events[indx].events == 0) continue;
            if (ev->cb != NULL && ev->fd > 0)
            {
                if (ev->type == MEVEL_TYPE_ACC)
                {
                    int fd = accept4(ev->fd, (struct sockaddr*)&peer, &plen, SOCK_NONBLOCK);
                    if (fd > 0)
                    {
                        mevel_add(ctx, mevel_ini_fio(ctx, ev->cb, fd, ev->evmask));
                    }
                }

                mevel_err_t cbr = ev->cb(ev, events[indx].events);
                if (cbr != MEVEL_ERR_NONE)
                {
                    mevel_del(ctx, ev);
                    if (cbr == MEVEL_ERR_CLOSE && ev->fd > 0) close(ev->fd);
                }
            }
        }
    }

    return ret;

}


mevel_err_t     mevel_add(mevel_ctx_t* ctx, mevel_event_t* ev)
{
    mevel_err_t     ret = MEVEL_ERR_NULL;

    if (ctx != NULL && ev != NULL)
    {
        ev->event.data.ptr = (void*) ev;
		if (epoll_ctl(ctx->epollfd, EPOLL_CTL_ADD, ev->fd, &ev->event) < 0)
		{
		    ret = MEVEL_ERR_ADD;
		}
		else
		{
		    queue_put(ctx->qctx, ev);
		    ret = MEVEL_ERR_NONE;
		}
    }

    return ret;
}

mevel_err_t     mevel_del(mevel_ctx_t* ctx, mevel_event_t* ev)
{
    mevel_err_t     ret = MEVEL_ERR_NONE;

    if (ctx != NULL && ev != NULL)
    {
		if (epoll_ctl(ctx->epollfd, EPOLL_CTL_DEL, ev->fd, &ev->event) < 0)
		{
		    ret = MEVEL_ERR_DEL;
		}

        if (ev->fd > 0) close(ev->fd);
        queue_del_ptr(ctx->qctx, ev);
    }
    else ret = MEVEL_ERR_NULL;

    return ret;
}


mevel_event_t*  mevel_ini_fio(mevel_ctx_t* ctx, mevel_cb_t cb, int fd, int evmask)
{
    mevel_event_t* ev = (mevel_event_t*) malloc(sizeof(mevel_event_t));

    if (ev)
    {
        ev->ctx             = ctx;
        ev->type            = MEVEL_TYPE_IO;
        ev->fd              = fd;
        ev->cb              = cb;
        ev->event.events    = evmask;
    }

    return ev;
}


mevel_event_t*  mevel_ini_timer(mevel_ctx_t* ctx, mevel_cb_t cb, int timeout, int period)
{
    if (cb == NULL) return NULL;

    mevel_event_t* ev = (mevel_event_t*) malloc(sizeof(mevel_event_t));

    if (ev)
    {
        ev->ctx             = ctx;
        ev->type            = MEVEL_TYPE_TIMER;
        ev->cb              = cb;
        ev->event.events    = MEVEL_EDGE | MEVEL_READ;
        ev->fd              = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

        if (ev->fd > 0)
        {
            struct timespec     ts;
            struct itimerspec   itime;

            ts.tv_sec           = timeout / 1000;
            ts.tv_nsec          = (timeout % 1000) * 1000000;
            itime.it_value      = ts;

            ts.tv_sec           = period / 1000;
            ts.tv_nsec          = (period % 1000) * 1000000;
            itime.it_interval   = ts;

            if (timerfd_settime(ev->fd, 0, &itime, NULL) < 0)
            {
                close(ev->fd);
                ev->fd = -1;
            }
        }

        if (ev->fd < 0)
        {
            free(ev);
            ev = NULL;
        }
    }

    return ev;
}


mevel_event_t*  mevel_ini_tcp(mevel_ctx_t* ctx, mevel_cb_t cb, int stype, const char* straddr, int port, int evmask)
{
    if (straddr == NULL || cb == NULL || straddr[0] == '\0') return NULL;

    mevel_event_t* ev = (mevel_event_t*) malloc(sizeof(mevel_event_t));

    if (ev == NULL) return NULL;

    ev->ctx             = ctx;
    ev->type            = MEVEL_TYPE_ACC;
    ev->cb              = cb;
    ev->event.events    = MEVEL_READ;
    ev->evmask          = evmask;

    if (stype == MEVEL_IPV4 || stype == MEVEL_IPV6)
    {
        struct sockaddr_in  baddr;
        memset(&baddr, 0x00, sizeof(struct sockaddr_in));
        baddr.sin_family    =   stype;
        baddr.sin_port      =   htons(port);
        if (inet_pton(stype, straddr, &baddr.sin_addr) != 1)
        {
            free(ev);
            return NULL;
        }

        ev->fd              =   socket(stype, SOCK_STREAM, 0);

        if (bind(ev->fd, (struct sockaddr*) &baddr, sizeof(struct sockaddr_in)) < 0)
        {
            if (ev->fd > 0) close(ev->fd);
            free(ev);
            return NULL;
        }
    }
    else if (stype == MEVEL_UNIX)
    {
        struct sockaddr_un  baddr;
        baddr.sun_family = AF_UNIX;
        strncpy(baddr.sun_path, straddr, sizeof(baddr.sun_path) - 1);
        ev->fd = socket(AF_UNIX, SOCK_STREAM, 0);

        if (bind(ev->fd, (struct sockaddr*) &baddr, sizeof(struct sockaddr_un)) < 0)
        {
            if (ev->fd > 0) close(ev->fd);
            free(ev);
            return NULL;
        }
    }
    else
    {
        free(ev);
        return NULL;
    }


    int flags = 1;

#ifdef O_NONBLOCK
    flags = fcntl(ev->fd, F_GETFL, 0);
    if (flags < 0)
    {
        close(ev->fd);
        free(ev);
        return NULL;
    }

    if (fcntl(ev->fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        close(ev->fd);
        free(ev);
        return NULL;
    }
#else // for those who do not support POSIX
    if (ioctl(ev->fd, FIONBIO, &flags) < 0)
    {
        close(ev->fd);
        free(ev);
        return NULL;
    }
#endif

    if (listen(ev->fd, MEVEL_MAX_EVENTS) < 0)
    {
        close(ev->fd);
        free(ev);
        ev = NULL;
    }

    return ev;
}

mevel_event_t*  mevel_ini_udp(mevel_ctx_t* ctx, mevel_cb_t cb, int stype, const char* straddr, int port, int evmask)
{
    if (straddr == NULL || cb == NULL || straddr[0] == '\0') return NULL;

    mevel_event_t* ev = (mevel_event_t*) malloc(sizeof(mevel_event_t));

    if (ev == NULL) return NULL;

    ev->ctx             = ctx;
    ev->type            = MEVEL_TYPE_IO;
    ev->cb              = cb;
    ev->event.events    = evmask;

    if (stype == MEVEL_IPV4 || stype == MEVEL_IPV6)
    {
        struct sockaddr_in  baddr;
        memset(&baddr, 0x00, sizeof(struct sockaddr_in));
        baddr.sin_family    =   stype;
        baddr.sin_port      =   htons(port);
        if (inet_pton(stype, straddr, &baddr.sin_addr) != 1)
        {
            free(ev);
            return NULL;
        }

        ev->fd              =   socket(stype, SOCK_DGRAM, 0);

        if (bind(ev->fd, (struct sockaddr*) &baddr, sizeof(struct sockaddr_in)) < 0)
        {
            if (ev->fd > 0) close(ev->fd);
            free(ev);
            return NULL;
        }
    }
    else if (stype == MEVEL_UNIX)
    {
        struct sockaddr_un  baddr;
        baddr.sun_family = AF_UNIX;
        strncpy(baddr.sun_path, straddr, sizeof(baddr.sun_path) - 1);
        ev->fd = socket(AF_UNIX, SOCK_DGRAM, 0);

        if (bind(ev->fd, (struct sockaddr*) &baddr, sizeof(struct sockaddr_un)) < 0)
        {
            if (ev->fd > 0) close(ev->fd);
            free(ev);
            return NULL;
        }
    }
    else
    {
        free(ev);
        return NULL;
    }


    return ev;
}

mevel_event_t*  mevel_ini_sig(mevel_ctx_t* ctx, mevel_cb_t cb)
{
    mevel_event_t* ev = (mevel_event_t*) malloc(sizeof(mevel_event_t));

    if (ev == NULL) return NULL;

    sigemptyset(&ev->smask);

    ev->ctx             = ctx;
    ev->type            = MEVEL_TYPE_SIGNAL;
    ev->event.events    = MEVEL_READ;
    ev->cb              = cb;
    ev->fd              = 0;

    if (sigprocmask(SIG_BLOCK, &ev->smask, NULL) == 0)
    {
        ev->fd          = signalfd(-1, &ev->smask, SFD_NONBLOCK | SFD_CLOEXEC);
	}

    if (ev->fd <= 0)
    {
        free(ev);
        ev = NULL;
    }

    return ev;
}


mevel_err_t mevel_ini_sig_add(mevel_event_t* ev, int signum)
{
    mevel_err_t ret = MEVEL_ERR_SIGNAL;

    if (ev && ev->type == MEVEL_TYPE_SIGNAL && ev->fd > 0)
    {
        sigaddset(&ev->smask, signum);

        if (sigprocmask(SIG_BLOCK, &ev->smask, NULL) == 0)
        {
            if (signalfd(ev->fd, &ev->smask, SFD_NONBLOCK | SFD_CLOEXEC) == ev->fd)
            {
                ret = MEVEL_ERR_NONE;
            }
        }
    }

    return ret;
}

mevel_err_t  mevel_add_fio(mevel_ctx_t* ctx, mevel_cb_t cb, int fd, int evmask)
{
    mevel_event_t* event = mevel_ini_fio(ctx, cb, fd, evmask);
    if (!event) return MEVEL_ERR_FIO;

    return mevel_add(ctx, event);
}

mevel_err_t  mevel_add_timer(mevel_ctx_t* ctx, mevel_cb_t cb, int timeout, int period)
{
    mevel_event_t* event = mevel_ini_timer(ctx, cb, timeout, period);
    if (!event) return MEVEL_ERR_TIMER;

    return mevel_add(ctx, event);
}

mevel_err_t  mevel_add_tcp(mevel_ctx_t* ctx, mevel_cb_t cb, int stype, const char* straddr, int port, int evmask)
{
    mevel_event_t* event = mevel_ini_tcp(ctx, cb, stype, straddr, port, evmask);
    if (!event) return MEVEL_ERR_TCP;

    return mevel_add(ctx, event);
}

mevel_err_t  mevel_add_udp(mevel_ctx_t* ctx, mevel_cb_t cb, int stype, const char* straddr, int port, int evmask)
{
    mevel_event_t* event = mevel_ini_udp(ctx, cb, stype, straddr, port, evmask);
    if (!event) return MEVEL_ERR_UDP;

    return mevel_add(ctx, event);
}

mevel_err_t  mevel_add_sig(mevel_ctx_t* ctx, mevel_cb_t cb, int count, ...)
{
    if (count < 1) return MEVEL_ERR_SIGNAL;

    mevel_event_t* event = mevel_ini_sig(ctx, cb);
    if (!event) return MEVEL_ERR_SIGNAL;

    mevel_err_t ret = MEVEL_ERR_NONE;
    va_list argp;
    va_start(argp, count);
    for (int i = 0; i < count; i++)
    {
        int sig = va_arg(argp, int);
        if (mevel_ini_sig_add(event, sig) != MEVEL_ERR_NONE)
        {
            ret = MEVEL_ERR_SIGNAL;
            break;
        }
    }
    va_end(argp);

    if (ret)
    {
        close(event->fd);
        free(event);
        return ret;
    }

    return mevel_add(ctx, event);
}
