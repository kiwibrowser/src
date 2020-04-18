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
#include "main/mtypes.h"
#include "main/macros.h"
#include "ralloc.h"
#include "uniform_initializer_utils.h"
#include <stdio.h>

void
fill_storage_array_with_sentinels(gl_constant_value *storage,
				  unsigned data_size,
				  unsigned red_zone_size)
{
   for (unsigned i = 0; i < data_size; i++)
      storage[i].u = 0xDEADBEEF;

   for (unsigned i = 0; i < red_zone_size; i++)
      storage[data_size + i].u = 0xBADDC0DE;
}

/**
 * Verfiy that markers past the end of the real uniform are unmodified
 */
static ::testing::AssertionResult
red_zone_is_intact(gl_constant_value *storage,
		   unsigned data_size,
		   unsigned red_zone_size)
{
   for (unsigned i = 0; i < red_zone_size; i++) {
      const unsigned idx = data_size + i;

      if (storage[idx].u != 0xBADDC0DE)
	 return ::testing::AssertionFailure()
	    << "storage[" << idx << "].u = "  << storage[idx].u
	    << ", exepected data values = " << data_size
	    << ", red-zone size = " << red_zone_size;
   }

   return ::testing::AssertionSuccess();
}

static const int values[] = {
   2, 0, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53
};

/**
 * Generate a single data element.
 *
 * This is by both \c generate_data and \c generate_array_data to create the
 * data.
 */
static void
generate_data_element(void *mem_ctx, const glsl_type *type,
		      ir_constant *&val, unsigned data_index_base)
{
   /* Set the initial data values for the generated constant.
    */
   ir_constant_data data;
   memset(&data, 0, sizeof(data));
   for (unsigned i = 0; i < type->components(); i++) {
      const unsigned idx = (i + data_index_base) % Elements(values);
      switch (type->base_type) {
      case GLSL_TYPE_UINT:
      case GLSL_TYPE_INT:
      case GLSL_TYPE_SAMPLER:
	 data.i[i] = values[idx];
	 break;
      case GLSL_TYPE_FLOAT:
	 data.f[i] = float(values[idx]);
	 break;
      case GLSL_TYPE_BOOL:
	 data.b[i] = bool(values[idx]);
	 break;
      case GLSL_TYPE_STRUCT:
      case GLSL_TYPE_ARRAY:
      case GLSL_TYPE_VOID:
      case GLSL_TYPE_ERROR:
	 ASSERT_TRUE(false);
	 break;
      }
   }

   /* Generate and verify the constant.
    */
   val = new(mem_ctx) ir_constant(type, &data);

   for (unsigned i = 0; i < type->components(); i++) {
      switch (type->base_type) {
      case GLSL_TYPE_UINT:
      case GLSL_TYPE_INT:
      case GLSL_TYPE_SAMPLER:
	 ASSERT_EQ(data.i[i], val->value.i[i]);
	 break;
      case GLSL_TYPE_FLOAT:
	 ASSERT_EQ(data.f[i], val->value.f[i]);
	 break;
      case GLSL_TYPE_BOOL:
	 ASSERT_EQ(data.b[i], val->value.b[i]);
	 break;
      case GLSL_TYPE_STRUCT:
      case GLSL_TYPE_ARRAY:
      case GLSL_TYPE_VOID:
      case GLSL_TYPE_ERROR:
	 ASSERT_TRUE(false);
	 break;
      }
   }
}

void
generate_data(void *mem_ctx, enum glsl_base_type base_type,
	      unsigned columns, unsigned rows,
	      ir_constant *&val)
{
   /* Determine what the type of the generated constant should be.
    */
   const glsl_type *const type =
      glsl_type::get_instance(base_type, rows, columns);
   ASSERT_FALSE(type->is_error());

   generate_data_element(mem_ctx, type, val, 0);
}

void
generate_array_data(void *mem_ctx, enum glsl_base_type base_type,
		    unsigned columns, unsigned rows, unsigned array_size,
		    ir_constant *&val)
{
   /* Determine what the type of the generated constant should be.
    */
   const glsl_type *const element_type =
      glsl_type::get_instance(base_type, rows, columns);
   ASSERT_FALSE(element_type->is_error());

   const glsl_type *const array_type =
      glsl_type::get_array_instance(element_type, array_size);
   ASSERT_FALSE(array_type->is_error());

   /* Set the initial data values for the generated constant.
    */
   exec_list values_for_array;
   for (unsigned i = 0; i < array_size; i++) {
      ir_constant *element;

      generate_data_element(mem_ctx, element_type, element, i);
      values_for_array.push_tail(element);
   }

   val = new(mem_ctx) ir_constant(array_type, &values_for_array);
}

/**
 * Verify that the data stored for the uniform matches the initializer
 *
 * \param storage              Backing storage for the uniform
 * \param storage_array_size  Array size of the backing storage.  This must be
 *                            less than or equal to the array size of the type
 *                            of \c val.  If \c val is not an array, this must
 *                            be zero.
 * \param val                 Value of the initializer for the unifrom.
 * \param red_zone
 */
void
verify_data(gl_constant_value *storage, unsigned storage_array_size,
	    ir_constant *val, unsigned red_zone_size)
{
   if (val->type->base_type == GLSL_TYPE_ARRAY) {
      const glsl_type *const element_type = val->array_elements[0]->type;

      for (unsigned i = 0; i < storage_array_size; i++) {
	 verify_data(storage + (i * element_type->components()), 0,
		     val->array_elements[i], 0);
      }

      const unsigned components = element_type->components();

      if (red_zone_size > 0) {
	 EXPECT_TRUE(red_zone_is_intact(storage,
					storage_array_size * components,
					red_zone_size));
      }
   } else {
      ASSERT_EQ(0u, storage_array_size);
      for (unsigned i = 0; i < val->type->components(); i++) {
	 switch (val->type->base_type) {
	 case GLSL_TYPE_UINT:
	 case GLSL_TYPE_INT:
	 case GLSL_TYPE_SAMPLER:
	    EXPECT_EQ(val->value.i[i], storage[i].i);
	    break;
	 case GLSL_TYPE_FLOAT:
	    EXPECT_EQ(val->value.f[i], storage[i].f);
	    break;
	 case GLSL_TYPE_BOOL:
	    EXPECT_EQ(int(val->value.b[i]), storage[i].i);
	    break;
	 case GLSL_TYPE_STRUCT:
	 case GLSL_TYPE_ARRAY:
	 case GLSL_TYPE_VOID:
	 case GLSL_TYPE_ERROR:
	    ASSERT_TRUE(false);
	    break;
	 }
      }

      if (red_zone_size > 0) {
	 EXPECT_TRUE(red_zone_is_intact(storage,
					val->type->components(),
					red_zone_size));
      }
   }
}
