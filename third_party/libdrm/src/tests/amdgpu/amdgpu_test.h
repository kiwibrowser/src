/*
 * Copyright 2014 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
*/

#ifndef _AMDGPU_TEST_H_
#define _AMDGPU_TEST_H_

#include "amdgpu.h"
#include "amdgpu_drm.h"

/**
 * Define max. number of card in system which we are able to handle
 */
#define MAX_CARDS_SUPPORTED     4

/* Forward reference for array to keep "drm" handles */
extern int drm_amdgpu[MAX_CARDS_SUPPORTED];

/* Global variables */
extern int open_render_node;

/*************************  Basic test suite ********************************/

/*
 * Define basic test suite to serve as the starting point for future testing
*/

/**
 * Initialize basic test suite
 */
int suite_basic_tests_init();

/**
 * Deinitialize basic test suite
 */
int suite_basic_tests_clean();

/**
 * Tests in basic test suite
 */
extern CU_TestInfo basic_tests[];

/**
 * Initialize bo test suite
 */
int suite_bo_tests_init();

/**
 * Deinitialize bo test suite
 */
int suite_bo_tests_clean();

/**
 * Tests in bo test suite
 */
extern CU_TestInfo bo_tests[];

/**
 * Initialize cs test suite
 */
int suite_cs_tests_init();

/**
 * Deinitialize cs test suite
 */
int suite_cs_tests_clean();

/**
 * Tests in cs test suite
 */
extern CU_TestInfo cs_tests[];

/**
 * Initialize vce test suite
 */
int suite_vce_tests_init();

/**
 * Deinitialize vce test suite
 */
int suite_vce_tests_clean();

/**
 * Tests in vce test suite
 */
extern CU_TestInfo vce_tests[];

/**
+ * Initialize vcn test suite
+ */
int suite_vcn_tests_init();

/**
+ * Deinitialize vcn test suite
+ */
int suite_vcn_tests_clean();

/**
+ * Tests in vcn test suite
+ */
extern CU_TestInfo vcn_tests[];

/**
 * Initialize uvd enc test suite
 */
int suite_uvd_enc_tests_init();

/**
 * Deinitialize uvd enc test suite
 */
int suite_uvd_enc_tests_clean();

/**
 * Tests in uvd enc test suite
 */
extern CU_TestInfo uvd_enc_tests[];

/**
 * Initialize deadlock test suite
 */
int suite_deadlock_tests_init();

/**
 * Deinitialize deadlock test suite
 */
int suite_deadlock_tests_clean();

/**
 * Tests in uvd enc test suite
 */
extern CU_TestInfo deadlock_tests[];

/**
 * Helper functions
 */
static inline amdgpu_bo_handle gpu_mem_alloc(
					amdgpu_device_handle device_handle,
					uint64_t size,
					uint64_t alignment,
					uint32_t type,
					uint64_t flags,
					uint64_t *vmc_addr,
					amdgpu_va_handle *va_handle)
{
	struct amdgpu_bo_alloc_request req = {0};
	amdgpu_bo_handle buf_handle;
	int r;

	CU_ASSERT_NOT_EQUAL(vmc_addr, NULL);

	req.alloc_size = size;
	req.phys_alignment = alignment;
	req.preferred_heap = type;
	req.flags = flags;

	r = amdgpu_bo_alloc(device_handle, &req, &buf_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_va_range_alloc(device_handle,
				  amdgpu_gpu_va_range_general,
				  size, alignment, 0, vmc_addr,
				  va_handle, 0);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_va_op(buf_handle, 0, size, *vmc_addr, 0, AMDGPU_VA_OP_MAP);
	CU_ASSERT_EQUAL(r, 0);

	return buf_handle;
}

static inline int gpu_mem_free(amdgpu_bo_handle bo,
			       amdgpu_va_handle va_handle,
			       uint64_t vmc_addr,
			       uint64_t size)
{
	int r;

	r = amdgpu_bo_va_op(bo, 0, size, vmc_addr, 0, AMDGPU_VA_OP_UNMAP);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_va_range_free(va_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_free(bo);
	CU_ASSERT_EQUAL(r, 0);

	return 0;
}

static inline int
amdgpu_bo_alloc_and_map(amdgpu_device_handle dev, unsigned size,
			unsigned alignment, unsigned heap, uint64_t flags,
			amdgpu_bo_handle *bo, void **cpu, uint64_t *mc_address,
			amdgpu_va_handle *va_handle)
{
	struct amdgpu_bo_alloc_request request = {};
	amdgpu_bo_handle buf_handle;
	amdgpu_va_handle handle;
	uint64_t vmc_addr;
	int r;

	request.alloc_size = size;
	request.phys_alignment = alignment;
	request.preferred_heap = heap;
	request.flags = flags;

	r = amdgpu_bo_alloc(dev, &request, &buf_handle);
	if (r)
		return r;

	r = amdgpu_va_range_alloc(dev,
				  amdgpu_gpu_va_range_general,
				  size, alignment, 0, &vmc_addr,
				  &handle, 0);
	if (r)
		goto error_va_alloc;

	r = amdgpu_bo_va_op(buf_handle, 0, size, vmc_addr, 0, AMDGPU_VA_OP_MAP);
	if (r)
		goto error_va_map;

	r = amdgpu_bo_cpu_map(buf_handle, cpu);
	if (r)
		goto error_cpu_map;

	*bo = buf_handle;
	*mc_address = vmc_addr;
	*va_handle = handle;

	return 0;

error_cpu_map:
	amdgpu_bo_cpu_unmap(buf_handle);

error_va_map:
	amdgpu_bo_va_op(buf_handle, 0, size, vmc_addr, 0, AMDGPU_VA_OP_UNMAP);

error_va_alloc:
	amdgpu_bo_free(buf_handle);
	return r;
}

static inline int
amdgpu_bo_unmap_and_free(amdgpu_bo_handle bo, amdgpu_va_handle va_handle,
			 uint64_t mc_addr, uint64_t size)
{
	amdgpu_bo_cpu_unmap(bo);
	amdgpu_bo_va_op(bo, 0, size, mc_addr, 0, AMDGPU_VA_OP_UNMAP);
	amdgpu_va_range_free(va_handle);
	amdgpu_bo_free(bo);

	return 0;

}

static inline int
amdgpu_get_bo_list(amdgpu_device_handle dev, amdgpu_bo_handle bo1,
		   amdgpu_bo_handle bo2, amdgpu_bo_list_handle *list)
{
	amdgpu_bo_handle resources[] = {bo1, bo2};

	return amdgpu_bo_list_create(dev, bo2 ? 2 : 1, resources, NULL, list);
}

#endif  /* #ifdef _AMDGPU_TEST_H_ */
