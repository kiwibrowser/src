/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file contains the Read-Eval-Print loop.  It defines the test
 * language embedded in the s-expressions.
 */

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> /* portability.h */

#define MAX_THREADS 1024
#define MAX_FILES   1024
#define MIN_EPSILON_DELAY_NANOS 1000

#include "native_client/tests/lock_manager/nacl_test_util_repl.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/posix/nacl_file_lock.h"
#include "native_client/src/shared/platform/posix/nacl_file_lock_intern.h"
#include "native_client/tests/lock_manager/nacl_test_util_sexp.h"

/*
 * To test the NaClFileLockManager object's API, we have to invoke the
 * API from multiple threads -- since the NaClFileLockManagerLock
 * function can block -- and ensure that locking and unlocking happens
 * in an acceptable order.  The output sequence -- "output" occurs
 * when a lock is taken or released, i.e., at lock state transitions
 * -- can be serialized, but the test will be inherently racy, e.g.,
 * dropping a lock in one thread while two threads are already blocked
 * attempting to get the lock has two valid/acceptable output
 * sequences.  Instead of handling this, we avoid the mess: the test
 * driver will be multithreaded, but will (attempt to) coordinate the
 * threads so that only one operation is done by one thread at any
 * particular time slot -- the sequence of operations will be strictly
 * sequential.  Because threads must acknowledge that they are about
 * to perform the requested operation before they actually do it, we
 * introduce a microsleep testing quantum in the execution of the test
 * machine, so that the next test step should not proceed prior to the
 * thread actually having done its task, even if the task would not
 * cause any events to fire.
 *
 * To handle the possibility that two or more threads will wake up at
 * the same time -- and as a result, output "wakeup" messages -- the
 * output matcher accepts a specification that is petri-net-like: the
 * expected output can be a list of outputs which all must occur, but
 * may occur in any order.  To handle races, alternation must be
 * possible.  This is done using a matcher-of-matcher that returns the
 * index of the matcher that succeeded, in combination with nth,
 * quote, and eval to run the continuation for the match.  See
 * test_input.txt for one example.
 *
 * The lisp-like execution environment does not include any bindings
 * for variables, so the static vs dynamic scoping question does not
 * apply.  If/when we include parametric matching, this will have to
 * be addressed, in addition to deciding on how the bound variables
 * are made available to the match's continuation.
 *
 * There is no garbage collection.  Instead, we copy lists (too much)
 * to avoid leaking memory.
 */

static int gVerbosity = 0;
static int gInteractive = 0;  /* do not crash on error if set */
static size_t gEpsilonDelayNanos = 0;

static pthread_key_t gNaClTestTsdKey;

#define DPRINTF(args)                           \
  do {                                          \
    if (gVerbosity > 1) {                       \
      printf args;                              \
    }                                           \
  } while (0)


struct ThreadState;

enum EventType {
  kLocked = 0,
  kUnlocked = 1,
};

struct Event {
  struct Event *next;
  enum EventType type;
  int thread_id;
  int file_id;
};

struct WorkState {
  /* initialized before going threaded */
  size_t num_workers;
  struct ThreadState *workers;

  /* protects rest */
  pthread_mutex_t mu;
  pthread_cond_t cv;

  int actor_thread;
#define WORK_PENDING  (-1)
#define WORK_ACCEPTED (-2)
#define WORK_ENDED    (-3)
  void (*action_functor)(void *functor_state);
  void *functor_state;

  struct Event *q, **qend;

  struct NaClFileLockManager flm;
  struct NaClFileLockTestInterface *test_if;

  void (*orig_set_identity)(struct NaClFileLockEntry *, int desc);
  void (*orig_take_lock)(int desc);
  void (*orig_drop_lock)(int desc);
};

/*
 * Error / abort handling function.  We do not grab locks here, since
 * this may be called in an error context where the lock is already
 * held.  Since this is a debugging aid, we tolerate the possibility
 * of inconsistent / undefined behavior.
 */
static void NaClReplDumpWs(struct WorkState *ws) {
  struct Event *q = ws->q;
  while (NULL != q) {
    printf("event type %d, tid %d, fid %d\n",
           q->type, q->thread_id, q->file_id);
    q = q->next;
  }
}

static void crash_node(struct WorkState *ws,
                       char const *reason,
                       struct NaClSexpNode *n) {
  fprintf(stderr, "Runtime error: %s\n", reason);
  NaClSexpPrintNode(stderr, n);
  putc('\n', stderr);
  NaClReplDumpWs(ws);
  exit(1);
}

static void crash_cons(struct WorkState *ws,
                       char const *reason,
                       struct NaClSexpCons *c) {
  fprintf(stderr, "Runtime error: %s\n", reason);
  NaClSexpPrintCons(stderr, c);
  putc('\n', stderr);
  NaClReplDumpWs(ws);
  exit(1);
}

static void error_node(struct WorkState *ws,
                       char const *reason,
                       struct NaClSexpNode *n) {
  printf("Error: %s\n", reason);
  NaClSexpPrintNode(stdout, n);
  putchar('\n');
  NaClReplDumpWs(ws);
  /*
   * Eventually, with proper garbage collection, we may distinguish
   * between batch execution and interactive execution, with errors
   * aborting batch execution but letting errors longjmp back to the
   * REPL for interactive use.
   */
  if (!gInteractive) {
    exit(1);
  }
}

static void error_cons(struct WorkState *ws,
                       char const *reason,
                       struct NaClSexpCons *c) {
  printf("Error: %s\n", reason);
  NaClSexpPrintCons(stdout, c);
  putchar('\n');
  NaClReplDumpWs(ws);
  if (!gInteractive) {
    exit(1);
  }
}

void EventOccurred(struct ThreadState *ts, enum EventType type, int file_id);

struct ThreadState {
  pthread_t tid;
  int this_thread;
  struct WorkState *ws;
};

static void NaClFileLockTestSetFileIdentityDataWrapper(
    struct NaClFileLockEntry *entry,
    int desc) {
  struct ThreadState *ts;

  ts = (struct ThreadState *) pthread_getspecific(gNaClTestTsdKey);
  CHECK(NULL != ts);
  (*ts->ws->test_if->set_identity)(ts->ws->test_if,
                                   ts->ws->orig_set_identity,
                                   entry, desc);
}

