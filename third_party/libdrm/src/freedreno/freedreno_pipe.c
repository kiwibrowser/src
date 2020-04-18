/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "freedreno_drmif.h"
#include "freedreno_priv.h"

struct fd_pipe *
fd_pipe_new(struct fd_device *dev, enum fd_pipe_id id)
{
	struct fd_pipe *pipe;
	uint64_t val;

	if (id > FD_PIPE_MAX) {
		ERROR_MSG("invalid pipe id: %d", id);
		return NULL;
	}

	pipe = dev->funcs->pipe_new(dev, id);
	if (!pipe) {
		ERROR_MSG("allocation failed");
		return NULL;
	}

	pipe->dev = dev;
	pipe->id = id;

	fd_pipe_get_param(pipe, FD_GPU_ID, &val);
	pipe->gpu_id = val;

	return pipe;
}

void fd_pipe_del(struct fd_pipe *pipe)
{
	pipe->funcs->destroy(pipe);
}

int fd_pipe_get_param(struct fd_pipe *pipe,
				 enum fd_param_id param, uint64_t *value)
{
	return pipe->funcs->get_param(pipe, param, value);
}

int fd_pipe_wait(struct fd_pipe *pipe, uint32_t timestamp)
{
	return fd_pipe_wait_timeout(pipe, timestamp, ~0);
}

int fd_pipe_wait_timeout(struct fd_pipe *pipe, uint32_t timestamp,
		uint64_t timeout)
{
	return pipe->funcs->wait(pipe, timestamp, timeout);
}
