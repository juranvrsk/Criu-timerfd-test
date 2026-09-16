#define _GNU_SOURCE
#include "logger.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static FILE *g_log_fp = NULL;

int logger_init(const char *filename)
{
    if (!filename)
        return -1;

    g_log_fp = fopen(filename, "a");
    if (!g_log_fp) {
        fprintf(stderr, "Failed to open log file '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    setbuf(g_log_fp, NULL);
    return 0;
}

void logger_close(void)
{
    if (g_log_fp) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
}

static void log_write(log_level_t level, const char *fmt, va_list ap)
{
    const char *level_str;
    time_t now;
    struct tm tm_info;
    char timebuf[32];
    va_list ap_copy;

    switch (level) {
        case LOG_LEVEL_INFO:  level_str = "INFO";  break;
        case LOG_LEVEL_WARN:  level_str = "WARN";  break;
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        default:              level_str = "UNK";   break;
    }

    now = time(NULL);
    localtime_r(&now, &tm_info);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_info);

    printf("[%s] [%lld] [%s] ", timebuf, log_timestamp(), level_str);
    va_copy(ap_copy, ap);
    vprintf(fmt, ap_copy);
    va_end(ap_copy);
    printf("\n");

    if (g_log_fp) {
        fprintf(g_log_fp, "[%s] [%s] ", timebuf, level_str);
        vfprintf(g_log_fp, fmt, ap);
        fprintf(g_log_fp, "\n");
    }
}

long long log_timestamp(void)
{
    struct timespec ts;
    if(clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
    {
        long long ms_from_sec = (long long)ts.tv_sec * 1000LL;
        long long ms_from_nsec = ts.tv_nsec / 1000000L;
        long long total_ms = ms_from_sec + ms_from_nsec;
        return total_ms;
    }
    return -1;
}



void log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_write(LOG_LEVEL_INFO, fmt, ap);
    va_end(ap);
}

void log_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_write(LOG_LEVEL_WARN, fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_write(LOG_LEVEL_ERROR, fmt, ap);
    va_end(ap);
}

void log_debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_write(LOG_LEVEL_DEBUG, fmt, ap);
    va_end(ap);
}

void log_perror(const char *prefix)
{
    int saved_errno = errno;
    log_error("%s: %s", prefix ? prefix : "", strerror(saved_errno));
}