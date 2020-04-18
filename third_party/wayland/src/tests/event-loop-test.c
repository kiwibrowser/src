/*
 * Copyright © 2012 Intel Corporation
 * Copyright © 2012 Jason Ekstrand
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "wayland-private.h"
#include "wayland-server.h"
#include "test-runner.h"

static int
fd_dispatch(int fd, uint32_t mask, void *data)
{
	int *p = data;

	assert(mask == 0);
	++(*p);

	return 0;
}

TEST(event_loop_post_dispatch_check)
{
	struct wl_event_loop *loop = wl_event_loop_create();
	struct wl_event_source *source;
	int dispatch_ran = 0;
	int p[2];

	assert(loop);
	assert(pipe(p) == 0);

	source = wl_event_loop_add_fd(loop, p[0], WL_EVENT_READABLE,
				      fd_dispatch, &dispatch_ran);
	assert(source);
	wl_event_source_check(source);

	wl_event_loop_dispatch(loop, 0);
	assert(dispatch_ran == 1);

	assert(close(p[0]) == 0);
	assert(close(p[1]) == 0);
	wl_event_source_remove(source);
	wl_event_loop_destroy(loop);
}

struct free_source_context {
	struct wl_event_source *source1, *source2;
	int p1[2], p2[2];
	int count;
};

static int
free_source_callback(int fd, uint32_t mask, void *data)
{
	struct free_source_context *context = data;

	context->count++;

	/* Remove other source */
	if (fd == context->p1[0]) {
		wl_event_source_remove(context->source2);
		context->source2 = NULL;
	} else if (fd == context->p2[0]) {
		wl_event_source_remove(context->source1);
		context->source1 = NULL;
	} else {
		assert(0);
	}

	return 1;
}

TEST(event_loop_free_source_with_data)
{
	struct wl_event_loop *loop = wl_event_loop_create();
	struct free_source_context context;
	int data;

	/* This test is a little tricky to get right, since we don't
	 * have any guarantee from the event loop (ie epoll) on the
	 * order of which it reports events.  We want to have one
	 * source free the other, but we don't know which one is going
	 * to run first.  So we add two fd sources with a callback
	 * that frees the other source and check that only one of them
	 * run (and that we don't crash, of course).
	 */

	assert(loop);

	context.count = 0;
	assert(pipe(context.p1) == 0);
	assert(pipe(context.p2) == 0);
	context.source1 =
		wl_event_loop_add_fd(loop, context.p1[0], WL_EVENT_READABLE,
				     free_source_callback, &context);
	assert(context.source1);
	context.source2 =
		wl_event_loop_add_fd(loop, context.p2[0], WL_EVENT_READABLE,
				     free_source_callback, &context);
	assert(context.source2);

	data = 5;
	assert(write(context.p1[1], &data, sizeof data) == sizeof data);
	assert(write(context.p2[1], &data, sizeof data) == sizeof data);

	wl_event_loop_dispatch(loop, 0);

	assert(context.count == 1);

	if (context.source1)
		wl_event_source_remove(context.source1);
	if (context.source2)
		wl_event_source_remove(context.source2);
	wl_event_loop_destroy(loop);

	assert(close(context.p1[0]) == 0);
	assert(close(context.p1[1]) == 0);
	assert(close(context.p2[0]) == 0);
	assert(close(context.p2[1]) == 0);
}

static int
signal_callback(int signal_number, void *data)
{
	int *got_it = data;

	assert(signal_number == SIGUSR1);
	++(*got_it);

	return 1;
}

TEST(event_loop_signal)
{
	struct wl_event_loop *loop = wl_event_loop_create();
	struct wl_event_source *source;
	int got_it = 0;

	source = wl_event_loop_add_signal(loop, SIGUSR1,
					  signal_callback, &got_it);
	assert(source);

	wl_event_loop_dispatch(loop, 0);
	assert(!got_it);
	kill(getpid(), SIGUSR1);
	wl_event_loop_dispatch(loop, 0);
	assert(got_it == 1);

	wl_event_source_remove(source);
	wl_event_loop_destroy(loop);
}

TEST(event_loop_multiple_same_signals)
{
	struct wl_event_loop *loop = wl_event_loop_create();
	struct wl_event_source *s1, *s2;
	int calls_no = 0;
	int i;

	s1 = wl_event_loop_add_signal(loop, SIGUSR1,
				      signal_callback, &calls_no);
	assert(s1);

	s2 = wl_event_loop_add_signal(loop, SIGUSR1,
				      signal_callback, &calls_no);
	assert(s2);

	assert(wl_event_loop_dispatch(loop, 0) == 0);
	assert(!calls_no);

	/* Try it more times */
	for (i = 0; i < 5; ++i) {
		calls_no = 0;
		kill(getpid(), SIGUSR1);
		assert(wl_event_loop_dispatch(loop, 0) == 0);
		assert(calls_no == 2);
	}

	wl_event_source_remove(s1);

	/* Try it again  with one source */
	calls_no = 0;
	kill(getpid(), SIGUSR1);
	assert(wl_event_loop_dispatch(loop, 0) == 0);
	assert(calls_no == 1);

	wl_event_source_remove(s2);

	wl_event_loop_destroy(loop);
}

