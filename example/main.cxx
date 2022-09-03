/*
 *   Copyright 2018 Behrooz Kamary
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

#include <iostream>

#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <mevel.hpp>

mevel::error_en  timer(const mevel::mevent& ev, int flags)
{
    static uint64_t exp = 0;
    static uint64_t tot_exp = 0;

    if (read(ev.fd, &exp, sizeof(uint64_t)) != sizeof(uint64_t)) return mevel::MEVEL_ERR_TIMER;

    tot_exp += exp;
    printf("read: %llu; total=%llu\n", (unsigned long long) exp, (unsigned long long) tot_exp);

    if (tot_exp > 20) return mevel::MEVEL_ERR_STOP;

    return mevel::MEVEL_ERR_NONE;
}

mevel::error_en  sig_handler(const mevel::mevent& ev, int flags)
{
    struct signalfd_siginfo si;
	ssize_t res = read(ev.fd, &si, sizeof(si));

    if (res != sizeof(si))
        return mevel::MEVEL_ERR_NONE;

    printf("\rreceived signal (%d).\n", si.ssi_signo);

    exit(EXIT_SUCCESS);

    return mevel::MEVEL_ERR_NONE;
}

int main (int argc, char** argv)
{
    mevel::mevel evlp;
    evlp.add_timer(timer, 500, 500);
    if (!evlp.add_signal(sig_handler, {SIGTERM, SIGINT, SIGQUIT}))
    {
        return EXIT_FAILURE;
    }

    evlp.run();
    return EXIT_SUCCESS;
}
