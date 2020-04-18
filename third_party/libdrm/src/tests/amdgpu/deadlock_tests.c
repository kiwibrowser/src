/*
 * Copyright 2017 Advanced Micro Devices, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#include "CUnit/Basic.h"

#include "amdgpu_test.h"
#include "amdgpu_drm.h"

#include <pthread.h>


/*
 * This defines the delay in MS after which memory location designated for
 * compression against reference value is written to, unblocking command
 * processor
 */
#define WRITE_MEM_ADDRESS_DELAY_MS 100

#define	PACKET_TYPE3	3

#define PACKET3(op, n)	((PACKET_TYPE3 << 30) |				\
			 (((op) & 0xFF) << 8) |				\
			 ((n) & 0x3FFF) << 16)

#define	PACKET3_WAIT_REG_MEM				0x3C
#define		WAIT_REG_MEM_FUNCTION(x)                ((x) << 0)
		/* 0 - always
		 * 1 - <
		 * 2 - <=
		 * 3 - ==
		 * 4 - !=
		 * 5 - >=
		 * 6 - >
		 */
#define		WAIT_REG_MEM_MEM_SPACE(x)               ((x) << 4)
		/* 0 - reg
		 * 1 - mem
		 */
#define		WAIT_REG_MEM_OPERATION(x)               ((x) << 6)
		/* 0 - wait_reg_mem
		 * 1 - wr_wait_wr_reg
		 */
#define		WAIT_REG_MEM_ENGINE(x)                  ((x) << 8)
		/* 0 - me
		 * 1 - pfp
		 */

static  amdgpu_device_handle device_handle;
static  uint32_t  major_version;
static  uint32_t  minor_version;

static pthread_t stress_thread;
static uint32_t *ptr;

static void amdgpu_deadlock_helper(unsigned ip_type);
static void amdgpu_deadlock_gfx(void);
static void amdgpu_deadlock_compute(void);

int suite_deadlock_tests_init(void)
{
	struct amdgpu_gpu_info gpu_info = {0};
	int r;

	r = amdgpu_device_initialize(drm_amdgpu[0], &major_version,
				   &minor_version, &device_handle);

	if (r) {
		if ((r == -EACCES) && (errno == EACCES))
			printf("\n\nError:%s. "
				"Hint:Try to run this test program as root.",
				strerror(errno));
		return CUE_SINIT_FAILED;
	}

	return CUE_SUCCESS;
}

int suite_deadlock_tests_clean(void)
{
	int r = amdgpu_device_deinitialize(device_handle);

	if (r == 0)
		return CUE_SUCCESS;
	else
		return CUE_SCLEAN_FAILED;
}


CU_TestInfo deadlock_tests[] = {
	{ "gfx ring block test",  amdgpu_deadlock_gfx },

	/*
	* BUG: Compute ring stalls and never recovers when the address is
	* written after the command already submitted
	*/
	/* { "compute ring block test",  amdgpu_deadlock_compute }, */

	CU_TEST_INFO_NULL,
};

static void *write_mem_address(void *data)
{
	int i;

	/* useconds_t range is [0, 1,000,000] so use loop for waits > 1s */
	for (i = 0; i < WRITE_MEM_ADDRESS_DELAY_MS; i++)
		usleep(1000);

	ptr[256] = 0x1;

	return 0;
}

static void amdgpu_deadlock_gfx(void)
{
	amdgpu_deadlock_helper(AMDGPU_HW_IP_GFX);
}

static void amdgpu_deadlock_compute(void)
{
	amdgpu_deadlock_helper(AMDGPU_HW_IP_COMPUTE);
}

static void amdgpu_deadlock_helper(unsigned ip_type)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle;
	void *ib_result_cpu;
	uint64_t ib_result_mc_address;
	struct amdgpu_cs_request ibs_request;
	struct amdgpu_cs_ib_info ib_info;
	struct amdgpu_cs_fence fence_status;
	uint32_t expired;
	int i, r, instance;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle;

	r = pthread_create(&stress_thread, NULL, write_mem_address, NULL);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
			AMDGPU_GEM_DOMAIN_GTT, 0,
						    &ib_result_handle, &ib_result_cpu,
						    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_get_bo_list(device_handle, ib_result_handle, NULL,
			       &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	ptr = ib_result_cpu;

	ptr[0] = PACKET3(PACKET3_WAIT_REG_MEM, 5);
	ptr[1] = (WAIT_REG_MEM_MEM_SPACE(1) | /* memory */
			 WAIT_REG_MEM_FUNCTION(4) | /* != */
			 WAIT_REG_MEM_ENGINE(0));  /* me */
	ptr[2] = (ib_result_mc_address + 256*4) & 0xfffffffc;
	ptr[3] = ((ib_result_mc_address + 256*4) >> 32) & 0xffffffff;
	ptr[4] = 0x00000000; /* reference value */
	ptr[5] = 0xffffffff; /* and mask */
	ptr[6] = 0x00000004; /* poll interval */

	for (i = 7; i < 16; ++i)
		ptr[i] = 0xffff1000;


	ptr[256] = 0x0; /* the memory we wait on to change */



	memset(&ib_info, 0, sizeof(struct amdgpu_cs_ib_info));
	ib_info.ib_mc_address = ib_result_mc_address;
	ib_info.size = 16;

	memset(&ibs_request, 0, sizeof(struct amdgpu_cs_request));
	ibs_request.ip_type = ip_type;
	ibs_request.ring = 0;
	ibs_request.number_of_ibs = 1;
	ibs_request.ibs = &ib_info;
	ibs_request.resources = bo_list;
	ibs_request.fence_info.handle = NULL;

	for (i = 0; i < 200; i++) {
		r = amdgpu_cs_submit(context_handle, 0,&ibs_request, 1);
		CU_ASSERT_EQUAL(r, 0);

	}

	memset(&fence_status, 0, sizeof(struct amdgpu_cs_fence));
	fence_status.context = context_handle;
	fence_status.ip_type = ip_type;
	fence_status.ip_instance = 0;
	fence_status.ring = 0;
	fence_status.fence = ibs_request.seq_no;

	r = amdgpu_cs_query_fence_status(&fence_status,
			AMDGPU_TIMEOUT_INFINITE,0, &expired);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);

	pthread_join(stress_thread, NULL);
}
