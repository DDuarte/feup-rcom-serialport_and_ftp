#include "misc.h"
#include <stdarg.h>
#include <stdio.h>

static bool verbose_output = false;

void set_verbose_output(bool v)
{
    verbose_output = v;
}

int print_message(const char* fmt, ...)
{
    if (verbose_output)
    {
        va_list args;
        va_start(args, fmt);
        int ret = printf(fmt, args);
        va_end(args);
        return ret;
    }
    else
        return 0;
}
