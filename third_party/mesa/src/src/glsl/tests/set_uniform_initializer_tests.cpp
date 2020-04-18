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
set_uniform_initializer(void *mem_ctx, gl_shader_program *prog,
			const char *name, const glsl_type *type,
			ir_constant *val);
}

class set_uniform_initializer : public ::testing::Test {
public:
   virtual void SetUp();
   virtual void TearDown();

   /**
    * Index of the uniform to be tested.
    *
    * All of the \c set_uniform_initializer tests create several slots for
    * unifroms.  All but one of the slots is fake.  This field holds the index
    * of the slot for the uniform being tested.
    */
   unsigned actual_index;

   /**
    * Name of the uniform to be tested.
    */
   const char *name;

   /**
    * Shader program used in the test.
    */
   struct gl_shader_program *prog;

   /**
    * Ralloc memory context used for all temporary allocations.
    */
   void *mem_ctx;
};

void
set_uniform_initializer::SetUp()
{
   this->mem_ctx = ralloc_context(NULL);
   this->prog = rzalloc(NULL, struct gl_shader_program);

   /* Set default values used by the test cases.
    */
   this->actual_index = 1;
   this->name = "i";
}

void
set_uniform_initializer::TearDown()
{
   ralloc_free(this->mem_ctx);
   this->mem_ctx = NULL;

   ralloc_free(this->prog);
   this->prog = NULL;
}

/**
 * Create some uniform storage for a program.
 *
 * \param prog          Program to get some storage
 * \param num_storage   Total number of storage slots
 * \param index_to_set  Storage slot that will actually get a value
 * \param name          Name for the actual storage slot
 * \param type          Type for the elements of the actual storage slot
 * \param array_size    Size for the array of the actual storage slot.  This
 *                      should be zero for non-arrays.
 */
static unsigned
establish_uniform_storage(struct gl_shader_program *prog, unsigned num_storage,
			  unsigned index_to_set, const char *name,
			  const glsl_type *type, unsigned array_size)
{
   const unsigned elements = MAX2(1, array_size);
   const unsigned data_components = elements * type->components();
   const unsigned total_components = MAX2(17, (data_components
					       + type->components()));
   const unsigned red_zone_components = total_components - data_components;

   prog->UniformStorage = rzalloc_array(prog, struct gl_uniform_storage,
					num_storage);
   prog->NumUserUniformStorage = num_storage;

   prog->UniformStorage[index_to_set].name = (char *) name;
   prog->UniformStorage[index_to_set].type = type;
   prog->UniformStorage[index_to_set].array_elements = array_size;
   prog->UniformStorage[index_to_set].initialized = false;
   prog->UniformStorage[index_to_set].sampler = ~0;
   prog->UniformStorage[index_to_set].num_driver_storage = 0;
   prog->UniformStorage[index_to_set].driver_storage = NULL;
   prog->UniformStorage[index_to_set].storage =
      rzalloc_array(prog, union gl_constant_value, total_components);

   fill_storage_array_with_sentinels(prog->UniformStorage[index_to_set].storage,
				     data_components,
				     red_zone_components);

   for (unsigned i = 0; i < num_storage; i++) {
      if (i == index_to_set)
	 continue;

      prog->UniformStorage[i].name = (char *) "invalid slot";
      prog->UniformStorage[i].type = glsl_type::void_type;
      prog->UniformStorage[i].array_elements = 0;
      prog->UniformStorage[i].initialized = false;
      prog->UniformStorage[i].sampler = ~0;
      prog->UniformStorage[i].num_driver_storage = 0;
      prog->UniformStorage[i].driver_storage = NULL;
      prog->UniformStorage[i].storage = NULL;
   }

   return red_zone_components;
}

/**
 * Verify that the correct uniform is marked as having been initialized.
 */
static void
verify_initialization(struct gl_shader_program *prog, unsigned actual_index)
{
   for (unsigned i = 0; i < prog->NumUserUniformStorage; i++) {
      if (i == actual_index) {
	 EXPECT_TRUE(prog->UniformStorage[actual_index].initialized);
      } else {
	 EXPECT_FALSE(prog->UniformStorage[i].initialized);
      }
   }
}

