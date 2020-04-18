/*
 * Copyright (c) 2014 Red Hat, Inc.
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

#define WL_HIDE_DEPRECATED

#include "test-runner.h"
#include "test-compositor.h"

/* --- Protocol --- */
struct test_compositor;

static const struct wl_message tc_requests[] = {
	/* this request serves as a barrier for synchronizing*/
	{ "stop_display", "u", NULL }
};

static const struct wl_message tc_events[] = {
	{ "display_resumed", "", NULL }
};

const struct wl_interface test_compositor_interface = {
	"test", 1,
	1, tc_requests,
	1, tc_events
};

struct test_compositor_interface {
	void (*stop_display)(struct wl_client *client,
			     struct wl_resource *resource,
			     uint32_t num);
};

struct test_compositor_listener {
	void (*display_resumed)(void *data, struct test_compositor *tc);

};

enum {
	STOP_DISPLAY = 0
};

enum {
	DISPLAY_RESUMED = 0
};

/* Since tests can run parallelly, we need unique socket names
 * for each test, otherwise the test can fail on wl_display_add_socket. */
static const char *
get_socket_name(void)
{
	struct timeval tv;
	static char retval[64];

	gettimeofday(&tv, NULL);
	snprintf(retval, sizeof retval, "wayland-test-%d-%ld%ld",
		 getpid(), tv.tv_sec, tv.tv_usec);

	return retval;
}

static void
handle_client_destroy(void *data)
{
	struct client_info *ci = data;
	struct display *d;
	siginfo_t status;

	d = ci->display;

	assert(waitid(P_PID, ci->pid, &status, WEXITED) != -1);

	switch (status.si_code) {
	case CLD_KILLED:
	case CLD_DUMPED:
		fprintf(stderr, "Client '%s' was killed by signal %d\n",
			ci->name, status.si_status);
		ci->exit_code = status.si_status;
		break;
	case CLD_EXITED:
		if (status.si_status != EXIT_SUCCESS)
			fprintf(stderr, "Client '%s' exited with code %d\n",
				ci->name, status.si_status);

		ci->exit_code = status.si_status;
		break;
	}

	++d->clients_terminated_no;
	if (d->clients_no == d->clients_terminated_no) {
		wl_display_terminate(d->wl_display);
	}

	/* the clients are not removed from the list, because
	 * at the end of the test we check the exit codes of all
	 * clients. In the case that the test would go through
	 * the clients list manually, zero out the wl_client as a sign
	 * that the client is not running anymore */
}

/**
 * Check client's state and terminate display when all clients exited
 */
static void
client_destroyed(struct wl_listener *listener, void *data)
{
	struct client_info *ci;
	struct display *d;
	struct wl_event_loop *loop;

	/* Wait for client in an idle handler to avoid blocking the actual
	 * client destruction (fd close etc. */
	ci = wl_container_of(listener, ci, destroy_listener);
	d = ci->display;
	loop = wl_display_get_event_loop(d->wl_display);
	wl_event_loop_add_idle(loop, handle_client_destroy, ci);

	ci->wl_client = NULL;
}

static void
run_client(void (*client_main)(void *data), void *data,
	   int wayland_sock, int client_pipe)
{
	char s[8];
	int cur_alloc, cur_fds;
	int can_continue = 0;

	/* Wait until display signals that client can continue */
	assert(read(client_pipe, &can_continue, sizeof(int)) == sizeof(int));

	if (can_continue == 0)
		abort(); /* error in parent */

	/* for wl_display_connect() */
	snprintf(s, sizeof s, "%d", wayland_sock);
	setenv("WAYLAND_SOCKET", s, 0);

	cur_alloc = get_current_alloc_num();
	cur_fds = count_open_fds();

	client_main(data);

	/* Clients using wl_display_connect() will end up closing the socket
	 * passed in through the WAYLAND_SOCKET environment variable. When
	 * doing this, it clears the environment variable, so if it's been
	 * unset, then we assume the client consumed the file descriptor and
	 * do not count it towards leak checking. */
	if (!getenv("WAYLAND_SOCKET"))
		cur_fds--;

	check_leaks(cur_alloc, cur_fds);
}

