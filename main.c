#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#define TIMER_INTERVAL_SEC 0
#define TIMER_INTERVAL_NSEC 10000000

void timer_callback_signal(union sigval arg) {
  printf("SIGNAL\n");
}

sigset_t sigmask;

int main() {
  timer_t timer_id;
  struct sigevent se;
  struct sigaction sa;
  struct itimerspec ts;

  se.sigev_signo = SIGRTMIN;
  se.sigev_notify = SIGEV_SIGNAL;
  se.sigev_notify_attributes = NULL;
  se.sigev_value.sival_ptr = &timer_id;
  sa.sa_flags = 0;
  sa.sa_handler = (void *)dccthread_yield;

  ts.it_interval.tv_sec = TIMER_INTERVAL_SEC;
  ts.it_interval.tv_nsec = TIMER_INTERVAL_NSEC;
  ts.it_value.tv_sec = TIMER_INTERVAL_SEC;
  ts.it_value.tv_nsec = TIMER_INTERVAL_NSEC;

  sigaction(SIGRTMIN, &sa, NULL);
  timer_create(CLOCK_PROCESS_CPUTIME_ID, &se, &timer_id);
  timer_settime(timer_id, 0, &ts, NULL);

  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGRTMIN);

  // sigprocmask(SIG_BLOCK, &sigmask, NULL);

  if (timer_create(CLOCK_PROCESS_CPUTIME_ID, &se, &timer_id) == -1) {
    perror("Error creating timer");
    exit(EXIT_FAILURE);
  }

  if (timer_settime(timer_id, 0, &ts, NULL) == -1) {
    perror("Error setting timer");
    exit(EXIT_FAILURE);
  }

  printf("Timer started\n");

  sleep(100);

  if (timer_delete(timer_id) == -1) {
    perror("Error deleting timer");
    exit(EXIT_FAILURE);
  }
  // sigprocmask(SIG_UNBLOCK, &sigmask, NULL);

  printf("Timer deleted\n");
  return 0;
}s