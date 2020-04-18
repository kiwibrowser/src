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

#ifndef __CORE_EVENT_HPP__
#define __CORE_EVENT_HPP__

#include <functional>

#include "core/base.hpp"
#include "core/queue.hpp"

namespace clover {
   typedef struct _cl_event event;
}

///
/// Class that represents a task that might be executed asynchronously
/// at some point in the future.
///
/// An event consists of a list of dependencies, a boolean signalled()
/// flag, and an associated task.  An event is considered signalled as
/// soon as all its dependencies (if any) are signalled as well, and
/// the trigger() method is called; at that point the associated task
/// will be started through the specified \a action_ok.  If the
/// abort() method is called instead, the specified \a action_fail is
/// executed and the associated task will never be started.  Dependent
/// events will be aborted recursively.
///
/// The execution status of the associated task can be queried using
/// the status() method, and it can be waited for completion using the
/// wait() method.
///
struct _cl_event : public clover::ref_counter {
public:
   typedef std::function<void (clover::event &)> action;

   _cl_event(clover::context &ctx, std::vector<clover::event *> deps,
             action action_ok, action action_fail);
   virtual ~_cl_event();

   void trigger();
   void abort(cl_int status);
   bool signalled() const;

   virtual cl_int status() const = 0;
   virtual cl_command_queue queue() const = 0;
   virtual cl_command_type command() const = 0;
   virtual void wait() const = 0;

   clover::context &ctx;

protected:
   void chain(clover::event *ev);

   cl_int __status;
   std::vector<clover::ref_ptr<clover::event>> deps;

private:
   unsigned wait_count;
   action action_ok;
   action action_fail;
   std::vector<clover::ref_ptr<clover::event>> __chain;
};

namespace clover {
   ///
   /// Class that represents a task executed by a command queue.
   ///
   /// Similar to a normal clover::event.  In addition it's associated
   /// with a given command queue \a q and a given OpenCL \a command.
   /// hard_event instances created for the same queue are implicitly
   /// ordered with respect to each other, and they are implicitly
   /// triggered on construction.
   ///
   /// A hard_event is considered complete when the associated
   /// hardware task finishes execution.
   ///
   class hard_event : public event {
   public:
      hard_event(clover::command_queue &q, cl_command_type command,
                 std::vector<clover::event *> deps,
                 action action = [](event &){});
      ~hard_event();

      virtual cl_int status() const;
      virtual cl_command_queue queue() const;
      virtual cl_command_type command() const;
      virtual void wait() const;

      friend class ::_cl_command_queue;

   private:
      virtual void fence(pipe_fence_handle *fence);

      clover::command_queue &__queue;
      cl_command_type __command;
      pipe_fence_handle *__fence;
   };

   ///
   /// Class that represents a software event.
   ///
   /// A soft_event is not associated with any specific hardware task
   /// or command queue.  It's considered complete as soon as all its
   /// dependencies finish execution.
   ///
   class soft_event : public event {
   public:
      soft_event(clover::context &ctx, std::vector<clover::event *> deps,
                 bool trigger, action action = [](event &){});

      virtual cl_int status() const;
      virtual cl_command_queue queue() const;
      virtual cl_command_type command() const;
      virtual void wait() const;
   };
}

#endif
