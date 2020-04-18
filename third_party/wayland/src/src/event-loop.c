/*
 * Copyright © 2008 Kristian Høgsberg
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

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "wayland-util.h"
#include "wayland-private.h"
#include "wayland-server-core.h"
#include "wayland-os.h"

struct wl_event_loop {
	int epoll_fd;
	struct wl_list check_list;
	struct wl_list idle_list;
	struct wl_list destroy_list;

	struct wl_signal destroy_signal;
};

struct wl_event_source_interface {
	int (*dispatch)(struct wl_event_source *source,
			struct epoll_event *ep);
};

struct wl_event_source {
	struct wl_event_source_interface *interface;
	struct wl_event_loop *loop;
	struct wl_list link;
	void *data;
	int fd;
};

struct wl_event_source_fd {
	struct wl_event_source base;
	wl_event_loop_fd_func_t func;
	int fd;
};

static int
wl_event_source_fd_dispatch(struct wl_event_source *source,
			    struct epoll_event *ep)
{
	struct wl_event_source_fd *fd_source = (struct wl_event_source_fd *) source;
	uint32_t mask;

	mask = 0;
	if (ep->events & EPOLLIN)
		mask |= WL_EVENT_READABLE;
	if (ep->events & EPOLLOUT)
		mask |= WL_EVENT_WRITABLE;
	if (ep->events & EPOLLHUP)
		mask |= WL_EVENT_HANGUP;
	if (ep->events & EPOLLERR)
		mask |= WL_EVENT_ERROR;

	return fd_source->func(fd_source->fd, mask, source->data);
}

struct wl_event_source_interface fd_source_interface = {
	wl_event_source_fd_dispatch,
};

static struct wl_event_source *
add_source(struct wl_event_loop *loop,
	   struct wl_event_source *source, uint32_t mask, void *data)
{
	struct epoll_event ep;

	if (source->fd < 0) {
		free(source);
		return NULL;
	}

	source->loop = loop;
	source->data = data;
	wl_list_init(&source->link);

	memset(&ep, 0, sizeof ep);
	if (mask & WL_EVENT_READABLE)
		ep.events |= EPOLLIN;
	if (mask & WL_EVENT_WRITABLE)
		ep.events |= EPOLLOUT;
	ep.data.ptr = source;

	if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, source->fd, &ep) < 0) {
		close(source->fd);
		free(source);
		return NULL;
	}

	return source;
}

WL_EXPORT struct wl_event_source *
wl_event_loop_add_fd(struct wl_event_loop *loop,
		     int fd, uint32_t mask,
		     wl_event_loop_fd_func_t func,
		     void *data)
{
	struct wl_event_source_fd *source;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &fd_source_interface;
	source->base.fd = wl_os_dupfd_cloexec(fd, 0);
	source->func = func;
	source->fd = fd;

	return add_source(loop, &source->base, mask, data);
}

WL_EXPORT int
wl_event_source_fd_update(struct wl_event_source *source, uint32_t mask)
{
	struct wl_event_loop *loop = source->loop;
	struct epoll_event ep;

	memset(&ep, 0, sizeof ep);
	if (mask & WL_EVENT_READABLE)
		ep.events |= EPOLLIN;
	if (mask & WL_EVENT_WRITABLE)
		ep.events |= EPOLLOUT;
	ep.data.ptr = source;

	return epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD, source->fd, &ep);
}

struct wl_event_source_timer {
	struct wl_event_source base;
	wl_event_loop_timer_func_t func;
};

static int
wl_event_source_timer_dispatch(struct wl_event_source *source,
			       struct epoll_event *ep)
{
	struct wl_event_source_timer *timer_source =
		(struct wl_event_source_timer *) source;
	uint64_t expires;
	int len;

	len = read(source->fd, &expires, sizeof expires);
	if (!(len == -1 && errno == EAGAIN) && len != sizeof expires)
		/* Is there anything we can do here?  Will this ever happen? */
		wl_log("timerfd read error: %m\n");

	return timer_source->func(timer_source->base.data);
}

struct wl_event_source_interface timer_source_interface = {
	wl_event_source_timer_dispatch,
};

WL_EXPORT struct wl_event_source *
wl_event_loop_add_timer(struct wl_event_loop *loop,
			wl_event_loop_timer_func_t func,
			void *data)
{
	struct wl_event_source_timer *source;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &timer_source_interface;
	source->base.fd = timerfd_create(CLOCK_MONOTONIC,
					 TFD_CLOEXEC | TFD_NONBLOCK);
	source->func = func;

	return add_source(loop, &source->base, WL_EVENT_READABLE, data);
}

WL_EXPORT int
wl_event_source_timer_update(struct wl_event_source *source, int ms_delay)
{
	struct itimerspec its;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = ms_delay / 1000;
	its.it_value.tv_nsec = (ms_delay % 1000) * 1000 * 1000;
	if (timerfd_settime(source->fd, 0, &its, NULL) < 0)
		return -1;

	return 0;
}

struct wl_event_source_signal {
	struct wl_event_source base;
	int signal_number;
	wl_event_loop_signal_func_t func;
};

static int
wl_event_source_signal_dispatch(struct wl_event_source *source,
				struct epoll_event *ep)
{
	struct wl_event_source_signal *signal_source =
		(struct wl_event_source_signal *) source;
	struct signalfd_siginfo signal_info;
	int len;

