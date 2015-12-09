#include <stdio.h>
#include "co.h"

#define PRINT_COUNT 2

#define COROUTINE_COUNT 1

// Note that we're not incrementing atomically. We're assuming a
// single OS thread with no yields between load and store.
int done_count = 0;

void print_loop(void *arg) {
  size_t val = (size_t)arg;
  for (int i = 0; i < PRINT_COUNT; ++i) {
    printf("COROUTINE (%zu, %d)\n", val, i);
    co_yield();
    printf("%zu back from yielding\n", val);
  }
  ++done_count;
}

int comain(int argc, char **argv) {
  for (size_t val = 0; val < COROUTINE_COUNT; ++val) {
    co(&print_loop, (void *)val);
  }

  while (done_count < COROUTINE_COUNT) {
    printf("main thread yielding\n");
    co_yield();
  }

  printf("main thread exiting\n");
  return 0;
}

int main(int argc, char **argv) {
  printf("Running test...\n");
  return co_main(&comain, argc, argv);
}
