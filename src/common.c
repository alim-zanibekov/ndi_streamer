// Copyright 2022 Alim Zanibekov
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include "common.h"

#include <libavutil/avutil.h>

void
av_error_fmt(char *out, char *str, int ec)
{
    char error_buf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(error_buf, AV_ERROR_MAX_STRING_SIZE, ec);
    sprintf(out, "%s, (%s)\n", str, error_buf);
}
