// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef UTIL_H
#define UTIL_H
#include <getopt.h>

typedef struct ProgramOption {
    char *name;
    char *description;
    int is_flag;
} ProgramOption;

typedef struct OptionParserCtx OptionParserCtx;

void
eh_init();

int
eh_alive();

void
eh_wait();

OptionParserCtx *
op_init(const ProgramOption *options);

void
op_free(OptionParserCtx **ctx);

int
op_parse(int argc, char **argv, const OptionParserCtx *ctx,
         const ProgramOption **res);

void
op_print_help(char *name, const OptionParserCtx *ctx);

#endif
