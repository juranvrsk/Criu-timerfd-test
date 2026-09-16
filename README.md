# Criu-timerfd-test
A test for CRIU timerfd's in value = {0,0} state

Compile: `gcc -Wall -Wextra -o TimerFdTest TimerFdTest.c logger.c`

Run: `./TimerFdTest <initial_time aka value> <timer_period aka interval> <logfile>`