static struct client_info *
display_create_client(struct display *d,
		      void (*client_main)(void *data),
		      void *data,
		      const char *name)
{
	int pipe_cli[2];
	int sock_wayl[2];
	pid_t pid;
	int can_continue = 0;
	struct client_info *cl;

	assert(pipe(pipe_cli) == 0 && "Failed creating pipe");
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sock_wayl) == 0
	       && "Failed creating socket pair");

	pid = fork();
	assert(pid != -1 && "Fork failed");

	if (pid == 0) {
		close(sock_wayl[1]);
		close(pipe_cli[1]);

		run_client(client_main, data, sock_wayl[0], pipe_cli[0]);

		close(sock_wayl[0]);
		close(pipe_cli[0]);

		exit(0);
	}

	close(sock_wayl[0]);
	close(pipe_cli[0]);

	cl = calloc(1, sizeof(struct client_info));
	assert(cl && "Out of memory");

	wl_list_insert(&d->clients, &cl->link);

	cl->display = d;
	cl->name = name;
	cl->pid = pid;
	cl->pipe = pipe_cli[1];
	cl->destroy_listener.notify = &client_destroyed;

	cl->wl_client = wl_client_create(d->wl_display, sock_wayl[1]);
	if (!cl->wl_client) {
		int ret;

		/* abort the client */
		ret = write(cl->pipe, &can_continue, sizeof(int));
		assert(ret == sizeof(int) && "aborting the client failed");
		assert(0 && "Couldn't create wayland client");
	}

	wl_client_add_destroy_listener(cl->wl_client,
				       &cl->destroy_listener);

	++d->clients_no;

	return cl;
}

struct client_info *
client_create_with_name(struct display *d,
			void (*client_main)(void *data), void *data,
			const char *name)
{
	int can_continue = 1;
	struct client_info *cl = display_create_client(d,
						       client_main, data,
						       name);

	/* let the show begin! */
	assert(write(cl->pipe, &can_continue, sizeof(int)) == sizeof(int));

	return cl;
}

/* wfr = waiting for resume */
struct wfr {
	struct wl_resource *resource;
	struct wl_list link;
};

static void
handle_stop_display(struct wl_client *client,
		    struct wl_resource *resource, uint32_t num)
{
	struct display *d = wl_resource_get_user_data(resource);
	struct wfr *wfr;

	assert(d->wfr_num < num
	       && "test error: Too many clients sent stop_display request");

	++d->wfr_num;

	wfr = malloc(sizeof *wfr);
	if (!wfr) {
		wl_client_post_no_memory(client);
		assert(0 && "Out of memory");
	}

	wfr->resource = resource;
	wl_list_insert(&d->waiting_for_resume, &wfr->link);

	if (d->wfr_num == num)
		wl_display_terminate(d->wl_display);
}

static const struct test_compositor_interface tc_implementation = {
	handle_stop_display
};

static void
tc_bind(struct wl_client *client, void *data,
	uint32_t ver, uint32_t id)
{
	struct wl_resource *res;

	res = wl_resource_create(client, &test_compositor_interface, ver, id);
	if (!res) {
		wl_client_post_no_memory(client);
		assert(0 && "Out of memory");
	}

	wl_resource_set_implementation(res, &tc_implementation, data, NULL);
}

struct display *
display_create(void)
{
	struct display *d = NULL;
	struct wl_global *g;
	const char *socket_name;
	int stat = 0;

	d = calloc(1, sizeof *d);
	assert(d && "Out of memory");

	d->wl_display = wl_display_create();
	assert(d->wl_display && "Creating display failed");

	/* hope the path won't be longer than 108 ... */
	socket_name = get_socket_name();
	stat = wl_display_add_socket(d->wl_display, socket_name);
	assert(stat == 0 && "Failed adding socket");

	wl_list_init(&d->clients);
	d->clients_no = d->clients_terminated_no = 0;

	wl_list_init(&d->waiting_for_resume);
	d->wfr_num = 0;

	g = wl_global_create(d->wl_display, &test_compositor_interface,
			     1, d, tc_bind);
	assert(g && "Creating test global failed");

	return d;
}

