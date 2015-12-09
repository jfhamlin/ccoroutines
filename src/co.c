#define _XOPEN_SOURCE 700
#include <ucontext.h>
#include <stdio.h>

#include "co.h"

/******************************************************************************
 *****************************************************************************/
typedef size_t coid;

typedef struct cor_s {
  coid id;
  void (*f)(void *);
  void *arg;
  ucontext_t ucp;
} cor;

static cor *cor_create(void (*f)(void *), void *arg);
static void cor_destroy(cor *r);
static void cor_start();
static void cor_resume(cor *r);

/* The currently executing coroutine. */
static cor * volatile s_curr = NULL;

static coid s_next_id = 1;

#define STACK_SIZE (64 * 1024)

/******************************************************************************
 *****************************************************************************/

typedef struct conode_s {
  struct conode_s *next;
  struct conode_s *prev;

  cor *r;
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

static costate *s_state = NULL;

/******************************************************************************
 *****************************************************************************/

int co_main(int (*f)(int, char **), int argc, char **argv) {
  if (s_state) {
    return -1;
  }

  costate_create();

  cor *r = cor_create(NULL, NULL);
  s_curr = r;

  int result = f(argc, argv);

  costate_destroy();
  return result;
}

void co(void (*f)(void *), void *arg) {
  cor *r = cor_create(f, arg);
  getcontext(&r->ucp); //TODO: Handle failure
  r->ucp.uc_stack.ss_sp = calloc(1, STACK_SIZE);
  r->ucp.uc_stack.ss_size = STACK_SIZE;
  r->ucp.uc_link = NULL;
  makecontext(&r->ucp, &cor_start, 0);
}

/**
 * Implements a simple round-robin coroutine scheduler.
 */
void co_yield() {
  if (s_curr) {
    fprintf(stderr, "Coroutine %zu yielding\n", s_curr->id);
  }

  conode *n = s_state->sentinel->next;
  cor *r = n->r;
  if (r && r != s_curr) {
    conode_remove(n);
    conode_insert(s_state->sentinel->prev, n);

    cor_resume(r);
  }
}

/******************************************************************************
 *****************************************************************************/

void costate_create() {
  s_state = (costate *)calloc(1, sizeof(costate));
  conode *s = s_state->sentinel = (conode *)calloc(1, sizeof(conode));
  s->next = s;
  s->prev = s;
}

void costate_destroy() {
}

cor *cor_create(void (*f)(void *), void *arg) {
  cor *r = (cor *)calloc(1, sizeof(cor));
  r->id = s_next_id++;
  r->f = f;
  r->arg = arg;

  conode *n = (conode *)calloc(1, sizeof(conode));
  n->r = r;
  conode_insert(s_state->sentinel, n);

  return r;
}

void cor_destroy(cor *r) {
  fprintf(stderr, "Destroying coroutine %zu\n", r->id);

  conode *n = s_state->sentinel->next;
  while (n->r != r) {
    n = n->next;
  }
  conode_remove(n);

  // TODO: Add stack to pool.
  free(r);
  free(n);

  s_curr = NULL;
}

void cor_start() {
  cor *r = s_curr;
  r->f(r->arg);
  cor_destroy(r);
  co_yield();
}

void cor_resume(cor *r) {
  fprintf(stderr, "Switching to coroutine %zu\n", r->id);

  cor *prev_r = s_curr;
  s_curr = r;
  if (!prev_r) {
    setcontext(&r->ucp);
  } else {
    swapcontext(&prev_r->ucp, &r->ucp);
    s_curr = prev_r;
  }
}
