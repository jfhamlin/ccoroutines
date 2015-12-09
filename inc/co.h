#ifndef _CO_H_
#define _CO_H_

/**
 * Executes the main coroutine. co_main() blocks until f completes.
 */
int co_main(int (*f)(int, char **), int argc, char **argv);

/**
 * Create a new coroutine.
 */
void co(void (*f)(void *), void *arg);

/**
 * Yield execution to the coroutine scheduler.
 */
void co_yield();

#endif