static void
non_array_test(void *mem_ctx, struct gl_shader_program *prog,
	       unsigned actual_index, const char *name,
	       enum glsl_base_type base_type,
	       unsigned columns, unsigned rows)
{
   const glsl_type *const type =
      glsl_type::get_instance(base_type, rows, columns);

   unsigned red_zone_components =
      establish_uniform_storage(prog, 3, actual_index, name, type, 0);

   ir_constant *val;
   generate_data(mem_ctx, base_type, columns, rows, val);

   linker::set_uniform_initializer(mem_ctx, prog, name, type, val);

   verify_initialization(prog, actual_index);
   verify_data(prog->UniformStorage[actual_index].storage, 0, val,
	       red_zone_components);
}

TEST_F(set_uniform_initializer, int_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 1);
}

TEST_F(set_uniform_initializer, ivec2_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 2);
}

TEST_F(set_uniform_initializer, ivec3_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 3);
}

TEST_F(set_uniform_initializer, ivec4_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 4);
}

TEST_F(set_uniform_initializer, uint_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 1);
}

TEST_F(set_uniform_initializer, uvec2_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 2);
}

TEST_F(set_uniform_initializer, uvec3_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 3);
}

TEST_F(set_uniform_initializer, uvec4_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 4);
}

TEST_F(set_uniform_initializer, bool_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 1);
}

TEST_F(set_uniform_initializer, bvec2_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 2);
}

TEST_F(set_uniform_initializer, bvec3_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 3);
}

TEST_F(set_uniform_initializer, bvec4_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 4);
}

TEST_F(set_uniform_initializer, float_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 2);
}

TEST_F(set_uniform_initializer, vec2_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 2);
}

TEST_F(set_uniform_initializer, vec3_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 3);
}

TEST_F(set_uniform_initializer, vec4_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 4);
}

TEST_F(set_uniform_initializer, mat2x2_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 2);
}

TEST_F(set_uniform_initializer, mat2x3_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 3);
}

TEST_F(set_uniform_initializer, mat2x4_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 4);
}

TEST_F(set_uniform_initializer, mat3x2_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 2);
}

TEST_F(set_uniform_initializer, mat3x3_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 3);
}

TEST_F(set_uniform_initializer, mat3x4_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 4);
}

TEST_F(set_uniform_initializer, mat4x2_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 2);
}

TEST_F(set_uniform_initializer, mat4x3_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 3);
}

TEST_F(set_uniform_initializer, mat4x4_uniform)
{
   non_array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 4);
}

static void
array_test(void *mem_ctx, struct gl_shader_program *prog,
	   unsigned actual_index, const char *name,
	   enum glsl_base_type base_type,
	   unsigned columns, unsigned rows, unsigned array_size,
	   unsigned excess_data_size)
{
   const glsl_type *const element_type =
      glsl_type::get_instance(base_type, rows, columns);

   const unsigned red_zone_components =
      establish_uniform_storage(prog, 3, actual_index, name, element_type,
				array_size);

   /* The constant value generated may have more array elements than the
    * uniform that it initializes.  In the real compiler and linker this can
    * happen when a uniform array is compacted because some of the tail
    * elements are not used.  In this case, the type of the uniform will be
    * modified, but the initializer will not.
    */
   ir_constant *val;
   generate_array_data(mem_ctx, base_type, columns, rows,
		       array_size + excess_data_size, val);

   linker::set_uniform_initializer(mem_ctx, prog, name, element_type, val);

   verify_initialization(prog, actual_index);
   verify_data(prog->UniformStorage[actual_index].storage, array_size,
	       val, red_zone_components);
}

TEST_F(set_uniform_initializer, int_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 1, 4, 0);
}

TEST_F(set_uniform_initializer, ivec2_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 2, 4, 0);
}

TEST_F(set_uniform_initializer, ivec3_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 3, 4, 0);
}

TEST_F(set_uniform_initializer, ivec4_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 4, 4, 0);
}

TEST_F(set_uniform_initializer, uint_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 1, 4, 0);
}

