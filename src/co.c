#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include "co.h"

/******************************************************************************
 *****************************************************************************/
typedef struct coroutine_s {
  size_t id;

  void (*f)(void *);
  void *arg;

  void *stack;

  jmp_buf jmpbuf;

  int started;
} coroutine;

static coroutine *coroutine_create(void (*f)(void *), void *arg);
static void coroutine_destroy(coroutine *r);
static void coroutine_start(coroutine *r);
static void coroutine_resume(coroutine *r);
static inline int coroutine_is_started(coroutine *r) {
  return r->started;
}

#define CURRENT_COROUTINE g_current_coroutine
static coroutine *g_current_coroutine = NULL;

static size_t g_next_id = 1;

/******************************************************************************
 *****************************************************************************/

typedef struct conode_s {
  struct conode_s *next;
  struct conode_s *prev;

  coroutine *r;
} conode;

static inline void conode_remove(conode *n) {
  n->next->prev = n->prev;
  n->prev->next = n->next;
}

static inline void conode_insert(conode *prev, conode *n) {
  n->next = prev->next;
  n->prev = prev;
  prev->next->prev = n;
  prev->next = n;
}

typedef struct costate_s {
  conode *sentinel;
} costate;

static void costate_create();
static void costate_destroy();

static costate *g_state = NULL;

/******************************************************************************
 *****************************************************************************/

int co_main(int (*f)(int, char **), int argc, char **argv) {
  if (g_state) {
    return -1;
  }

  costate_create();

  fprintf(stderr, "Creating main coroutine\n");
  coroutine *r = coroutine_create(NULL, NULL);
  r->started = 1;
  CURRENT_COROUTINE = r;

  int result = f(argc, argv);

  costate_destroy();
  return result;
}

void co(void (*f)(void *), void *arg) {
  coroutine_create(f, arg);
}

/**
 * Implements a simple round-robin coroutine scheduler.
 */
void co_yield() {
  if (CURRENT_COROUTINE) {
    fprintf(stderr, "Coroutine %zu yielding\n", CURRENT_COROUTINE->id);
  }

  conode *n = g_state->sentinel->next;
  coroutine *r = n->r;
  if (r && r != CURRENT_COROUTINE) {
    conode_remove(n);
    conode_insert(g_state->sentinel->prev, n);

    coroutine_resume(r);
  }
}

/******************************************************************************
 *****************************************************************************/

void costate_create() {
  g_state = (costate *)calloc(1, sizeof(costate));
  conode *s = g_state->sentinel = (conode *)calloc(1, sizeof(conode));
  s->next = s;
  s->prev = s;
}

void costate_destroy() {
}

#define STACK_SIZE (2 * 1024 * 1024)
#define SET_STACK(sp)

coroutine *coroutine_create(void (*f)(void *), void *arg) {
  coroutine *r = (coroutine *)calloc(1, sizeof(coroutine));
  r->id = g_next_id++;
  r->f = f;
  r->arg = arg;

  conode *n = (conode *)calloc(1, sizeof(conode));
  n->r = r;
  conode_insert(g_state->sentinel, n);

  fprintf(stderr, "Created coroutine %zu\n", r->id);

  return r;
}

void coroutine_destroy(coroutine *r) {
  fprintf(stderr, "Destroying coroutine %zu\n", r->id);

  conode *n = g_state->sentinel->next;
  while (n->r != r) {
    n = n->next;
  }
  conode_remove(n);

  free(r->stack);
  free(r);
  free(n);

  CURRENT_COROUTINE = NULL;
  co_yield();
}

void coroutine_start(coroutine *r) {
  r->stack = calloc(1, STACK_SIZE);
  r->started = 1;

  SET_STACK(r->stack + STACK_SIZE);
  CURRENT_COROUTINE = r;

  r->f(r->arg);
  coroutine_destroy(r);
}

void coroutine_resume(coroutine *r) {
  coroutine *yr = CURRENT_COROUTINE;
  if (yr && setjmp(yr->jmpbuf)) {
    CURRENT_COROUTINE = yr;
    return;
  }

  if (coroutine_is_started(r)) {
    fprintf(stderr, "Switching to coroutine %zu\n", r->id);
    longjmp(r->jmpbuf, 1);
  } else {
    fprintf(stderr, "Starting coroutine %zu\n", r->id);
    coroutine_start(r);
  }
}
