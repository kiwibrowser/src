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

#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "test-runner.h"

int
count_open_fds(void)
{
	DIR *dir;
	struct dirent *ent;
	int count = 0;

	dir = opendir("/proc/self/fd");
	assert(dir && "opening /proc/self/fd failed.");

	errno = 0;
	while ((ent = readdir(dir))) {
		const char *s = ent->d_name;
		if (s[0] == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)))
			continue;
		count++;
	}
	assert(errno == 0 && "reading /proc/self/fd failed.");

	closedir(dir);

	return count;
}

void
exec_fd_leak_check(int nr_expected_fds)
{
	const char *exe = "./exec-fd-leak-checker";
	char number[16] = { 0 };

	snprintf(number, sizeof number - 1, "%d", nr_expected_fds);
	execl(exe, exe, number, (char *)NULL);
	assert(0 && "execing fd leak checker failed");
}

#define USEC_TO_NSEC(n) (1000 * (n))

/* our implementation of usleep and sleep functions that are safe to use with
 * timeouts (timeouts are implemented using alarm(), so it is not safe use
 * usleep and sleep. See man pages of these functions)
 */
void
test_usleep(useconds_t usec)
{
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = USEC_TO_NSEC(usec)
	};

	assert(nanosleep(&ts, NULL) == 0);
}

/* we must write the whole function instead of
 * wrapping test_usleep, because useconds_t may not
 * be able to contain such a big number of microseconds */
void
test_sleep(unsigned int sec)
{
	struct timespec ts = {
		.tv_sec = sec,
		.tv_nsec = 0
	};

	assert(nanosleep(&ts, NULL) == 0);
}
