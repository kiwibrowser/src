/*
 * Copyright © 2012 Collabora, Ltd.
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

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#include "../config.h"
#include "wayland-os.h"

static int
set_cloexec_or_close(int fd)
{
	long flags;

	if (fd == -1)
		return -1;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		goto err;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		goto err;

	return fd;

err:
	close(fd);
	return -1;
}

int
wl_os_socket_cloexec(int domain, int type, int protocol)
{
	int fd;

	fd = socket(domain, type | SOCK_CLOEXEC, protocol);
	if (fd >= 0)
		return fd;
	if (errno != EINVAL)
		return -1;

	fd = socket(domain, type, protocol);
	return set_cloexec_or_close(fd);
}

int
wl_os_dupfd_cloexec(int fd, long minfd)
{
	int newfd;

	newfd = fcntl(fd, F_DUPFD_CLOEXEC, minfd);
	if (newfd >= 0)
		return newfd;
	if (errno != EINVAL)
		return -1;

	newfd = fcntl(fd, F_DUPFD, minfd);
	return set_cloexec_or_close(newfd);
}

static ssize_t
recvmsg_cloexec_fallback(int sockfd, struct msghdr *msg, int flags)
{
	ssize_t len;
	struct cmsghdr *cmsg;
	unsigned char *data;
	int *fd;
	int *end;

	len = recvmsg(sockfd, msg, flags);
	if (len == -1)
		return -1;

	if (!msg->msg_control || msg->msg_controllen == 0)
		return len;

	cmsg = CMSG_FIRSTHDR(msg);
	for (; cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg)) {
		if (cmsg->cmsg_level != SOL_SOCKET ||
		    cmsg->cmsg_type != SCM_RIGHTS)
			continue;

		data = CMSG_DATA(cmsg);
		end = (int *)(data + cmsg->cmsg_len - CMSG_LEN(0));
		for (fd = (int *)data; fd < end; ++fd)
			*fd = set_cloexec_or_close(*fd);
	}

	return len;
}

ssize_t
wl_os_recvmsg_cloexec(int sockfd, struct msghdr *msg, int flags)
{
	ssize_t len;

	len = recvmsg(sockfd, msg, flags | MSG_CMSG_CLOEXEC);
	if (len >= 0)
		return len;
	if (errno != EINVAL)
		return -1;

	return recvmsg_cloexec_fallback(sockfd, msg, flags);
}

int
wl_os_epoll_create_cloexec(void)
{
	int fd;

#ifdef EPOLL_CLOEXEC
	fd = epoll_create1(EPOLL_CLOEXEC);
	if (fd >= 0)
		return fd;
	if (errno != EINVAL)
		return -1;
#endif

	fd = epoll_create(1);
	return set_cloexec_or_close(fd);
}

int
wl_os_accept_cloexec(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int fd;

#ifdef HAVE_ACCEPT4
	fd = accept4(sockfd, addr, addrlen, SOCK_CLOEXEC);
	if (fd >= 0)
		return fd;
	if (errno != ENOSYS)
		return -1;
#endif

	fd = accept(sockfd, addr, addrlen);
	return set_cloexec_or_close(fd);
}