static void NaClFileLockTestTakeFileLockWrapper(int desc) {
  struct ThreadState *ts;

  ts = (struct ThreadState *) pthread_getspecific(gNaClTestTsdKey);
  CHECK(NULL != ts);
  if ((*ts->ws->test_if->take_lock)(ts->ws->test_if,
                                    ts->ws->orig_take_lock,
                                    ts->this_thread, desc)) {
    EventOccurred(ts, kLocked, desc);
  }
}

static void NaClFileLockTestDropFileLockWrapper(int desc) {
  struct ThreadState *ts;

  ts = (struct ThreadState *) pthread_getspecific(gNaClTestTsdKey);
  CHECK(NULL != ts);
  if ((*ts->ws->test_if->drop_lock)(ts->ws->test_if,
                                    ts->ws->orig_drop_lock,
                                    ts->this_thread, desc)) {
    EventOccurred(ts, kUnlocked, desc);
  }
}

void WorkStateCtor(struct WorkState *ws,
                   struct NaClFileLockTestInterface *test_if) {
  ws->num_workers = 0;  /* unknown */
  ws->workers = NULL;
  pthread_mutex_init(&ws->mu, (pthread_mutexattr_t *) NULL);
  pthread_cond_init(&ws->cv, (pthread_condattr_t *) NULL);
  ws->q = NULL;
  ws->qend = &ws->q;
  ws->actor_thread = WORK_PENDING;
  NaClFileLockManagerCtor(&ws->flm);
  ws->orig_set_identity = ws->flm.set_file_identity_data;
  ws->orig_take_lock = ws->flm.take_file_lock;
  ws->orig_drop_lock = ws->flm.drop_file_lock;
  ws->test_if = test_if;
  ws->flm.set_file_identity_data = NaClFileLockTestSetFileIdentityDataWrapper;
  ws->flm.take_file_lock = NaClFileLockTestTakeFileLockWrapper;
  ws->flm.drop_file_lock = NaClFileLockTestDropFileLockWrapper;
}

void WorkStateDtor(struct WorkState *ws) {
  struct Event *p;
  free(ws->workers);
  pthread_mutex_destroy(&ws->mu);
  pthread_cond_destroy(&ws->cv);
  while (NULL != (p = ws->q)) {
    ws->q = p->next;
    free(p);
  }
  NaClFileLockManagerDtor(&ws->flm);
}

size_t EventQueueLengthMu(struct WorkState *ws) {
  struct Event *p;
  size_t count = 0;
  for (p = ws->q; NULL != p; p = p->next) {
    ++count;
  }
  return count;
}

void EnqueueEvent(struct WorkState *ws, struct Event *ev) {
  DPRINTF(("EnqueueEvent: %d %d %d\n", ev->type, ev->thread_id, ev->file_id));
  pthread_mutex_lock(&ws->mu);
  ev->next = NULL;
  *ws->qend = ev;
  ws->qend = &ev->next;
  pthread_cond_broadcast(&ws->cv);
  DPRINTF(("EventQueueLength -> %d\n", (int) EventQueueLengthMu(ws)));
  pthread_mutex_unlock(&ws->mu);
}

void ComputeAbsTimeout(struct timespec *tspec, int timeout_nanoseconds) {
  struct timeval now;

  gettimeofday(&now, (struct timezone *) NULL);
  tspec->tv_sec = now.tv_sec;
  tspec->tv_nsec = now.tv_usec * 1000 + timeout_nanoseconds;
  if (tspec->tv_nsec > 1000000000) {
    ++tspec->tv_sec;
    tspec->tv_nsec -= 1000000000;
  }
}

/*
 * WaitForEventMu: wait for event, with a timeout. May spuriously
 * return without timing out.  Returns 0 on timeout.
 */
int WaitForEventMu(struct WorkState *ws, struct timespec *abs_timeout) {
  int timed_out_failure = 0;

  if (NULL != abs_timeout) {
    if (pthread_cond_timedwait(&ws->cv, &ws->mu, abs_timeout) == ETIMEDOUT) {
      timed_out_failure = 1;
    }
  } else {
    int err;

    if (0 != (err = pthread_cond_wait(&ws->cv, &ws->mu))) {
      fprintf(stderr, "WaitForEventMu: error %d\n", err);
    }
  }

  return !timed_out_failure;
}

struct Event *EventFactory(enum EventType type, int thread_id, int file_id) {
  struct Event *ev = malloc(sizeof *ev);
  CHECK(NULL != ev);
  ev->next = NULL;
  ev->type = type;
  ev->thread_id = thread_id;
  ev->file_id = file_id;
  DPRINTF(("EventFactory(%d, %d, %d)\n", type, thread_id, file_id));
  return ev;
}

void EventOccurred(struct ThreadState *ts, enum EventType type, int file_id) {
  struct Event *ev = EventFactory(type, ts->this_thread, file_id);
  EnqueueEvent(ts->ws, ev);
}

void EventQueueDiscardMu(struct WorkState *ws) {
  struct Event *p;
  struct Event *rest;

  DPRINTF(("EventQueueDiscardMu\n"));
  for (p = ws->q; p != NULL; p = rest) {
    rest = p->next;
    free(p);
  }
  ws->q = NULL;
  ws->qend = &ws->q;
}

int GetWork(struct ThreadState *ts,
            void (**functor_out)(void *functor_state),
            void **functor_state_out) {
  int actor_thread;
  int has_work;

  DPRINTF(("GetWork(%d)\n", ts->this_thread));
  pthread_mutex_lock(&ts->ws->mu);
  while (WORK_ENDED != (actor_thread = ts->ws->actor_thread) &&
         actor_thread != ts->this_thread) {
    DPRINTF(("GetWork(%d) waiting\n", ts->this_thread));
    pthread_cond_wait(&ts->ws->cv, &ts->ws->mu);
  }
  has_work = (actor_thread == ts->this_thread);
  if (has_work) {
    *functor_out = ts->ws->action_functor;
    *functor_state_out = ts->ws->functor_state;
    ts->ws->actor_thread = WORK_ACCEPTED;
  }
  pthread_mutex_unlock(&ts->ws->mu);
  DPRINTF(("GetWork(%d) returning %d\n", ts->this_thread, has_work));
  return has_work;
}

void PutWork(struct WorkState *ws, int actor_thread,
             void (*action_functor)(void *functor_state),
             void *functor_state) {
  DPRINTF(("PutWork(thread=%d)\n", actor_thread));
  pthread_mutex_lock(&ws->mu);
  while (ws->actor_thread != WORK_PENDING &&
         ws->actor_thread != WORK_ACCEPTED) {
    pthread_cond_wait(&ws->cv, &ws->mu);
  }
  ws->actor_thread = actor_thread;
  ws->action_functor = action_functor;
  ws->functor_state = functor_state;
  pthread_cond_broadcast(&ws->cv);
  pthread_mutex_unlock(&ws->mu);
  DPRINTF(("PutWork(thread=%d) done\n", actor_thread));
}