void
display_run(struct display *d)
{
	assert(d->wfr_num == 0
	       && "test error: Have waiting clients. Use display_resume.");
	wl_display_run(d->wl_display);
}

void
display_resume(struct display *d)
{
	struct wfr *wfr, *next;

	assert(d->wfr_num > 0 && "test error: No clients waiting.");

	wl_list_for_each_safe(wfr, next, &d->waiting_for_resume, link) {
		wl_resource_post_event(wfr->resource, DISPLAY_RESUMED);
		wl_list_remove(&wfr->link);
		free(wfr);
	}

	assert(wl_list_empty(&d->waiting_for_resume));
	d->wfr_num = 0;

	wl_display_run(d->wl_display);
}

void
display_destroy(struct display *d)
{
	struct client_info *cl, *next;
	int failed = 0;

	assert(d->wfr_num == 0
	       && "test error: Didn't you forget to call display_resume?");

	wl_list_for_each_safe(cl, next, &d->clients, link) {
		assert(cl->wl_client == NULL);

		if (cl->exit_code != 0) {
			++failed;
			fprintf(stderr, "Client '%s' failed\n", cl->name);
		}

		close(cl->pipe);
		free(cl);
	}

	wl_display_destroy(d->wl_display);
	free(d);

	if (failed) {
		fprintf(stderr, "%d child(ren) failed\n", failed);
		abort();
	}
}

/*
 * --- Client helper functions ---
 */
static void
handle_display_resumed(void *data, struct test_compositor *tc)
{
	struct client *c = data;

	c->display_stopped = 0;
}

static const struct test_compositor_listener tc_listener = {
	handle_display_resumed
};

static void
registry_handle_globals(void *data, struct wl_registry *registry,
			uint32_t id, const char *intf, uint32_t ver)
{
	struct client *c = data;

	if (strcmp(intf, "test") != 0)
		return;

	c->tc = wl_registry_bind(registry, id, &test_compositor_interface, ver);
	assert(c->tc && "Failed binding to registry");

	wl_proxy_add_listener((struct wl_proxy *) c->tc,
			      (void *) &tc_listener, c);
}

static const struct wl_registry_listener registry_listener =
{
	registry_handle_globals,
	NULL
};

struct client *client_connect()
{
	struct wl_registry *reg;
	struct client *c = calloc(1, sizeof *c);
	assert(c && "Out of memory");

	c->wl_display = wl_display_connect(NULL);
	assert(c->wl_display && "Failed connecting to display");

	/* create test_compositor proxy. Do it with temporary
	 * registry so that client can define it's own listener later */
	reg = wl_display_get_registry(c->wl_display);
	assert(reg);
	wl_registry_add_listener(reg, &registry_listener, c);
	wl_display_roundtrip(c->wl_display);
	assert(c->tc);

	wl_registry_destroy(reg);

	return c;
}

static void
check_error(struct wl_display *display)
{
	uint32_t ec, id;
	const struct wl_interface *intf;
	int err;

	err = wl_display_get_error(display);
	/* write out message about protocol error */
	if (err == EPROTO) {
		ec = wl_display_get_protocol_error(display, &intf, &id);
		fprintf(stderr, "Client: Got protocol error %u on interface %s"
				" (object %u)\n", ec, intf->name, id);
	}

	if (err) {
		fprintf(stderr, "Client error: %s\n", strerror(err));
		abort();
	}
}

void
client_disconnect(struct client *c)
{
	/* check for errors */
	check_error(c->wl_display);

	wl_proxy_destroy((struct wl_proxy *) c->tc);
	wl_display_disconnect(c->wl_display);
	free(c);
}

/* num is number of clients that requests to stop display.
 * Display is stopped after it receives num STOP_DISPLAY requests */
int
stop_display(struct client *c, int num)
{
	int n = 0;

	c->display_stopped = 1;
	wl_proxy_marshal((struct wl_proxy *) c->tc, STOP_DISPLAY, num);

	while (c->display_stopped && n >= 0) {
		n = wl_display_dispatch(c->wl_display);
	}

	return n;
}
