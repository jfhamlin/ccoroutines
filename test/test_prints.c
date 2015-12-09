#include <stdio.h>
#include "co.h"

#define PRINT_COUNT 2

#define COROUTINE_COUNT 2

static volatile int done_count = 0;

void print_loop(void *arg) {
  size_t val = (size_t)arg;
  for (int i = 0; i < PRINT_COUNT; ++i) {
    printf("COROUTINE (%zu, %d)\n", val, i);
    co_yield();
  }
  ++done_count;
}

int comain(int argc, char **argv) {
  for (int i = 0; i < COROUTINE_COUNT; ++i) {
    co(&print_loop, i);
  }

  while (done_count < COROUTINE_COUNT) {
    printf("waiting\n");
    co_yield();
  }

  printf("main coroutine exiting\n");
  return 0;
}

int main(int argc, char **argv) {
  printf("Running test...\n");
  return co_main(comain, argc, argv);
}