void WaitForWorkAcceptance(struct WorkState *ws) {
  pthread_mutex_lock(&ws->mu);
  while (ws->actor_thread != WORK_ACCEPTED) {
    pthread_cond_wait(&ws->cv, &ws->mu);
  }
  pthread_mutex_unlock(&ws->mu);
}

void AnnounceEndOfWork(struct WorkState *ws) {
  DPRINTF(("AnnounceEndOfWork\n"));
  pthread_mutex_lock(&ws->mu);
  /*
   * ensure all previous work is accepted, allowing for no-work programs.
   */
  while (ws->actor_thread != WORK_ACCEPTED &&
         ws->actor_thread != WORK_PENDING) {
    DPRINTF(("AnnounceEndOfWork: waiting for work to drain\n"));
    pthread_cond_wait(&ws->cv, &ws->mu);
  }
  ws->actor_thread = WORK_ENDED;
  DPRINTF(("AnnounceEndOfWork: broadcast!\n"));
  pthread_cond_broadcast(&ws->cv);
  pthread_mutex_unlock(&ws->mu);
}

void *ThreadFunc(void *vstate) {
  struct ThreadState *ts = (struct ThreadState *) vstate;
  void (*functor)(void *functor_state);
  void *functor_state;

  /* set ts in TSD */
  pthread_setspecific(gNaClTestTsdKey, ts);

  DPRINTF(("thread %d\n", ts->this_thread));
  while (GetWork(ts, &functor, &functor_state)) {
    (*functor)(functor_state);
  }
  return NULL;
}

void SpawnThreadsMu(struct WorkState *ws, size_t num_threads) {
  size_t ix;
  int err;

  CHECK(0 < num_threads);
  ws->num_workers = num_threads;
  ws->workers = malloc(num_threads * sizeof *ws->workers);
  CHECK(NULL != ws->workers);

  for (ix = 0; ix < num_threads; ++ix) {
    ws->workers[ix].this_thread = (int) ix;
    ws->workers[ix].ws = ws;
    if (0 != (err = pthread_create(&ws->workers[ix].tid,
                                   (pthread_attr_t *) NULL,
                                   ThreadFunc, (void *) &ws->workers[ix]))) {
      fprintf(stderr, "nacl_file_lock_test: pthread_create failed, %d\n", err);
      exit(1);
    }
  }
}

