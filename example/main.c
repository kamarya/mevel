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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <signal.h>
#include <sys/signalfd.h>
#include "mevel.h"
#include "queue.h"

// curl http://localhost:5251 --data 'this is tcp'
// echo -n "this is udp" > /dev/udp/127.0.0.1/5252

mevel_ctx_t*   ctx;

void cleanup()
{
    mevel_rel(ctx);
}

uint32_t randi(uint32_t min, uint32_t max)
{
    return (rand() % (max - min + 1) + min);
}

void gen_str(char* str, size_t len)
{
    static const uint32_t min = 0x2F;
    static const uint32_t max = 0x7E;

    str[--len] = 0x00;

    while (len > 0)
    {
        str[--len] = (char) (rand() % (max - min + 1) + min);
    }
}

void err(const char* msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}


mevel_err_t  cb_udp(mevel_event_t* ev, int flags)
{
    char buf[1024];
    memset(buf, 0x00, 1024);

    if (read(ev->fd, buf, 1024) > 0)
    {
        printf( "UDP data (%s)\n", buf);
        return MEVEL_ERR_NONE;
    }
    return MEVEL_ERR_CLOSE;
}

mevel_err_t  cb_tcp(mevel_event_t* ev, int flags)
{
    char buf[1024];
    memset(buf, 0x00, 1024);

    if (read(ev->fd, buf, 1024) > 0)
    {
        printf( "TCP data (%s)\n", buf);
        return MEVEL_ERR_CLOSE;
    }
    return MEVEL_ERR_NONE;
}

mevel_err_t  timer(mevel_event_t* ev, int flags)
{
    static uint64_t exp = 0;
    static uint64_t tot_exp = 0;

    if (read(ev->fd, &exp, sizeof(uint64_t)) != sizeof(uint64_t)) return MEVEL_ERR_TIMER;

    tot_exp += exp;
    printf("read: %llu; total=%llu\n", (unsigned long long) exp, (unsigned long long) tot_exp);

    if (tot_exp > 20) return MEVEL_ERR_STOP;// ev->ctx->running = 0;

    return MEVEL_ERR_NONE;
}


mevel_err_t  sig_handle(mevel_event_t* ev, int flags)
{
    struct signalfd_siginfo si;
	ssize_t res = read(ev->fd, &si, sizeof(si));

    if (res != sizeof(si))
        return MEVEL_ERR_SIGNAL;

    printf("\rsignal received (%d).\n", si.ssi_signo);

    cleanup();

    exit(EXIT_SUCCESS);

    return MEVEL_ERR_NONE;
}

int main(int argc, char** argv)
{
    const size_t    tsize = 1e4;
    srand(time(NULL));

    queue_ctx_t* qctx = queue_ini();

    if (!qctx) err("queue_ini() returned null.");

    for (size_t i = 0; i < tsize; i++)
    {
        size_t len = randi(5,30);
        char* str  = (char*)malloc(len);
        if (!str) err("malloc() returned null.");
        str[0] = 0x00;
        gen_str(str, len);
        if (!queue_put(qctx, str)) err("queue_put() returned null.");
    }

    printf("\n");
    queue_t* elm = qctx->head;
    size_t counter = 0;

    while (elm != NULL)
    {
        printf("(%zd) %s\r", counter, (const char*)elm->ptr);
        elm = elm->nxt;
        counter++;
    }

    queue_rel_ptr(qctx);

    printf("\n");

    ctx = mevel_ini();
    if (!ctx) err("mevel_ini()");

    mevel_event_t* tox = mevel_ini_tot(ctx, timer, 500, 500);
    if (tox) mevel_add(ctx,tox);
    else err("mevel_ini_to()");

    if (mevel_add_sig(ctx, sig_handle, 3, SIGTERM, SIGINT, SIGQUIT))
    {
        err("mevel_ad_sig()");
    }



    mevel_event_t* tcpx = mevel_ini_tcp(ctx, cb_tcp, MEVEL_IPV4, "127.0.0.1", 5251, MEVEL_READ);
    if (!tcpx) err("mevel_ini_tcp()");
    if (mevel_add(ctx, tcpx) != MEVEL_ERR_NONE) err("mevel_add()");


    // echo -n "This is my data" > /dev/udp/127.0.0.1/5252
    mevel_event_t* udpx = mevel_ini_udp(ctx, cb_udp, MEVEL_IPV4, "127.0.0.1", 5252, MEVEL_READ);
    if (mevel_add(ctx, udpx) != MEVEL_ERR_NONE) err("mevel_add()");

    mevel_run(ctx);

    cleanup();

    return EXIT_SUCCESS;
}
