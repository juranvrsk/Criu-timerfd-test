/*
 * TimerFdTest.c
 *
 * Timerfd + log.
 *
 * Arguments:
 *   argv[1] - initial time in ms (it_value),  != 0
 *   argv[2] - timer firing period in ms (it_interval), != 0
 *   argv[3] - path to log file (optional, default is: /tmp/default.log)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/timerfd.h>
#include "logger.h"

/* Global expiriation timer */
static uint64_t g_expirations = 0;

static void ms_to_timespec(unsigned long ms, struct timespec *ts)
{
    ts->tv_sec  = ms / 1000UL;
    ts->tv_nsec = (ms % 1000UL) * 1000000UL;
}

int main(int argc, char *argv[])
{
    int tfd;
    struct itimerspec its;
    unsigned long initial_ms, period_ms;
    const char *logfile;
    uint64_t expirations;
    ssize_t s;

    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <initial_ms> <period_ms> [logfile]\n", argv[0]);
        fprintf(stderr, "  initial_ms - initial time in ms (not 0)\n");
        fprintf(stderr, "  period_ms  - timer firing period in ms (not 0)\n");
        fprintf(stderr, "  logfile    - path to log file (optional, default is: /tmp/default.log)\n");
        return EXIT_FAILURE;
    }

    logfile = (argc == 4) ? argv[3] : "/tmp/default.log";

    if (logger_init(logfile) != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }

    /* arg parse */
    initial_ms = strtoul(argv[1], NULL, 10);
    period_ms  = strtoul(argv[2], NULL, 10);

    /* arg check */
    if (initial_ms == 0) {
        log_error("Initial time (it_value) can't be zero");
        logger_close();
        return EXIT_FAILURE;
    }
    if (period_ms == 0) {
        log_error("Period (it_interval) can't be zero");
        logger_close();
        return EXIT_FAILURE;
    }

    tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        log_perror("timerfd_create");
        logger_close();
        return EXIT_FAILURE;
    }

    memset(&its, 0, sizeof(its));
    ms_to_timespec(initial_ms, &its.it_value);
    ms_to_timespec(period_ms,  &its.it_interval);

    log_info("TimerFdTest started:");
    log_info("  Initial time: %lu ms (%ld s %ld ns)",
             initial_ms, its.it_value.tv_sec, its.it_value.tv_nsec);
    log_info("  Period:          %lu ms (%ld s %ld ns)",
             period_ms, its.it_interval.tv_sec, its.it_interval.tv_nsec);
    log_info("  Log:        %s", logfile);
    log_info("  Waiting for timer firing...");

    if (timerfd_settime(tfd, 0, &its, NULL) == -1) {
        log_perror("timerfd_settime");
        close(tfd);
        logger_close();
        return EXIT_FAILURE;
    }

    /* Rearm using read */
    for (;;) {
        s = read(tfd, &expirations, sizeof(expirations));
        if (s != sizeof(expirations)) {
            log_perror("read");
            close(tfd);
            logger_close();
            return EXIT_FAILURE;
        }

        g_expirations += expirations;

        log_info("Timer fired! expirations (this time): %lu, "
                 "overall expiriations (g_expirations): %lu",
                 (unsigned long)expirations, (unsigned long)g_expirations);
    }

    /* Press ctrl+c to stop */
    close(tfd);
    logger_close();
    return EXIT_SUCCESS;
}