// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "util.h"

#include <stdatomic.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
HANDLE mu;
CONDITION_VARIABLE cv;
CRITICAL_SECTION cvl;
#else
pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
#endif

int eh_initialized = 0;
_Atomic(int) eh_got_signal = 0;

typedef struct OPInternalCtx {
    char *short_options;
    struct option *r_options;
    const ProgramOption *p_options;
} OPInternalCtx;

#ifdef _WIN32
BOOL WINAPI
eh_signal_handler(DWORD param)
{
    atomic_store(&eh_got_signal, 1);
    WakeAllConditionVariable(&cv);
    return 1;
}

void
eh_init()
{
    if (!eh_initialized) {
        atomic_store(&eh_got_signal, 0);
        mu = CreateMutex(NULL, FALSE, NULL);
        InitializeConditionVariable(&cv);
        InitializeCriticalSection(&cvl);
        SetConsoleCtrlHandler(&eh_signal_handler, 1);
        eh_initialized = 1;
    }
}

void
eh_wait()
{
    if (!eh_initialized) {
        eh_init();
    }

    WaitForSingleObject(mu, INFINITE);

    while (eh_alive()) {
        SleepConditionVariableCS (&cv, &cvl, INFINITE);
    }
    ReleaseMutex(mu);
}

#else

void
eh_signal_handler(__attribute__((unused)) int _)
{
    atomic_store(&eh_got_signal, 1);
    pthread_cond_signal(&cv);
}

void
eh_init()
{
    if (!eh_initialized) {
        atomic_store(&eh_got_signal, 0);
        struct sigaction action = {};
        action.sa_handler = &eh_signal_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        sigaction(SIGINT, &action, NULL);
        eh_initialized = 1;
    }
}

void
eh_wait()
{
    pthread_mutex_lock(&mu);
    if (!eh_initialized) {
        eh_init();
    }

    while (eh_alive()) {
        pthread_cond_wait(&cv, &mu);
    }
    pthread_mutex_unlock(&mu);
}
#endif

int
eh_alive()
{
    return !atomic_load(&eh_got_signal);
}

OptionParserCtx *
op_init(const ProgramOption *options)
{
    int n, i, j, no_s_args = 0;
    struct option *raw_options;
    char *ptr, *short_options, *name;
    OPInternalCtx *res;

    for (n = 0; options[n].name != NULL; ++n) {
        ptr = strchr(options[n].name, ',');
        if (ptr != NULL) {
            no_s_args += options[n].is_flag ? 1 : 2;
        }
    }
    res = malloc(sizeof(OPInternalCtx));
    raw_options = malloc(sizeof(struct option) * (n + 1));
    memset(raw_options, 0, sizeof(struct option) * (n + 1));
    short_options = malloc(no_s_args + 1);
    j = 0;

    for (i = 0; i < n; ++i) {
        name = options[i].name;
        ptr = strchr(name, ',');
        if (ptr == NULL) {
            raw_options[i].name = name;
        }
        else {
            raw_options[i].name = &ptr[1];
            if (options[i].is_flag) {
                short_options[j++] = name[0];
            }
            else {
                short_options[j++] = name[0];
                short_options[j++] = ':';
            }
        }

        raw_options[i].has_arg
                = options[n].is_flag ? no_argument : required_argument;
        raw_options[i].val = i;
        raw_options[i].flag = NULL;
    }

    short_options[j] = '\0';
    res->r_options = raw_options;
    res->p_options = options;
    res->short_options = short_options;

    return (OptionParserCtx *)res;
}

void
op_free(OptionParserCtx **ctx)
{
    OPInternalCtx *ctx_internal = (OPInternalCtx *)(*ctx);
    free(ctx_internal->r_options);
    free(ctx_internal->short_options);
    free(ctx_internal);
    *ctx = NULL;
}

int
op_parse(int argc, char **argv, const OptionParserCtx *ctx,
         const ProgramOption **res)
{
    OPInternalCtx *ctx_internal = (OPInternalCtx *)ctx;
    int option_index = -1;
    int c = getopt_long(argc, argv, ctx_internal->short_options,
                        ctx_internal->r_options, &option_index);
    *res = NULL;

    if (c < 0)
        return c;

    const ProgramOption *options = ctx_internal->p_options;
    if (option_index == -1) {
        for (int i = 0; options[i].name; ++i) {
            if (strchr(options[i].name, ',') != NULL
                && options[i].name[0] == c) {
                *res = &options[i];
            }
        }

        return c;
    }

    *res = &options[option_index];

    if (strchr(options[option_index].name, ',') != NULL) {
        return options[option_index].name[0];
    }

    return c;
}

void
op_print_help(char *name, const OptionParserCtx *ctx)
{
    OPInternalCtx *ctx_internal = (OPInternalCtx *)ctx;
    const ProgramOption *options = ctx_internal->p_options;

    printf("Usage of %s: \n", name);
    for (int i = 0; options[i].name; ++i) {
        char *ptr = strchr(options[i].name, ',');

        if (ptr == NULL) {
            printf("    --%s: %s\n", options[i].name, options[i].description);
        }
        else {
            printf("    -%c, --%s: %s\n", options[i].name[0], &ptr[1],
                   options[i].description);
        }
    }
}
