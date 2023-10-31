#include "strftime_micro.h"
#include "mem.h"

size_t strftime_micro(char *buf, size_t size, const char *format, const struct timeval *tv)
{
    struct tm *tm;
    char *temp_name = av_malloc(size);
    if (temp_name == NULL)
        return 0;

    if ((tm = localtime(&(tv->tv_sec))) == NULL)
        return 0;

    strftime(temp_name, size, format, tm);
    size_t retval = snprintf(buf, size, temp_name, tv->tv_usec);

    av_free(temp_name);
    return retval;
}