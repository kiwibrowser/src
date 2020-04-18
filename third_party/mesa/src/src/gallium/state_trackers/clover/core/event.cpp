//
// Copyright 2012 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "core/event.hpp"
#include "pipe/p_screen.h"

using namespace clover;

_cl_event::_cl_event(clover::context &ctx,
                     std::vector<clover::event *> deps,
                     action action_ok, action action_fail) :
   ctx(ctx), __status(0), wait_count(1),
   action_ok(action_ok), action_fail(action_fail) {
   for (auto ev : deps)
      ev->chain(this);
}

_cl_event::~_cl_event() {
}

void
_cl_event::trigger() {
   if (!--wait_count) {
      action_ok(*this);

      while (!__chain.empty()) {
         __chain.back()->trigger();
         __chain.pop_back();
      }
   }
}

void
_cl_event::abort(cl_int status) {
   __status = status;
   action_fail(*this);

   while (!__chain.empty()) {
      __chain.back()->abort(status);
      __chain.pop_back();
   }
}

bool
_cl_event::signalled() const {
   return !wait_count;
}

void
_cl_event::chain(clover::event *ev) {
   if (wait_count) {
      ev->wait_count++;
      __chain.push_back(ev);
      ev->deps.push_back(this);
   }
}

hard_event::hard_event(clover::command_queue &q, cl_command_type command,
                       std::vector<clover::event *> deps, action action) :
   _cl_event(q.ctx, deps, action, [](event &ev){}),
   __queue(q), __command(command), __fence(NULL) {
   q.sequence(this);
   trigger();
}

hard_event::~hard_event() {
   pipe_screen *screen = queue()->dev.pipe;
   screen->fence_reference(screen, &__fence, NULL);
}

cl_int
hard_event::status() const {
   pipe_screen *screen = queue()->dev.pipe;

   if (__status < 0)
      return __status;

   else if (!__fence)
      return CL_QUEUED;

   else if (!screen->fence_signalled(screen, __fence))
      return CL_SUBMITTED;

   else
      return CL_COMPLETE;
}

cl_command_queue
hard_event::queue() const {
   return &__queue;
}

cl_command_type
hard_event::command() const {
   return __command;
}

void
hard_event::wait() const {
   pipe_screen *screen = queue()->dev.pipe;

   if (status() == CL_QUEUED)
      queue()->flush();

   if (!__fence ||
       !screen->fence_finish(screen, __fence, PIPE_TIMEOUT_INFINITE))
      throw error(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
}

void
hard_event::fence(pipe_fence_handle *fence) {
   pipe_screen *screen = queue()->dev.pipe;
   screen->fence_reference(screen, &__fence, fence);
}

soft_event::soft_event(clover::context &ctx,
                       std::vector<clover::event *> deps,
                       bool __trigger, action action) :
   _cl_event(ctx, deps, action, action) {
   if (__trigger)
      trigger();
}

cl_int
soft_event::status() const {
   if (__status < 0)
      return __status;

   else if (!signalled() ||
            any_of([](const ref_ptr<event> &ev) {
                  return ev->status() != CL_COMPLETE;
               }, deps.begin(), deps.end()))
      return CL_SUBMITTED;

   else
      return CL_COMPLETE;
}

cl_command_queue
soft_event::queue() const {
   return NULL;
}

cl_command_type
soft_event::command() const {
   return CL_COMMAND_USER;
}

void
soft_event::wait() const {
   for (auto ev : deps)
      ev->wait();

   if (status() != CL_COMPLETE)
      throw error(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
}
