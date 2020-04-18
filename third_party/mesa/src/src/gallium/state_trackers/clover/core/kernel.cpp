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

#include "core/kernel.hpp"
#include "core/resource.hpp"
#include "pipe/p_context.h"

using namespace clover;

_cl_kernel::_cl_kernel(clover::program &prog,
                       const std::string &name,
                       const std::vector<clover::module::argument> &args) :
   prog(prog), __name(name), exec(*this) {
   for (auto arg : args) {
      if (arg.type == module::argument::scalar)
         this->args.emplace_back(new scalar_argument(arg.size));
      else if (arg.type == module::argument::global)
         this->args.emplace_back(new global_argument(arg.size));
      else if (arg.type == module::argument::local)
         this->args.emplace_back(new local_argument());
      else if (arg.type == module::argument::constant)
         this->args.emplace_back(new constant_argument());
      else if (arg.type == module::argument::image2d_rd ||
               arg.type == module::argument::image3d_rd)
         this->args.emplace_back(new image_rd_argument());
      else if (arg.type == module::argument::image2d_wr ||
               arg.type == module::argument::image3d_wr)
         this->args.emplace_back(new image_wr_argument());
      else if (arg.type == module::argument::sampler)
         this->args.emplace_back(new sampler_argument());
      else
         throw error(CL_INVALID_KERNEL_DEFINITION);
   }
}

template<typename T, typename V>
static inline std::vector<T>
pad_vector(clover::command_queue &q, const V &v, T x) {
   std::vector<T> w { v.begin(), v.end() };
   w.resize(q.dev.max_block_size().size(), x);
   return w;
}

void
_cl_kernel::launch(clover::command_queue &q,
                   const std::vector<size_t> &grid_offset,
                   const std::vector<size_t> &grid_size,
                   const std::vector<size_t> &block_size) {
   void *st = exec.bind(&q);
   auto g_handles = map([&](size_t h) { return (uint32_t *)&exec.input[h]; },
                        exec.g_handles.begin(), exec.g_handles.end());

   q.pipe->bind_compute_state(q.pipe, st);
   q.pipe->bind_compute_sampler_states(q.pipe, 0, exec.samplers.size(),
                                       exec.samplers.data());
   q.pipe->set_compute_sampler_views(q.pipe, 0, exec.sviews.size(),
                                     exec.sviews.data());
   q.pipe->set_compute_resources(q.pipe, 0, exec.resources.size(),
                                     exec.resources.data());
   q.pipe->set_global_binding(q.pipe, 0, exec.g_buffers.size(),
                              exec.g_buffers.data(), g_handles.data());

   q.pipe->launch_grid(q.pipe,
                       pad_vector<uint>(q, block_size, 1).data(),
                       pad_vector<uint>(q, grid_size, 1).data(),
                       module(q).sym(__name).offset,
                       exec.input.data());

   q.pipe->set_global_binding(q.pipe, 0, exec.g_buffers.size(), NULL, NULL);
   q.pipe->set_compute_resources(q.pipe, 0, exec.resources.size(), NULL);
   q.pipe->set_compute_sampler_views(q.pipe, 0, exec.sviews.size(), NULL);
   q.pipe->bind_compute_sampler_states(q.pipe, 0, exec.samplers.size(), NULL);
   exec.unbind();
}

size_t
_cl_kernel::mem_local() const {
   size_t sz = 0;

   for (auto &arg : args) {
      if (dynamic_cast<local_argument *>(arg.get()))
         sz += arg->storage();
   }

   return sz;
}

size_t
_cl_kernel::mem_private() const {
   return 0;
}

size_t
_cl_kernel::max_block_size() const {
   return SIZE_MAX;
}

const std::string &
_cl_kernel::name() const {
   return __name;
}

std::vector<size_t>
_cl_kernel::block_size() const {
   return { 0, 0, 0 };
}

const clover::module &
_cl_kernel::module(const clover::command_queue &q) const {
   return prog.binaries().find(&q.dev)->second;
}


_cl_kernel::exec_context::exec_context(clover::kernel &kern) :
   kern(kern), q(NULL), mem_local(0), st(NULL) {
}

_cl_kernel::exec_context::~exec_context() {
   if (st)
      q->pipe->delete_compute_state(q->pipe, st);
}

void *
_cl_kernel::exec_context::bind(clover::command_queue *__q) {
   std::swap(q, __q);

   for (auto &arg : kern.args)
      arg->bind(*this);

   // Create a new compute state if anything changed.
   if (!st || q != __q ||
       cs.req_local_mem != mem_local ||
       cs.req_input_mem != input.size()) {
      if (st)
         __q->pipe->delete_compute_state(__q->pipe, st);

      cs.prog = kern.module(*q).sec(module::section::text).data.begin();
      cs.req_local_mem = mem_local;
      cs.req_input_mem = input.size();
      st = q->pipe->create_compute_state(q->pipe, &cs);
   }

   return st;
}

void
_cl_kernel::exec_context::unbind() {
   for (auto &arg : kern.args)
      arg->unbind(*this);

   input.clear();
   samplers.clear();
   sviews.clear();
   resources.clear();
   g_buffers.clear();
   g_handles.clear();
   mem_local = 0;
}

_cl_kernel::argument::argument(size_t size) :
   __size(size), __set(false) {
}

bool
_cl_kernel::argument::set() const {
   return __set;
}

size_t
_cl_kernel::argument::storage() const {
   return 0;
}

_cl_kernel::scalar_argument::scalar_argument(size_t size) :
   argument(size) {
}