TEST_F(set_uniform_initializer, uvec2_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 2, 4, 0);
}

TEST_F(set_uniform_initializer, uvec3_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 3, 4, 0);
}

TEST_F(set_uniform_initializer, uvec4_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 4, 4, 0);
}

TEST_F(set_uniform_initializer, bool_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 1, 4, 0);
}

TEST_F(set_uniform_initializer, bvec2_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 2, 4, 0);
}

TEST_F(set_uniform_initializer, bvec3_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 3, 4, 0);
}

TEST_F(set_uniform_initializer, bvec4_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 4, 4, 0);
}

TEST_F(set_uniform_initializer, float_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 1, 4, 0);
}

TEST_F(set_uniform_initializer, vec2_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 2, 4, 0);
}

TEST_F(set_uniform_initializer, vec3_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 3, 4, 0);
}

TEST_F(set_uniform_initializer, vec4_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 4, 4, 0);
}

TEST_F(set_uniform_initializer, mat2x2_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 2, 4, 0);
}

TEST_F(set_uniform_initializer, mat2x3_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 3, 4, 0);
}

TEST_F(set_uniform_initializer, mat2x4_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 4, 4, 0);
}

TEST_F(set_uniform_initializer, mat3x2_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 2, 4, 0);
}

TEST_F(set_uniform_initializer, mat3x3_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 3, 4, 0);
}

TEST_F(set_uniform_initializer, mat3x4_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 4, 4, 0);
}

TEST_F(set_uniform_initializer, mat4x2_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 2, 4, 0);
}

TEST_F(set_uniform_initializer, mat4x3_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 3, 4, 0);
}

TEST_F(set_uniform_initializer, mat4x4_array_uniform)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 4, 4, 0);
}

TEST_F(set_uniform_initializer, int_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 1, 4, 5);
}

TEST_F(set_uniform_initializer, ivec2_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 2, 4, 5);
}

TEST_F(set_uniform_initializer, ivec3_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 3, 4, 5);
}

TEST_F(set_uniform_initializer, ivec4_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_INT, 1, 4, 4, 5);
}

TEST_F(set_uniform_initializer, uint_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 1, 4, 5);
}

TEST_F(set_uniform_initializer, uvec2_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 2, 4, 5);
}

TEST_F(set_uniform_initializer, uvec3_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 3, 4, 5);
}

TEST_F(set_uniform_initializer, uvec4_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_UINT, 1, 4, 4, 5);
}

TEST_F(set_uniform_initializer, bool_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 1, 4, 5);
}

TEST_F(set_uniform_initializer, bvec2_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 2, 4, 5);
}

TEST_F(set_uniform_initializer, bvec3_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 3, 4, 5);
}

TEST_F(set_uniform_initializer, bvec4_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_BOOL, 1, 4, 4, 5);
}

TEST_F(set_uniform_initializer, float_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 1, 4, 5);
}

TEST_F(set_uniform_initializer, vec2_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 2, 4, 5);
}

TEST_F(set_uniform_initializer, vec3_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 3, 4, 5);
}

TEST_F(set_uniform_initializer, vec4_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 1, 4, 4, 5);
}

TEST_F(set_uniform_initializer, mat2x2_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 2, 4, 5);
}

TEST_F(set_uniform_initializer, mat2x3_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 3, 4, 5);
}

TEST_F(set_uniform_initializer, mat2x4_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 2, 4, 4, 5);
}

TEST_F(set_uniform_initializer, mat3x2_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 2, 4, 5);
}

TEST_F(set_uniform_initializer, mat3x3_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 3, 4, 5);
}

TEST_F(set_uniform_initializer, mat3x4_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 3, 4, 4, 5);
}

TEST_F(set_uniform_initializer, mat4x2_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 2, 4, 5);
}

TEST_F(set_uniform_initializer, mat4x3_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 3, 4, 5);
}

TEST_F(set_uniform_initializer, mat4x4_array_uniform_excess_initializer)
{
   array_test(mem_ctx, prog, actual_index, name, GLSL_TYPE_FLOAT, 4, 4, 4, 5);
}