static int
timer_callback(void *data)
{
	int *got_it = data;

	++(*got_it);

	return 1;
}

TEST(event_loop_timer)
{
	struct wl_event_loop *loop = wl_event_loop_create();
	struct wl_event_source *source;
	int got_it = 0;

	source = wl_event_loop_add_timer(loop, timer_callback, &got_it);
	assert(source);
	wl_event_source_timer_update(source, 10);
	wl_event_loop_dispatch(loop, 0);
	assert(!got_it);
	wl_event_loop_dispatch(loop, 20);
	assert(got_it == 1);

	wl_event_source_remove(source);
	wl_event_loop_destroy(loop);
}

#define MSEC_TO_USEC(msec) ((msec) * 1000)

struct timer_update_context {
	struct wl_event_source *source1, *source2;
	int count;
};

static int
timer_update_callback_1(void *data)
{
	struct timer_update_context *context = data;

	context->count++;
	wl_event_source_timer_update(context->source2, 1000);
	return 1;
}

static int
timer_update_callback_2(void *data)
{
	struct timer_update_context *context = data;

	context->count++;
	wl_event_source_timer_update(context->source1, 1000);
	return 1;
}

TEST(event_loop_timer_updates)
{
	struct wl_event_loop *loop = wl_event_loop_create();
	struct timer_update_context context;
	struct timeval start_time, end_time, interval;

	/* Create two timers that should expire at the same time (after 10ms).
	 * The first timer to receive its expiry callback updates the other timer
	 * with a much larger timeout (1s). This highlights a bug where
	 * wl_event_source_timer_dispatch would block for this larger timeout
	 * when reading from the timer fd, before calling the second timer's
	 * callback.
	 */

	context.source1 = wl_event_loop_add_timer(loop, timer_update_callback_1,
						  &context);
	assert(context.source1);
	assert(wl_event_source_timer_update(context.source1, 10) == 0);

	context.source2 = wl_event_loop_add_timer(loop, timer_update_callback_2,
						  &context);
	assert(context.source2);
	assert(wl_event_source_timer_update(context.source2, 10) == 0);

	context.count = 0;

	/* Since calling the functions between source2's update and
	 * wl_event_loop_dispatch() takes some time, it may happen
	 * that only one timer expires until we call epoll_wait.
	 * This naturally means that only one source is dispatched
	 * and the test fails. To fix that, sleep 15 ms before
	 * calling wl_event_loop_dispatch(). That should be enough
	 * for the second timer to expire.
	 *
	 * https://bugs.freedesktop.org/show_bug.cgi?id=80594
	 */
	usleep(MSEC_TO_USEC(15));

	gettimeofday(&start_time, NULL);
	wl_event_loop_dispatch(loop, 20);
	gettimeofday(&end_time, NULL);

	assert(context.count == 2);

	/* Dispatching the events should not have taken much more than 20ms,
	 * since this is the timeout passed to wl_event_loop_dispatch. If it
	 * blocked, then it will have taken over 1s.
	 * Of course, it could take over 1s anyway on a very slow or heavily
	 * loaded system, so this test isn't 100% perfect.
	 */

	timersub(&end_time, &start_time, &interval);
	assert(interval.tv_sec < 1);

	wl_event_source_remove(context.source1);
	wl_event_source_remove(context.source2);
	wl_event_loop_destroy(loop);
}

struct event_loop_destroy_listener {
	struct wl_listener listener;
	int done;
};

static void
event_loop_destroy_notify(struct wl_listener *l, void *data)
{
	struct event_loop_destroy_listener *listener =
		container_of(l, struct event_loop_destroy_listener, listener);

	listener->done = 1;
}

TEST(event_loop_destroy)
{
	struct wl_event_loop *loop;
	struct wl_display * display;
	struct event_loop_destroy_listener a, b;

	loop = wl_event_loop_create();
	assert(loop);

	a.listener.notify = &event_loop_destroy_notify;
	a.done = 0;
	wl_event_loop_add_destroy_listener(loop, &a.listener);

	assert(wl_event_loop_get_destroy_listener(loop,
	       event_loop_destroy_notify) == &a.listener);

	b.listener.notify = &event_loop_destroy_notify;
	b.done = 0;
	wl_event_loop_add_destroy_listener(loop, &b.listener);

	wl_list_remove(&a.listener.link);
	wl_event_loop_destroy(loop);

	assert(!a.done);
	assert(b.done);

	/* Test to make sure it gets fired on display destruction */
	display = wl_display_create();
	assert(display);
	loop = wl_display_get_event_loop(display);
	assert(loop);

	a.done = 0;
	wl_event_loop_add_destroy_listener(loop, &a.listener);

	wl_display_destroy(display);

	assert(a.done);
}