	len = read(source->fd, &signal_info, sizeof signal_info);
	if (!(len == -1 && errno == EAGAIN) && len != sizeof signal_info)
		/* Is there anything we can do here?  Will this ever happen? */
		wl_log("signalfd read error: %m\n");

	return signal_source->func(signal_source->signal_number,
				   signal_source->base.data);
}

struct wl_event_source_interface signal_source_interface = {
	wl_event_source_signal_dispatch,
};

WL_EXPORT struct wl_event_source *
wl_event_loop_add_signal(struct wl_event_loop *loop,
			 int signal_number,
			 wl_event_loop_signal_func_t func,
			 void *data)
{
	struct wl_event_source_signal *source;
	sigset_t mask;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &signal_source_interface;
	source->signal_number = signal_number;

	sigemptyset(&mask);
	sigaddset(&mask, signal_number);
	source->base.fd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	source->func = func;

	return add_source(loop, &source->base, WL_EVENT_READABLE, data);
}

struct wl_event_source_idle {
	struct wl_event_source base;
	wl_event_loop_idle_func_t func;
};

struct wl_event_source_interface idle_source_interface = {
	NULL,
};

WL_EXPORT struct wl_event_source *
wl_event_loop_add_idle(struct wl_event_loop *loop,
		       wl_event_loop_idle_func_t func,
		       void *data)
{
	struct wl_event_source_idle *source;

	source = malloc(sizeof *source);
	if (source == NULL)
		return NULL;

	source->base.interface = &idle_source_interface;
	source->base.loop = loop;
	source->base.fd = -1;

	source->func = func;
	source->base.data = data;

	wl_list_insert(loop->idle_list.prev, &source->base.link);

	return &source->base;
}

WL_EXPORT void
wl_event_source_check(struct wl_event_source *source)
{
	wl_list_insert(source->loop->check_list.prev, &source->link);
}

WL_EXPORT int
wl_event_source_remove(struct wl_event_source *source)
{
	struct wl_event_loop *loop = source->loop;

	/* We need to explicitly remove the fd, since closing the fd
	 * isn't enough in case we've dup'ed the fd. */
	if (source->fd >= 0) {
		epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, source->fd, NULL);
		close(source->fd);
		source->fd = -1;
	}

	wl_list_remove(&source->link);
	wl_list_insert(&loop->destroy_list, &source->link);

	return 0;
}

static void
wl_event_loop_process_destroy_list(struct wl_event_loop *loop)
{
	struct wl_event_source *source, *next;

	wl_list_for_each_safe(source, next, &loop->destroy_list, link)
		free(source);

	wl_list_init(&loop->destroy_list);
}

WL_EXPORT struct wl_event_loop *
wl_event_loop_create(void)
{
	struct wl_event_loop *loop;

	loop = malloc(sizeof *loop);
	if (loop == NULL)
		return NULL;

	loop->epoll_fd = wl_os_epoll_create_cloexec();
	if (loop->epoll_fd < 0) {
		free(loop);
		return NULL;
	}
	wl_list_init(&loop->check_list);
	wl_list_init(&loop->idle_list);
	wl_list_init(&loop->destroy_list);

	wl_signal_init(&loop->destroy_signal);

	return loop;
}

WL_EXPORT void
wl_event_loop_destroy(struct wl_event_loop *loop)
{
	wl_signal_emit(&loop->destroy_signal, loop);

	wl_event_loop_process_destroy_list(loop);
	close(loop->epoll_fd);
	free(loop);
}

static int
post_dispatch_check(struct wl_event_loop *loop)
{
	struct epoll_event ep;
	struct wl_event_source *source, *next;
	int n;

	ep.events = 0;
	n = 0;
	wl_list_for_each_safe(source, next, &loop->check_list, link)
		n += source->interface->dispatch(source, &ep);

	return n;
}

WL_EXPORT void
wl_event_loop_dispatch_idle(struct wl_event_loop *loop)
{
	struct wl_event_source_idle *source;

	while (!wl_list_empty(&loop->idle_list)) {
		source = container_of(loop->idle_list.next,
				      struct wl_event_source_idle, base.link);
		source->func(source->base.data);
		wl_event_source_remove(&source->base);
	}
}

WL_EXPORT int
wl_event_loop_dispatch(struct wl_event_loop *loop, int timeout)
{
	struct epoll_event ep[32];
	struct wl_event_source *source;
	int i, count, n;

	wl_event_loop_dispatch_idle(loop);

	count = epoll_wait(loop->epoll_fd, ep, ARRAY_LENGTH(ep), timeout);
	if (count < 0)
		return -1;

	for (i = 0; i < count; i++) {
		source = ep[i].data.ptr;
		if (source->fd != -1)
			source->interface->dispatch(source, &ep[i]);
	}

	wl_event_loop_process_destroy_list(loop);

	wl_event_loop_dispatch_idle(loop);

	do {
		n = post_dispatch_check(loop);
	} while (n > 0);

	return 0;
}

WL_EXPORT int
wl_event_loop_get_fd(struct wl_event_loop *loop)
{
	return loop->epoll_fd;
}

WL_EXPORT void
wl_event_loop_add_destroy_listener(struct wl_event_loop *loop,
				   struct wl_listener *listener)
{
	wl_signal_add(&loop->destroy_signal, listener);
}

WL_EXPORT struct wl_listener *
wl_event_loop_get_destroy_listener(struct wl_event_loop *loop,
				   wl_notify_func_t notify)
{
	return wl_signal_get(&loop->destroy_signal, notify);
}

