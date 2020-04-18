/*
 * Copyright © 2012 Intel Corporation
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
#include <gtest/gtest.h>
#include "main/compiler.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "ralloc.h"
#include "uniform_initializer_utils.h"

namespace linker {
extern void
copy_constant_to_storage(union gl_constant_value *storage,
			 const ir_constant *val,
			 const enum glsl_base_type base_type,
			 const unsigned int elements);
}

class copy_constant_to_storage : public ::testing::Test {
public:
   void int_test(unsigned rows);
   void uint_test(unsigned rows);
   void bool_test(unsigned rows);
   void sampler_test();
   void float_test(unsigned columns, unsigned rows);

   virtual void SetUp();
   virtual void TearDown();

   gl_constant_value storage[17];
   void *mem_ctx;
};

void
copy_constant_to_storage::SetUp()
{
   this->mem_ctx = ralloc_context(NULL);
}

void
copy_constant_to_storage::TearDown()
{
   ralloc_free(this->mem_ctx);
   this->mem_ctx = NULL;
}

void
copy_constant_to_storage::int_test(unsigned rows)
{
   ir_constant *val;
   generate_data(mem_ctx, GLSL_TYPE_INT, 1, rows, val);

   const unsigned red_zone_size = Elements(storage) - val->type->components();
   fill_storage_array_with_sentinels(storage,
				     val->type->components(),
				     red_zone_size);

   linker::copy_constant_to_storage(storage,
				    val,
				    val->type->base_type,
				    val->type->components());

   verify_data(storage, 0, val, red_zone_size);
}

void
copy_constant_to_storage::uint_test(unsigned rows)
{
   ir_constant *val;
   generate_data(mem_ctx, GLSL_TYPE_UINT, 1, rows, val);

   const unsigned red_zone_size = Elements(storage) - val->type->components();
   fill_storage_array_with_sentinels(storage,
				     val->type->components(),
				     red_zone_size);

   linker::copy_constant_to_storage(storage,
				    val,
				    val->type->base_type,
				    val->type->components());

   verify_data(storage, 0, val, red_zone_size);
}

void
copy_constant_to_storage::float_test(unsigned columns, unsigned rows)
{
   ir_constant *val;
   generate_data(mem_ctx, GLSL_TYPE_FLOAT, columns, rows, val);

   const unsigned red_zone_size = Elements(storage) - val->type->components();
   fill_storage_array_with_sentinels(storage,
				     val->type->components(),
				     red_zone_size);

   linker::copy_constant_to_storage(storage,
				    val,
				    val->type->base_type,
				    val->type->components());

   verify_data(storage, 0, val, red_zone_size);
}

void
copy_constant_to_storage::bool_test(unsigned rows)
{
   ir_constant *val;
   generate_data(mem_ctx, GLSL_TYPE_BOOL, 1, rows, val);

   const unsigned red_zone_size = Elements(storage) - val->type->components();
   fill_storage_array_with_sentinels(storage,
				     val->type->components(),
				     red_zone_size);

   linker::copy_constant_to_storage(storage,
				    val,
				    val->type->base_type,
				    val->type->components());

   verify_data(storage, 0, val, red_zone_size);
}

/**
 * The only difference between this test and int_test is that the base type
 * passed to \c linker::copy_constant_to_storage is hard-coded to \c
 * GLSL_TYPE_SAMPLER instead of using the base type from the constant.
 */
void
copy_constant_to_storage::sampler_test(void)
{
   ir_constant *val;
   generate_data(mem_ctx, GLSL_TYPE_INT, 1, 1, val);

   const unsigned red_zone_size = Elements(storage) - val->type->components();
   fill_storage_array_with_sentinels(storage,
				     val->type->components(),
				     red_zone_size);

   linker::copy_constant_to_storage(storage,
				    val,
				    GLSL_TYPE_SAMPLER,
				    val->type->components());

   verify_data(storage, 0, val, red_zone_size);
}

TEST_F(copy_constant_to_storage, bool_uniform)
{
   bool_test(1);
}

TEST_F(copy_constant_to_storage, bvec2_uniform)
{
   bool_test(2);
}

TEST_F(copy_constant_to_storage, bvec3_uniform)
{
   bool_test(3);
}

TEST_F(copy_constant_to_storage, bvec4_uniform)
{
   bool_test(4);
}

TEST_F(copy_constant_to_storage, int_uniform)
{
   int_test(1);
}

TEST_F(copy_constant_to_storage, ivec2_uniform)
{
   int_test(2);
}

TEST_F(copy_constant_to_storage, ivec3_uniform)
{
   int_test(3);
}

TEST_F(copy_constant_to_storage, ivec4_uniform)
{
   int_test(4);
}

TEST_F(copy_constant_to_storage, uint_uniform)
{
   uint_test(1);
}

TEST_F(copy_constant_to_storage, uvec2_uniform)
{
   uint_test(2);
}

TEST_F(copy_constant_to_storage, uvec3_uniform)
{
   uint_test(3);
}

TEST_F(copy_constant_to_storage, uvec4_uniform)
{
   uint_test(4);
}

TEST_F(copy_constant_to_storage, float_uniform)
{
   float_test(1, 1);
}

TEST_F(copy_constant_to_storage, vec2_uniform)
{
   float_test(1, 2);
}

TEST_F(copy_constant_to_storage, vec3_uniform)
{
   float_test(1, 3);
}

TEST_F(copy_constant_to_storage, vec4_uniform)
{
   float_test(1, 4);
}

TEST_F(copy_constant_to_storage, mat2x2_uniform)
{
   float_test(2, 2);
}

TEST_F(copy_constant_to_storage, mat2x3_uniform)
{
   float_test(2, 3);
}

TEST_F(copy_constant_to_storage, mat2x4_uniform)
{
   float_test(2, 4);
}

TEST_F(copy_constant_to_storage, mat3x2_uniform)
{
   float_test(3, 2);
}

TEST_F(copy_constant_to_storage, mat3x3_uniform)
{
   float_test(3, 3);
}

TEST_F(copy_constant_to_storage, mat3x4_uniform)
{
   float_test(3, 4);
}

TEST_F(copy_constant_to_storage, mat4x2_uniform)
{
   float_test(4, 2);
}

TEST_F(copy_constant_to_storage, mat4x3_uniform)
{
   float_test(4, 3);
}

TEST_F(copy_constant_to_storage, mat4x4_uniform)
{
   float_test(4, 4);
}

TEST_F(copy_constant_to_storage, sampler_uniform)
{
   sampler_test();
}