#define CHECK_PROG(ws, expr, cons)                                       \
  do {                                                                  \
    if (!(expr)) {                                                      \
      fprintf(stderr,                                                   \
              "nacl_file_lock_test: %s does not hold at program:\n",    \
              #expr);                                                   \
      crash_cons(ws, "internal error", cons);                            \
    }                                                                   \
  } while (0)

void CheckStateMu(struct NaClSexpCons *cons, struct WorkState *ws) {
  CHECK_PROG(ws, ws->num_workers != 0, cons);
}

void CheckState(struct NaClSexpCons *cons, struct WorkState *ws) {
  pthread_mutex_lock(&ws->mu);
  CheckStateMu(cons, ws);
  pthread_mutex_unlock(&ws->mu);
}

void ReapThreads(struct WorkState *ws) {
  size_t ix;

  if (ws->num_workers > 0) {
    for (ix = 0; ix < ws->num_workers; ++ix) {
      pthread_join(ws->workers[ix].tid, (void **) NULL);
    }
  }
}

struct NaClSexpNode *Eval(struct NaClSexpNode *n, struct WorkState *ws);

struct NaClSexpNode *SetThreadsImpl(struct NaClSexpCons *cons,
                                    struct WorkState *ws) {
  int num_threads;

  if (NaClSexpListLength(cons) != 2) {
    crash_cons(ws, "set-threads should have 1 argument", cons);
  }
  if (!NaClSexpIntp(cons->cdr->car)) {
    crash_cons(ws, "set-threads should have 1 numeric argument", cons);
  }
  num_threads = NaClSexpNodeToInt(cons->cdr->car);
  if (num_threads < 1) {
    crash_cons(ws, "program specifies too few threads", cons);
  }
  if (num_threads >= MAX_THREADS) {
    crash_cons(ws, "program specifies too many threads", cons);
  }
  pthread_mutex_lock(&ws->mu);
  CHECK_PROG(ws, ws->num_workers == 0, cons);
  SpawnThreadsMu(ws, num_threads);
  pthread_mutex_unlock(&ws->mu);
  return NaClSexpNodeWrapInt(num_threads);
}

struct NaClSexpNode *SetFilesImpl(struct NaClSexpCons *cons,
                                  struct WorkState *ws) {
  int num_files;

  if (NaClSexpListLength(cons) != 2) {
    crash_cons(ws, "set-files should have 1 argument", cons);
  }
  if (!NaClSexpIntp(cons->cdr->car)) {
    crash_cons(ws, "set-files should have 1 numeric argument", cons);
  }
  num_files = NaClSexpNodeToInt(cons->cdr->car);
  if (num_files < 1) {
    crash_cons(ws, "program specifies too few files", cons);
  }
  if (num_files >= MAX_FILES) {
    crash_cons(ws, "program specifies too many files", cons);
  }

  if (!(*ws->test_if->set_num_files)(ws->test_if, num_files)) {
    crash_cons(ws, "set-files error", cons);
  }

  return NaClSexpNodeWrapInt(num_files);
}

struct NaClSexpNode *QuoteImpl(struct NaClSexpCons *cons,
                               struct WorkState *ws) {
  UNREFERENCED_PARAMETER(ws);
  if (NaClSexpListLength(cons) != 2) {
    error_cons(ws, "quote takes a single argument", cons);
    return NULL;
  }
  return NaClSexpDupNode(cons->cdr->car);
}

struct NaClSexpNode *CarImpl(struct NaClSexpCons *cons,
                             struct WorkState *ws) {
  struct NaClSexpNode *p;
  struct NaClSexpCons *arg;
  struct NaClSexpNode *result = NULL;
  if (NaClSexpListLength(cons) != 2) {
    error_cons(ws, "car takes a single argument", cons);
    return NULL;
  }
  p = Eval(cons->cdr->car, ws);
  if (NULL == p || !NaClSexpConsp(p)) {
    error_cons(ws, "car: argument must evaluate to a list", cons);
  } else {
    arg = NaClSexpNodeToCons(p);
    if (NULL != arg) {
      result = NaClSexpDupNode(arg->car);
    }
  }
  return result;
}

struct NaClSexpNode *CdrImpl(struct NaClSexpCons *cons,
                             struct WorkState *ws) {
  struct NaClSexpNode *p;
  struct NaClSexpCons *arg;
  struct NaClSexpNode *result = NULL;
  if (NaClSexpListLength(cons) != 2) {
    error_cons(ws, "cdr takes a single argument", cons);
    return NULL;
  }
  p = Eval(cons->cdr->car, ws);
  if (NULL == p || !NaClSexpConsp(p)) {
    error_cons(ws, "cdr: argument must evaluate to a list", cons);
  } else {
    arg = NaClSexpNodeToCons(p);
    if (NULL != arg) {
      result = NaClSexpNodeWrapCons(NaClSexpDupCons(arg->cdr));
    }
  }
  return result;
}

struct NaClSexpNode *ConsImpl(struct NaClSexpCons *cons,
                              struct WorkState *ws) {
  struct NaClSexpNode *first;
  struct NaClSexpNode *second;
  struct NaClSexpNode *result = NULL;

  if (NaClSexpListLength(cons) != 3) {
    error_cons(ws, "cons take two arguments", cons);
    return NULL;
  }
  first = Eval(cons->cdr->car, ws);
  second = Eval(cons->cdr->cdr->car, ws);
  if (!NaClSexpConsp(second)) {
    error_cons(ws, "cons: second argument must evaluate to a list", cons);
  } else {
    result = NaClSexpNodeWrapCons(
        NaClSexpConsCons(NaClSexpDupNode(first),
                         NaClSexpDupCons(NaClSexpNodeToCons(second))));
  }
  NaClSexpFreeNode(first);
  NaClSexpFreeNode(second);
  return result;
}

struct NaClSexpNode *ListImpl(struct NaClSexpCons *cons,
                               struct WorkState *ws) {
  struct NaClSexpCons *result = NULL;
  struct NaClSexpCons **addpos = &result;
  struct NaClSexpCons *p;

  for (p = cons->cdr; NULL != p; p = p->cdr) {
    *addpos = NaClSexpConsWrapNode(Eval(p->car, ws));
    addpos = &((*addpos)->cdr);
  }
  return NaClSexpNodeWrapCons(result);
}

struct NaClSexpNode *NthImpl(struct NaClSexpCons *cons,
                             struct WorkState *ws) {
  struct NaClSexpCons *result = NULL;
  struct NaClSexpCons *p;
  struct NaClSexpNode *arg_selector;
  struct NaClSexpNode *arg_list;
  size_t ix = 0;

  if (NaClSexpListLength(cons) != 3) {
    crash_cons(ws, "nth takes exactly two arguments", cons);
  }
  arg_selector = Eval(cons->cdr->car, ws);
  arg_list = Eval(cons->cdr->cdr->car, ws);
  if (!NaClSexpIntp(arg_selector)) {
    error_cons(ws, "nth: first arg does not evaluate to an integer", cons);
  } else if (!NaClSexpConsp(arg_list)) {
    error_cons(ws, "nth: second arg does not evaluate to a list", cons);
  } else {
    ix = NaClSexpNodeToInt(arg_selector);
    for (p = NaClSexpNodeToCons(arg_list);
         NULL != p && ix > 0;
         p = p->cdr, --ix) {
      continue;
    }
    if (ix == 0) {
      result = p;
    }
  }
  return NaClSexpDupNode(NULL == result ? NULL : result->car);
}

struct NaClSexpNode *AppendImpl(struct NaClSexpCons *cons,
                                struct WorkState *ws) {
  struct NaClSexpNode *first = NULL;
  struct NaClSexpNode *second = NULL;
  struct NaClSexpNode *result = NULL;

  if (NaClSexpListLength(cons) != 3) {
    crash_cons(ws, "append take exactly two arguments", cons);
  }
  first = Eval(cons->cdr->car, ws);
  second = Eval(cons->cdr->cdr->car, ws);
  if (!NaClSexpConsp(first)) {
    error_cons(ws, "append: first arg does not evaluate to a list", cons);
  } else if (!NaClSexpConsp(second)) {
    error_cons(ws, "append: second arg does not evaluate to a list", cons);
  } else {
    result = NaClSexpNodeWrapCons(NaClSexpAppend(
                                      NaClSexpNodeToCons(first),
                                      NaClSexpNodeToCons(second)));
  }
  NaClSexpFreeNode(first);
  NaClSexpFreeNode(second);
  return result;
}

/*
 * The arithmetic functions are just for fun.  And to test that the
 * basic evaluation framework makes sense.
 */
struct NaClSexpNode *MultImpl(struct NaClSexpCons *cons,
                              struct WorkState *ws) {
  int first = 1;
  int value = 0;
  struct NaClSexpNode *v = NULL;
  struct NaClSexpCons *p;

  for (p = cons->cdr; NULL != p; p = p->cdr) {
    v = Eval(p->car, ws);
    if (!NaClSexpIntp(v)) {
      error_node(ws, "argument not integral", v);
      return NULL;
    }
    if (first) {
      value = NaClSexpNodeToInt(v);
      first = 0;
    } else {
      value *= NaClSexpNodeToInt(v);
    }
    NaClSexpFreeNode(v);
  }
  return NaClSexpNodeWrapInt(value);
}

struct NaClSexpNode *DivImpl(struct NaClSexpCons *cons,
                              struct WorkState *ws) {
  int first = 1;
  int value = 0;
  struct NaClSexpNode *v = NULL;
  struct NaClSexpCons *p;

  for (p = cons->cdr; NULL != p; p = p->cdr) {
    v = Eval(p->car, ws);
    if (!NaClSexpIntp(v)) {
      error_node(ws, "argument not integral", v);
      return NULL;
    }
    if (first) {
      value = NaClSexpNodeToInt(v);
      first = 0;
    } else {
      value /= NaClSexpNodeToInt(v);
    }
    NaClSexpFreeNode(v);
  }
  return NaClSexpNodeWrapInt(value);
}

struct NaClSexpNode *AddImpl(struct NaClSexpCons *cons,
                              struct WorkState *ws) {
  int first = 1;
  int value = 0;
  struct NaClSexpNode *v = NULL;
  struct NaClSexpCons *p;

  for (p = cons->cdr; NULL != p; p = p->cdr) {
    v = Eval(p->car, ws);
    if (!NaClSexpIntp(v)) {
      error_node(ws, "argument not integral", v);
      return NULL;
    }
    if (first) {
      value = NaClSexpNodeToInt(v);
      first = 0;
    } else {
      value += NaClSexpNodeToInt(v);
    }
    NaClSexpFreeNode(v);
  }
  return NaClSexpNodeWrapInt(value);
}

struct NaClSexpNode *SubImpl(struct NaClSexpCons *cons,
                              struct WorkState *ws) {
  int first = 1;
  int value = 0;
  struct NaClSexpNode *v = NULL;
  struct NaClSexpCons *p;

  for (p = cons->cdr; NULL != p; p = p->cdr) {
    v = Eval(p->car, ws);
    if (!NaClSexpIntp(v)) {
      error_node(ws, "argument not integral", v);
      return NULL;
    }
    if (first) {
      value = NaClSexpNodeToInt(v);
      first = 0;
    } else {
      value -= NaClSexpNodeToInt(v);
    }
    NaClSexpFreeNode(v);
  }
  return NaClSexpNodeWrapInt(value);
}

static struct NaClSexpNode *MatcherResult(int64_t matcher_position,
                                          uint64_t matched_bitset) {
  DPRINTF(("MatcherResult(%"NACL_PRId64", 0x%"NACL_PRIx64")\n",
           matcher_position, matched_bitset));
  return NaClSexpNodeWrapCons(
      NaClSexpConsCons(NaClSexpNodeWrapInt(matcher_position),
                       NaClSexpConsCons(NaClSexpNodeWrapInt(matched_bitset),
                                        NULL)));
}

static int MatchResultExtract(int64_t *matcher_pos,
                              uint64_t *matched_bitset,
                              struct NaClSexpNode *result) {
  struct NaClSexpCons *result_cons;
  if (NULL == result) {
    return 0;
  }
  CHECK(NaClSexpConsp(result));
  result_cons = NaClSexpNodeToCons(result);
  CHECK(2 == NaClSexpListLength(result_cons));
  CHECK(NaClSexpIntp(result_cons->car));
  CHECK(NaClSexpIntp(result_cons->cdr->car));
  if (NULL != matcher_pos) {
    *matcher_pos = NaClSexpNodeToInt(result_cons->car);
  }
  if (NULL != matched_bitset) {
    *matched_bitset = NaClSexpNodeToInt(result_cons->cdr->car);
  }
  return 1;
}

struct NaClSexpNode *EpsilonMatcherImpl(struct NaClSexpCons *cons,
                                        struct WorkState *ws) {
  if (NaClSexpListLength(cons) != 1) {
    crash_cons(ws, "epsilon built-in should have no arguments", cons);
  }
  DPRINTF(("Epsilon\n"));
  /*
   * Check that the event queue is empty.
   */
  if (EventQueueLengthMu(ws) == 0) {
    DPRINTF(("epsilon: success -- empty event queue\n"));
    return MatcherResult(-1, 0);
  }
  DPRINTF(("epsilon: fail -- non-zero length event queue\n"));
  return NULL;
}

struct NaClSexpNode *PrognImpl(struct NaClSexpCons *cons,
                               struct WorkState *ws) {
  struct NaClSexpNode *node;
  struct NaClSexpNode *val = NULL;

  while (NULL != (cons = cons->cdr)) {
    node = cons->car;
    if (NaClSexpConsp(node)) {
      NaClSexpFreeNode(val);
      val = Eval(node, ws);
    } else {
      crash_node(ws, "Not a list", node);
    }
  }
  return val;
}

enum WorkType {
  kTakeLock = 0,
  kDropLock = 1,
};

struct WorkItem {
  struct WorkState *ws;
  enum WorkType type;
  int desc;
};

struct WorkItem *WorkItemFactory(struct WorkState *ws,
                                 enum WorkType type,
                                 int desc) {
  struct WorkItem *wi = malloc(sizeof *wi);
  CHECK(NULL != wi);
  wi->ws = ws;
  wi->type = type;
  wi->desc = desc;
  return wi;
}

static void WorkOnWorkItem(void *functor_state) {
  struct WorkItem *wi = (struct WorkItem *) functor_state;

  DPRINTF(("WorkOnWorkItem: entered\n"));
  switch (wi->type) {
    case kTakeLock:
      NaClFileLockManagerLock(&wi->ws->flm, wi->desc);
      break;
    case kDropLock:
      NaClFileLockManagerUnlock(&wi->ws->flm, wi->desc);
      break;
  }
  DPRINTF(("WorkOnWorkItem: done\n"));
  free(wi);
}

struct NaClSexpNode *LockUnlock(struct NaClSexpCons *cons,
                                struct WorkState *ws,
                                enum WorkType op) {
  struct NaClSexpNode *p;
  int actor_thread;
  int file_desc;

  CheckState(cons, ws);
  if (NaClSexpListLength(cons) != 3) {
    error_cons(ws,
               "lock/unlock takes 2 arguments: thread-id and file-id", cons);
    return NULL;
  }
  p = Eval(cons->cdr->car, ws);
  if (!NaClSexpIntp(p)) {
    error_cons(ws,
               "lock/unlock thread-id argument evaluate to an integer", cons);
    return NULL;
  }
  actor_thread = NaClSexpNodeToInt(p);
  NaClSexpFreeNode(p);
  p = Eval(cons->cdr->cdr->car, ws);
  if (!NaClSexpIntp(p)) {
    error_cons(ws,
               "lock/unlock file-id argument evaluate to an integer", cons);
    return NULL;
  }
  file_desc = NaClSexpNodeToInt(p);
  NaClSexpFreeNode(p);
  PutWork(ws, actor_thread,
          WorkOnWorkItem, WorkItemFactory(ws, op, file_desc));
  return NULL;
}

struct NaClSexpNode *LockImpl(struct NaClSexpCons *cons, struct WorkState *ws) {
  /*
   * (lock t f) -- tell thread t to take lock f.
   */
  return LockUnlock(cons, ws, kTakeLock);
}

struct NaClSexpNode *UnlockImpl(struct NaClSexpCons *cons,
                                struct WorkState *ws) {
  /*
   * (unlock t f) -- tell thread t to drop lock f.
   */
  return LockUnlock(cons, ws, kDropLock);
}

/*
 * Called by matchers, so ws->mu is held already.
 */
struct NaClSexpNode *EventQueueEventMatcher(struct NaClSexpCons *cons,
                                            struct WorkState *ws,
                                            enum EventType expected_event) {
  struct NaClSexpNode *n;
  int expected_thread_id;
  int expected_file_id;
  size_t list_pos = 0;
  struct NaClSexpNode *p = NULL;
  struct Event *event_entry;

  if (NaClSexpListLength(cons) != 3) {
    error_cons(ws,
               "locked/unlocked takes exactly two arguments, thread_id and"
               " lock_id", cons);
    return NULL;
  }
  n = Eval(cons->cdr->car, ws);
  if (!NaClSexpIntp(n)) {
    error_cons(ws,
               "locked/unlocked thread_id argument must eval to an integer",
               cons);
    NaClSexpFreeNode(n);
    return NULL;
  }
  expected_thread_id = NaClSexpNodeToInt(n);
  NaClSexpFreeNode(n);
  n = Eval(cons->cdr->cdr->car, ws);
  if (!NaClSexpIntp(n)) {
    error_cons(ws,
               "locked/unlocked file_id argument must eval to an integer",
               cons);
    NaClSexpFreeNode(n);
    return NULL;
  }
  expected_file_id = NaClSexpNodeToInt(n);
  NaClSexpFreeNode(n);

  DPRINTF(("locked/unlocked matcher: %s, thread %d, file %d\n",
           (expected_event == kLocked) ? "locked" : "unlocked",
           expected_thread_id, expected_file_id));

  for (event_entry = ws->q;
       NULL != event_entry;
       ++list_pos, event_entry = event_entry->next) {
    if (event_entry->type == expected_event &&
        event_entry->thread_id == expected_thread_id &&
        event_entry->file_id == expected_file_id) {
      if (list_pos > 8 * sizeof(int64_t)) {
        crash_cons(ws, "event queue too deep", cons);
      }
      p = MatcherResult(-1, 1 << list_pos);
      break;
    }
  }
  return p;
}

struct NaClSexpNode *LockedMatcherImpl(struct NaClSexpCons *cons,
                                       struct WorkState *ws) {
  /*
   * (locked t f) -- look for event (kLocked t f) in event queue.
   */
  return EventQueueEventMatcher(cons, ws, kLocked);
}

struct NaClSexpNode *UnlockedMatcherImpl(struct NaClSexpCons *cons,
                                         struct WorkState *ws) {
  /*
   * (unlocked t f) -- look for event (kUnlocked t f) in event queue.
   */
  return EventQueueEventMatcher(cons, ws, kUnlocked);
}

struct NaClSexpNode *AllMatcherImpl(struct NaClSexpCons *cons,
                                    struct WorkState *ws);

struct NaClSexpNode *AnyMatcherImpl(struct NaClSexpCons *cons,
                                    struct WorkState *ws);

struct NaClSexpNode *Eval(struct NaClSexpNode *n,
                          struct WorkState *ws);

struct NaClSexpNode *EvalImpl(struct NaClSexpCons *cons,
                              struct WorkState *ws) {
  struct NaClSexpNode *n;
  struct NaClSexpNode *eval_n;
  /*
   * Check that there is a single argument, the invoke Eval on it.
   */
  if (NaClSexpListLength(cons) != 2) {
    error_cons(ws, "eval takes exactly one argument", cons);
    return NULL;
  }
  n = Eval(cons->cdr->car, ws);
  eval_n = Eval(n, ws);
  NaClSexpFreeNode(n);
  return eval_n;
}

struct NaClSexpNode *MatchMatcherImpl(struct NaClSexpCons *cons,
                                      struct WorkState *ws);

struct SymbolTable {
  char const *name;
  int is_matcher;

  /*
   * The |fn| member may be a built-in function, or a matcher as
   * specified by |is_matcher|.  If !is_matcher, then it returns an
   * s-expression that is the value of the function.  If it is a
   * matcher, then at the lisp level the "apparent" return value is an
   * integer.  Internally, successful matchers return the following:
   *
   *   ( sub-matcher-index event-pos-bitset )
   *
   * For non-composite matchers, sub-matcher-index is NULL.  The list
   * of event-pos-bitset is an integer with a non-zero bit for each
   * events that were matched, with the bit position corresponding to
   * their positions in the event queue.  NB: it is a test
   * specification error if any sub-matchers used in the (any ...)
   * special form matches a subset of the events matched by another
   * sub-matcher, or sub-matchers used in the (all ...)  special form
   * overlap.
   *
   * A matcher that failed to match returns NULL.
   */
  struct NaClSexpNode *(*fn)(struct NaClSexpCons *cons, struct WorkState *ws);
};

struct SymbolTable g_symtab[] = {
  {
    "set-threads", 0, SetThreadsImpl
  }, {
    "set-files", 0, SetFilesImpl
  }, {
    "quote", 0, QuoteImpl
  }, {
    "car", 0, CarImpl
  }, {
    "cdr", 0, CdrImpl
  }, {
    "cons", 0, ConsImpl
  }, {
    "list", 0, ListImpl
  }, {
    "nth", 0, NthImpl
  }, {
    "append", 0, AppendImpl
  }, {
    "eval", 0, EvalImpl
  }, {
    "*", 0, MultImpl
  }, {
    "/", 0, DivImpl
  }, {
    "+", 0, AddImpl
  }, {
    "-", 0, SubImpl
  }, {
    "progn", 0, PrognImpl
  }, {
    "lock", 0, LockImpl
  }, {
    "unlock", 0, UnlockImpl
  }, {
    "match", 1, MatchMatcherImpl
  }, {
    "epsilon", 1, EpsilonMatcherImpl
  }, {
    "locked", 1, LockedMatcherImpl
  }, {
    "unlocked", 1, UnlockedMatcherImpl
  }, {
    "all", 1, AllMatcherImpl
  }, {
    "any", 1, AnyMatcherImpl
  }
};

static int CheckMatchers(struct WorkState *ws,
                         struct NaClSexpCons *matcher_list,
                         struct SymbolTable **matcher_entries) {
  size_t mx;
  size_t ix;
  struct NaClSexpCons *matchers;
  struct NaClSexpNode *cur;
  struct NaClSexpCons *submatcher;
  char const *submatcher_name;

  for (mx = 0, matchers = matcher_list;
       NULL != matchers;
       ++mx, matchers = matchers->cdr) {
    cur = matchers->car;
    if (!NaClSexpConsp(cur)) {
      error_node(ws, "all: submatcher not a list", cur);
      return 0;
    }
    submatcher = NaClSexpNodeToCons(cur);
    if (NULL == submatcher || !NaClSexpTokenp(submatcher->car)) {
      error_node(ws, "all: submatcher not a matcher", cur);
      return 0;
    }
    submatcher_name = NaClSexpNodeToToken(submatcher->car);
    for (ix = 0; ix < sizeof g_symtab/sizeof g_symtab[0]; ++ix) {
      if (!strcmp(g_symtab[ix].name, submatcher_name)) {
        /* found! */
        if (!g_symtab[ix].is_matcher) {
          error_node(ws, "all: submatcher not a matcher", cur);
          return 0;
        }
        /* save pointer for later use */
        matcher_entries[mx] = &g_symtab[ix];
        break;
      }
    }
    if (ix == sizeof g_symtab/sizeof g_symtab[0]) {
      error_node(ws, "all: submatcher not found", cur);
      return 0;
    }
  }
  return 1;
}

struct NaClSexpNode *AllMatcherImpl(struct NaClSexpCons *cons,
                                    struct WorkState *ws) {
  int64_t match_index = -1;
  uint64_t match_pos = 0;
  size_t num_matchers = NaClSexpListLength(cons) - 1;
  size_t mx;
  struct NaClSexpCons *matchers;
  struct NaClSexpNode *cur;
  struct SymbolTable **matcher_entries = NULL;
  struct NaClSexpNode *match_result;
  int64_t submatch_index;
  uint64_t submatch_pos;

  if (gVerbosity > 2) {
    printf("AllMatcherImpl: %"NACL_PRIdS" submatchers\n", num_matchers);
  }
  if (0 == num_matchers) {
    error_cons(ws, "all must have at least one sub-matcher", cons);
    return NULL;
  }
  matcher_entries = (struct SymbolTable **)
      malloc(num_matchers * sizeof *matcher_entries);
  if (!CheckMatchers(ws, cons->cdr, matcher_entries)) {
    error_cons(ws, "all: submatcher error", cons);
    free(matcher_entries);
    return NULL;
  }
  /*
   * Invoke all submatchers.  If any fail, we fail.  Accumulate event
   * queue indices.
   */
  for (mx = 0, matchers = cons->cdr;
       NULL != matchers;
       ++mx, matchers = matchers->cdr) {
    cur = matchers->car;
    match_result = (*matcher_entries[mx]->fn)(NaClSexpNodeToCons(cur), ws);

    if (gVerbosity > 2) {
      printf("submatcher "); NaClSexpPrintNode(stdout,cur); printf(" --> ");
      NaClSexpPrintNode(stdout, match_result);
      printf("\n");
    }

    if (!MatchResultExtract(&submatch_index, &submatch_pos, match_result)) {
      DPRINTF(("submatcher failed\n"));
      NaClSexpFreeNode(match_result);
      free(matcher_entries);
      return NULL;
    }
    NaClSexpFreeNode(match_result);

    if ((match_pos & submatch_pos) != 0) {
      error_cons(ws, "all: overlapping matchers", cons);
      free(matcher_entries);
      return NULL;
    }
    /*
     * Allow, for example, the (all (unlocked 0 0) (any (locked 1 0)
     * (locked 2 0))) to propagate the submatch index from the (any
     * ...) form up.  This does not generalize to more than one (any
     * ...) sub-sexpressions in the (all ...) form.
     */
    if (-1 == match_index && -1 != submatch_index) {
      match_index = submatch_index;
    }
    match_pos |= submatch_pos;
  }
  free(matcher_entries);
  return MatcherResult(match_index, match_pos);
}

struct NaClSexpNode *AnyMatcherImpl(struct NaClSexpCons *cons,
                                    struct WorkState *ws) {
  uint64_t submatch_pos = 0;
  size_t num_matchers = NaClSexpListLength(cons) - 1;
  size_t mx;
  struct NaClSexpCons *matchers;
  struct NaClSexpNode *cur;
  struct SymbolTable **matcher_entries = NULL;
  struct NaClSexpNode *match_result = NULL;

  if (gVerbosity > 2) {
    printf("AnyMatcherImpl: %"NACL_PRIdS" submatchers\n", num_matchers);
  }
  if (0 == num_matchers) {
    error_cons(ws, "any must have at least one sub-matcher", cons);
    return NULL;
  }
  matcher_entries = (struct SymbolTable **)
      malloc(num_matchers * sizeof *matcher_entries);
  if (!CheckMatchers(ws, cons->cdr, matcher_entries)) {
    error_cons(ws, "any: submatcher error", cons);
    free(matcher_entries);
    return NULL;
  }
  /*
   * Invoke all submatchers.  If any succeed, we succeed.
   */
  for (mx = 0, matchers = cons->cdr;
       NULL != matchers;
       ++mx, matchers = matchers->cdr) {
    cur = matchers->car;
    match_result = (*matcher_entries[mx]->fn)(NaClSexpNodeToCons(cur), ws);

    if (gVerbosity > 2) {
      printf("submatcher "); NaClSexpPrintNode(stdout,cur); printf(" --> ");
      NaClSexpPrintNode(stdout, match_result);
      printf("\n");
    }

    if (MatchResultExtract((int64_t *) NULL, &submatch_pos, match_result)) {
      DPRINTF(("submatcher succeeded\n"));
      NaClSexpFreeNode(match_result);
      match_result = MatcherResult(mx, submatch_pos);
      break;
    }
    NaClSexpFreeNode(match_result);
  }
  free(matcher_entries);
  return match_result;
}

/*
 * Single arg must be a matcher.  Evaluate it.  If successful, check
 * that all events in the event queue were matched; if so, discard all
 * events in event queue, and return the match index.  If not
 * successful or not all events were matched, leave the event queue
 * alone and return -1.  Typically returned value is used by (nth ...)
 * form.
 */
struct NaClSexpNode *MatchMatcherImpl(struct NaClSexpCons *cons,
                                      struct WorkState *ws) {

  struct SymbolTable *matcher[1];
  struct NaClSexpNode *result;
  int64_t which_match;
  uint64_t match_set;

  DPRINTF(("MatchMatcherImpl\n"));
  if (NaClSexpListLength(cons) != 2) {
    error_cons(ws, "match takes a single matcher as argument", cons);
    return NULL;
  }
  if (!CheckMatchers(ws, cons->cdr, matcher)) {
    error_cons(ws, "match argument should be a matcher", cons);
    return NULL;
  }
  if (!NaClSexpConsp(cons->cdr->car)) {
    error_cons(ws, "match argument not a list", cons);
    return NULL;
  }
  result = (*matcher[0]->fn)(NaClSexpNodeToCons(cons->cdr->car), ws);
  if (!MatchResultExtract(&which_match, &match_set, result)) {
    DPRINTF(("did not match\n"));
    NaClSexpFreeNode(result);
    result = NULL;
  } else if (match_set != ((((uint64_t) 1) << EventQueueLengthMu(ws)) - 1)) {
    /* did not match all entries */
    DPRINTF(("match set incomplete\n"));
    NaClSexpFreeNode(result);
    result = NULL;
  } else {
    DPRINTF(("matched all events\n"));
    EventQueueDiscardMu(ws);
    NaClSexpFreeNode(result);
    result = NaClSexpNodeWrapInt(which_match);
  }
  return result;
}

struct NaClSexpNode *Eval(struct NaClSexpNode *n, struct WorkState *ws) {
  struct NaClSexpCons *cons;
  size_t ix;
  struct NaClSexpNode *car;
  char const *token;
  struct NaClSexpNode *val = NULL;
  struct timespec timeout_time;
  int wait_for_more;
  int last_match;

  if (!NaClSexpConsp(n)) {
    /* for now; symbol table lookup when there are symbols with values */
    return NaClSexpDupNode(n);
  }
  cons = NaClSexpNodeToCons(n);
  if (NULL == cons) {
    return NaClSexpDupNode(n);
  }

  car = cons->car;
    if (!NaClSexpTokenp(car)) {
    crash_cons(ws,
               "nacl_file_lock_test: car of list should be a built-in function",
               cons);
  }
  token = NaClSexpNodeToToken(car);
  DPRINTF(("function %s\n", token));
  for (ix = 0; ix < sizeof g_symtab / sizeof g_symtab[0]; ++ix) {
    if (!strcmp(token, g_symtab[ix].name)) {
      if (!g_symtab[ix].is_matcher) {
        val = (*g_symtab[ix].fn)(cons, ws);
      } else {
        /*
         * A "matcher" special form is a bit weird/tricky.  These are
         * not blocking functions, since matchers can be used in
         * conjunction with other matchers in (all ...) or (any ...)
         * forms.  What we do is the following: we wait for events,
         * and as events arrive in the event queue, we run the
         * matcher, which must immediately return whether a match
         * against the contents of the event queue occurred.  If there
         * is no match, we wait for another event, until the timeout
         * occurs; if timeout occurs and the matcher did not match,
         * then it is an error and we abort the test program.  For the
         * (all ...) form, the AllMatcherImpl (essentially AND) will
         * check that all arguments are themselves matchers and run
         * them to see if all succeeds, and AllMatcherImpl only
         * succeeds if all succeeded.  The AnyMatcherImpl (OR) will
         * succeed if any of the argument matchers succeed.  Matchers
         * return the position in the event queue where the match
         * occurred, so the composite matchers can remove the events
         * appropriately.  Matching and removal is done with the event
         * list locked, so worker threads that wake up etc cannot add
         * new events.  Position is relative to the head, so even if
         * this were not the case, we would not get confused about
         * which event was matched.
         *
         * Later we may consider matchers that introduce a variable to
         * be bound to an event parameter, to be used later by
         * subsequent matchers.  This can be used to cut down on the
         * combinatoric explosion that occurs with possible futures
         * when, for example, several threads might wake up when a
         * lock becomes available.  We need to think about scoping in
         * that case.
         */
        DPRINTF(("Matcher found\n"));
        pthread_mutex_lock(&ws->mu);
        last_match = 0;
        ComputeAbsTimeout(&timeout_time, gEpsilonDelayNanos);

        wait_for_more = 0;
        while (!last_match) {
          DPRINTF(("Checking EventQueueLengthMu -> %d\n",
                   (int) EventQueueLengthMu(ws)));
          if (EventQueueLengthMu(ws) == 0 || wait_for_more) {
            DPRINTF(("Waiting for event\n"));
            if (!WaitForEventMu(ws, &timeout_time)) {
              DPRINTF(("Event timed out\n"));
              last_match = 1;
            }
          }
          /*
           * Run matcher on event queue; matchers are invoked while
           * holding the lock.
           */
          val = (*g_symtab[ix].fn)(cons, ws);
          if (gVerbosity > 1) {
            printf("matcher returned: ");
            NaClSexpPrintNode(stdout,val);
            putchar('\n');
          }
          if (NULL != val) {
            /* successful match! */
            break;
          }

          if (last_match) {
            error_cons(ws, "match failed; timed out", cons);
            val = NULL;
            break;
          }
          DPRINTF(("match failed, waiting for more events\n"));
          wait_for_more = 1;
        }

        pthread_mutex_unlock(&ws->mu);
      }
      return val;
    }
  }
  fprintf(stderr, "No such function: %s\n", token);
  return NULL;
}

void ReadEvalPrintLoop(struct NaClSexpIo *input,
                       int interactive,
                       int verbosity,
                       size_t epsilon_delay_nanos,
                       struct NaClFileLockTestInterface *test_if) {
  struct WorkState ws;
  struct NaClSexpNode *n;
  struct NaClSexpNode *val;
  int err;

  gInteractive = interactive;
  gVerbosity = verbosity;
  if (epsilon_delay_nanos < MIN_EPSILON_DELAY_NANOS) {
    epsilon_delay_nanos = MIN_EPSILON_DELAY_NANOS;
  }
  gEpsilonDelayNanos = epsilon_delay_nanos;

  err = pthread_key_create(&gNaClTestTsdKey, (void (*)(void *)) NULL);
  CHECK(0 == err);

  WorkStateCtor(&ws, test_if);

  while (NULL != (n = NaClSexpReadSexp(input))) {
    if (gVerbosity > 1 && n->type == kNaClSexpCons) {
      printf("Program list length: %"NACL_PRIdS"\n",
             NaClSexpListLength(n->u.cval));
    }

    if (gVerbosity) {
      NaClSexpPrintNode(stdout, n);
      printf(" => ");
    }
    val = Eval(n, &ws);
    NaClSexpFreeNode(n);
    NaClSexpPrintNode(stdout, val);
    printf("\n");
    NaClSexpFreeNode(val);
  }
  AnnounceEndOfWork(&ws);
  ReapThreads(&ws);
  WorkStateDtor(&ws);
}
