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

#include <algorithm>

#include "core/queue.hpp"
#include "core/event.hpp"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"

using namespace clover;

_cl_command_queue::_cl_command_queue(context &ctx, device &dev,
                                     cl_command_queue_properties props) :
   ctx(ctx), dev(dev), __props(props) {
   pipe = dev.pipe->context_create(dev.pipe, NULL);
   if (!pipe)
      throw error(CL_INVALID_DEVICE);
}

_cl_command_queue::~_cl_command_queue() {
   pipe->destroy(pipe);
}

void
_cl_command_queue::flush() {
   pipe_screen *screen = dev.pipe;
   pipe_fence_handle *fence = NULL;

   if (!queued_events.empty()) {
      // Find out which events have already been signalled.
      auto first = queued_events.begin();
      auto last = std::find_if(queued_events.begin(), queued_events.end(),
                               [](event_ptr &ev) { return !ev->signalled(); });

      // Flush and fence them.
      pipe->flush(pipe, &fence);
      std::for_each(first, last, [&](event_ptr &ev) { ev->fence(fence); });
      screen->fence_reference(screen, &fence, NULL);
      queued_events.erase(first, last);
   }
}

void
_cl_command_queue::sequence(clover::hard_event *ev) {
   if (!queued_events.empty())
      queued_events.back()->chain(ev);

   queued_events.push_back(ev);
}
