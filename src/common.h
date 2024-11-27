// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

void
av_error_fmt(char *out, char *str, int ec);

int64_t
get_current_ts_usec();

#endif