void
_cl_kernel::scalar_argument::set(size_t size, const void *value) {
   if (size != __size)
      throw error(CL_INVALID_ARG_SIZE);

   v = { (uint8_t *)value, (uint8_t *)value + size };
   __set = true;
}

void
_cl_kernel::scalar_argument::bind(exec_context &ctx) {
   ctx.input.insert(ctx.input.end(), v.begin(), v.end());
}

void
_cl_kernel::scalar_argument::unbind(exec_context &ctx) {
}

_cl_kernel::global_argument::global_argument(size_t size) :
   argument(size) {
}

void
_cl_kernel::global_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_mem))
      throw error(CL_INVALID_ARG_SIZE);

   obj = dynamic_cast<clover::buffer *>(*(cl_mem *)value);
   __set = true;
}

void
_cl_kernel::global_argument::bind(exec_context &ctx) {
   size_t offset = ctx.input.size();
   size_t idx = ctx.g_buffers.size();

   ctx.input.resize(offset + __size);

   ctx.g_buffers.resize(idx + 1);
   ctx.g_buffers[idx] = obj->resource(ctx.q).pipe;

   ctx.g_handles.resize(idx + 1);
   ctx.g_handles[idx] = offset;
}

void
_cl_kernel::global_argument::unbind(exec_context &ctx) {
}

_cl_kernel::local_argument::local_argument() :
   argument(sizeof(uint32_t)) {
}

size_t
_cl_kernel::local_argument::storage() const {
   return __storage;
}

void
_cl_kernel::local_argument::set(size_t size, const void *value) {
   if (value)
      throw error(CL_INVALID_ARG_VALUE);

   __storage = size;
   __set = true;
}

void
_cl_kernel::local_argument::bind(exec_context &ctx) {
   size_t offset = ctx.input.size();
   size_t ptr = ctx.mem_local;

   ctx.input.resize(offset + sizeof(uint32_t));
   *(uint32_t *)&ctx.input[offset] = ptr;

   ctx.mem_local += __storage;
}

void
_cl_kernel::local_argument::unbind(exec_context &ctx) {
}

_cl_kernel::constant_argument::constant_argument() :
   argument(sizeof(uint32_t)) {
}

void
_cl_kernel::constant_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_mem))
      throw error(CL_INVALID_ARG_SIZE);

   obj = dynamic_cast<clover::buffer *>(*(cl_mem *)value);
   __set = true;
}

void
_cl_kernel::constant_argument::bind(exec_context &ctx) {
   size_t offset = ctx.input.size();
   size_t idx = ctx.resources.size();

   ctx.input.resize(offset + sizeof(uint32_t));
   *(uint32_t *)&ctx.input[offset] = idx << 24;

   ctx.resources.resize(idx + 1);
   ctx.resources[idx] = st = obj->resource(ctx.q).bind_surface(*ctx.q, false);
}

void
_cl_kernel::constant_argument::unbind(exec_context &ctx) {
   obj->resource(ctx.q).unbind_surface(*ctx.q, st);
}

_cl_kernel::image_rd_argument::image_rd_argument() :
   argument(sizeof(uint32_t)) {
}

void
_cl_kernel::image_rd_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_mem))
      throw error(CL_INVALID_ARG_SIZE);

   obj = dynamic_cast<clover::image *>(*(cl_mem *)value);
   __set = true;
}

void
_cl_kernel::image_rd_argument::bind(exec_context &ctx) {
   size_t offset = ctx.input.size();
   size_t idx = ctx.sviews.size();

   ctx.input.resize(offset + sizeof(uint32_t));
   *(uint32_t *)&ctx.input[offset] = idx;

   ctx.sviews.resize(idx + 1);
   ctx.sviews[idx] = st = obj->resource(ctx.q).bind_sampler_view(*ctx.q);
}

void
_cl_kernel::image_rd_argument::unbind(exec_context &ctx) {
   obj->resource(ctx.q).unbind_sampler_view(*ctx.q, st);
}

_cl_kernel::image_wr_argument::image_wr_argument() :
   argument(sizeof(uint32_t)) {
}

void
_cl_kernel::image_wr_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_mem))
      throw error(CL_INVALID_ARG_SIZE);

   obj = dynamic_cast<clover::image *>(*(cl_mem *)value);
   __set = true;
}

void
_cl_kernel::image_wr_argument::bind(exec_context &ctx) {
   size_t offset = ctx.input.size();
   size_t idx = ctx.resources.size();

   ctx.input.resize(offset + sizeof(uint32_t));
   *(uint32_t *)&ctx.input[offset] = idx;

   ctx.resources.resize(idx + 1);
   ctx.resources[idx] = st = obj->resource(ctx.q).bind_surface(*ctx.q, true);
}

void
_cl_kernel::image_wr_argument::unbind(exec_context &ctx) {
   obj->resource(ctx.q).unbind_surface(*ctx.q, st);
}

_cl_kernel::sampler_argument::sampler_argument() :
   argument(0) {
}

void
_cl_kernel::sampler_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_sampler))
      throw error(CL_INVALID_ARG_SIZE);

   obj = *(cl_sampler *)value;
   __set = true;
}

void
_cl_kernel::sampler_argument::bind(exec_context &ctx) {
   size_t idx = ctx.samplers.size();

   ctx.samplers.resize(idx + 1);
   ctx.samplers[idx] = st = obj->bind(*ctx.q);
}

void
_cl_kernel::sampler_argument::unbind(exec_context &ctx) {
   obj->unbind(*ctx.q, st);
}
