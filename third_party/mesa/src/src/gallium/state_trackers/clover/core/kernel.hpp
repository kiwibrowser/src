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

#ifndef __CORE_KERNEL_HPP__
#define __CORE_KERNEL_HPP__

#include <memory>

#include "core/base.hpp"
#include "core/program.hpp"
#include "core/memory.hpp"
#include "core/sampler.hpp"
#include "pipe/p_state.h"

namespace clover {
   typedef struct _cl_kernel kernel;
   class argument;
}

struct _cl_kernel : public clover::ref_counter {
private:
   ///
   /// Class containing all the state required to execute a compute
   /// kernel.
   ///
   struct exec_context {
      exec_context(clover::kernel &kern);
      ~exec_context();

      void *bind(clover::command_queue *q);
      void unbind();

      clover::kernel &kern;
      clover::command_queue *q;

      std::vector<uint8_t> input;
      std::vector<void *> samplers;
      std::vector<pipe_sampler_view *> sviews;
      std::vector<pipe_surface *> resources;
      std::vector<pipe_resource *> g_buffers;
      std::vector<size_t> g_handles;
      size_t mem_local;

   private:
      void *st;
      pipe_compute_state cs;
   };

public:
   class argument {
   public:
      argument(size_t size);

      /// \a true if the argument has been set.
      bool set() const;

      /// Argument size in the input buffer.
      size_t size() const;

      /// Storage space required for the referenced object.
      virtual size_t storage() const;

      /// Set this argument to some object.
      virtual void set(size_t size, const void *value) = 0;

      /// Allocate the necessary resources to bind the specified
      /// object to this argument, and update \a ctx accordingly.
      virtual void bind(exec_context &ctx) = 0;

      /// Free any resources that were allocated in bind().
      virtual void unbind(exec_context &ctx) = 0;

   protected:
      size_t __size;
      bool __set;
   };

   _cl_kernel(clover::program &prog,
              const std::string &name,
              const std::vector<clover::module::argument> &args);

   void launch(clover::command_queue &q,
               const std::vector<size_t> &grid_offset,
               const std::vector<size_t> &grid_size,
               const std::vector<size_t> &block_size);

   size_t mem_local() const;
   size_t mem_private() const;
   size_t max_block_size() const;

   const std::string &name() const;
   std::vector<size_t> block_size() const;

   clover::program &prog;
   std::vector<std::unique_ptr<argument>> args;

private:
   const clover::module &
   module(const clover::command_queue &q) const;

   class scalar_argument : public argument {
   public:
      scalar_argument(size_t size);

      virtual void set(size_t size, const void *value);
      virtual void bind(exec_context &ctx);
      virtual void unbind(exec_context &ctx);

   private:
      std::vector<uint8_t> v;
   };

   class global_argument : public argument {
   public:
      global_argument(size_t size);

      virtual void set(size_t size, const void *value);
      virtual void bind(exec_context &ctx);
      virtual void unbind(exec_context &ctx);

   private:
      clover::buffer *obj;
   };

   class local_argument : public argument {
   public:
      local_argument();

      virtual size_t storage() const;

      virtual void set(size_t size, const void *value);
      virtual void bind(exec_context &ctx);
      virtual void unbind(exec_context &ctx);

   private:
      size_t __storage;
   };

   class constant_argument : public argument {
   public:
      constant_argument();

      virtual void set(size_t size, const void *value);
      virtual void bind(exec_context &ctx);
      virtual void unbind(exec_context &ctx);

   private:
      clover::buffer *obj;
      pipe_surface *st;
   };

   class image_rd_argument : public argument {
   public:
      image_rd_argument();

      virtual void set(size_t size, const void *value);
      virtual void bind(exec_context &ctx);
      virtual void unbind(exec_context &ctx);

   private:
      clover::image *obj;
      pipe_sampler_view *st;
   };

   class image_wr_argument : public argument {
   public:
      image_wr_argument();

      virtual void set(size_t size, const void *value);
      virtual void bind(exec_context &ctx);
      virtual void unbind(exec_context &ctx);

   private:
      clover::image *obj;
      pipe_surface *st;
   };

   class sampler_argument : public argument {
   public:
      sampler_argument();

      virtual void set(size_t size, const void *value);
      virtual void bind(exec_context &ctx);
      virtual void unbind(exec_context &ctx);

   private:
      clover::sampler *obj;
      void *st;
   };

   std::string __name;
   exec_context exec;
};

#endif
