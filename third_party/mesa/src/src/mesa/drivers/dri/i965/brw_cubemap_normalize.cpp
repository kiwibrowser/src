/*
 * Copyright © 2010 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file brw_cubemap_normalize.cpp
 *
 * IR lower pass to perform the normalization of the cubemap coordinates to
 * have the largest magnitude component be -1.0 or 1.0.
 *
 * \author Eric Anholt <eric@anholt.net>
 */

#include "glsl/glsl_types.h"
#include "glsl/ir.h"

class brw_cubemap_normalize_visitor : public ir_hierarchical_visitor {
public:
   brw_cubemap_normalize_visitor()
   {
      progress = false;
   }

   ir_visitor_status visit_leave(ir_texture *ir);

   bool progress;
};

ir_visitor_status
brw_cubemap_normalize_visitor::visit_leave(ir_texture *ir)
{
   if (ir->sampler->type->sampler_dimensionality != GLSL_SAMPLER_DIM_CUBE)
      return visit_continue;

   if (ir->op == ir_txs)
      return visit_continue;

   void *mem_ctx = ralloc_parent(ir);

   ir_variable *var = new(mem_ctx) ir_variable(ir->coordinate->type,
					       "coordinate", ir_var_auto);
   base_ir->insert_before(var);
   ir_dereference *deref = new(mem_ctx) ir_dereference_variable(var);
   ir_assignment *assign = new(mem_ctx) ir_assignment(deref, ir->coordinate,
						      NULL);
   base_ir->insert_before(assign);

   deref = new(mem_ctx) ir_dereference_variable(var);
   ir_rvalue *swiz0 = new(mem_ctx) ir_swizzle(deref, 0, 0, 0, 0, 1);
   deref = new(mem_ctx) ir_dereference_variable(var);
   ir_rvalue *swiz1 = new(mem_ctx) ir_swizzle(deref, 1, 0, 0, 0, 1);
   deref = new(mem_ctx) ir_dereference_variable(var);
   ir_rvalue *swiz2 = new(mem_ctx) ir_swizzle(deref, 2, 0, 0, 0, 1);

   swiz0 = new(mem_ctx) ir_expression(ir_unop_abs, swiz0->type, swiz0, NULL);
   swiz1 = new(mem_ctx) ir_expression(ir_unop_abs, swiz1->type, swiz1, NULL);
   swiz2 = new(mem_ctx) ir_expression(ir_unop_abs, swiz2->type, swiz2, NULL);

   ir_expression *expr;
   expr = new(mem_ctx) ir_expression(ir_binop_max,
				     glsl_type::float_type,
				     swiz0, swiz1);

   expr = new(mem_ctx) ir_expression(ir_binop_max,
				     glsl_type::float_type,
				     expr, swiz2);

   expr = new(mem_ctx) ir_expression(ir_unop_rcp,
				     glsl_type::float_type,
				     expr, NULL);

   deref = new(mem_ctx) ir_dereference_variable(var);
   ir->coordinate = new(mem_ctx) ir_expression(ir_binop_mul,
					       ir->coordinate->type,
					       deref,
					       expr);

   progress = true;
   return visit_continue;
}

extern "C" {

bool
brw_do_cubemap_normalize(exec_list *instructions)
{
   brw_cubemap_normalize_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}

}
