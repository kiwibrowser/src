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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif
#include <sys/wait.h>

#include "CUnit/Basic.h"

#include "amdgpu_test.h"
#include "amdgpu_drm.h"

static  amdgpu_device_handle device_handle;
static  uint32_t  major_version;
static  uint32_t  minor_version;
static  uint32_t  family_id;

static void amdgpu_query_info_test(void);
static void amdgpu_memory_alloc(void);
static void amdgpu_command_submission_gfx(void);
static void amdgpu_command_submission_compute(void);
static void amdgpu_command_submission_multi_fence(void);
static void amdgpu_command_submission_sdma(void);
static void amdgpu_userptr_test(void);
static void amdgpu_semaphore_test(void);

static void amdgpu_command_submission_write_linear_helper(unsigned ip_type);
static void amdgpu_command_submission_const_fill_helper(unsigned ip_type);
static void amdgpu_command_submission_copy_linear_helper(unsigned ip_type);

CU_TestInfo basic_tests[] = {
	{ "Query Info Test",  amdgpu_query_info_test },
	{ "Memory alloc Test",  amdgpu_memory_alloc },
	{ "Userptr Test",  amdgpu_userptr_test },
	{ "Command submission Test (GFX)",  amdgpu_command_submission_gfx },
	{ "Command submission Test (Compute)", amdgpu_command_submission_compute },
	{ "Command submission Test (Multi-Fence)", amdgpu_command_submission_multi_fence },
	{ "Command submission Test (SDMA)", amdgpu_command_submission_sdma },
	{ "SW semaphore Test",  amdgpu_semaphore_test },
	CU_TEST_INFO_NULL,
};
#define BUFFER_SIZE (8 * 1024)
#define SDMA_PKT_HEADER_op_offset 0
#define SDMA_PKT_HEADER_op_mask   0x000000FF
#define SDMA_PKT_HEADER_op_shift  0
#define SDMA_PKT_HEADER_OP(x) (((x) & SDMA_PKT_HEADER_op_mask) << SDMA_PKT_HEADER_op_shift)
#define SDMA_OPCODE_CONSTANT_FILL  11
#       define SDMA_CONSTANT_FILL_EXTRA_SIZE(x)           ((x) << 14)
	/* 0 = byte fill
	 * 2 = DW fill
	 */
#define SDMA_PACKET(op, sub_op, e)	((((e) & 0xFFFF) << 16) |	\
					(((sub_op) & 0xFF) << 8) |	\
					(((op) & 0xFF) << 0))
#define	SDMA_OPCODE_WRITE				  2
#       define SDMA_WRITE_SUB_OPCODE_LINEAR               0
#       define SDMA_WRTIE_SUB_OPCODE_TILED                1

#define	SDMA_OPCODE_COPY				  1
#       define SDMA_COPY_SUB_OPCODE_LINEAR                0

#define GFX_COMPUTE_NOP  0xffff1000
#define SDMA_NOP  0x0

/* PM4 */
#define	PACKET_TYPE0	0
#define	PACKET_TYPE1	1
#define	PACKET_TYPE2	2
#define	PACKET_TYPE3	3

#define CP_PACKET_GET_TYPE(h) (((h) >> 30) & 3)
#define CP_PACKET_GET_COUNT(h) (((h) >> 16) & 0x3FFF)
#define CP_PACKET0_GET_REG(h) ((h) & 0xFFFF)
#define CP_PACKET3_GET_OPCODE(h) (((h) >> 8) & 0xFF)
#define PACKET0(reg, n)	((PACKET_TYPE0 << 30) |				\
			 ((reg) & 0xFFFF) |			\
			 ((n) & 0x3FFF) << 16)
#define CP_PACKET2			0x80000000
#define		PACKET2_PAD_SHIFT		0
#define		PACKET2_PAD_MASK		(0x3fffffff << 0)

#define PACKET2(v)	(CP_PACKET2 | REG_SET(PACKET2_PAD, (v)))

#define PACKET3(op, n)	((PACKET_TYPE3 << 30) |				\
			 (((op) & 0xFF) << 8) |				\
			 ((n) & 0x3FFF) << 16)

/* Packet 3 types */
#define	PACKET3_NOP					0x10

#define	PACKET3_WRITE_DATA				0x37
#define		WRITE_DATA_DST_SEL(x)                   ((x) << 8)
		/* 0 - register
		 * 1 - memory (sync - via GRBM)
		 * 2 - gl2
		 * 3 - gds
		 * 4 - reserved
		 * 5 - memory (async - direct)
		 */
#define		WR_ONE_ADDR                             (1 << 16)
#define		WR_CONFIRM                              (1 << 20)
#define		WRITE_DATA_CACHE_POLICY(x)              ((x) << 25)
		/* 0 - LRU
		 * 1 - Stream
		 */
#define		WRITE_DATA_ENGINE_SEL(x)                ((x) << 30)
		/* 0 - me
		 * 1 - pfp
		 * 2 - ce
		 */

#define	PACKET3_DMA_DATA				0x50
/* 1. header
 * 2. CONTROL
 * 3. SRC_ADDR_LO or DATA [31:0]
 * 4. SRC_ADDR_HI [31:0]
 * 5. DST_ADDR_LO [31:0]
 * 6. DST_ADDR_HI [7:0]
 * 7. COMMAND [30:21] | BYTE_COUNT [20:0]
 */
/* CONTROL */
#              define PACKET3_DMA_DATA_ENGINE(x)     ((x) << 0)
		/* 0 - ME
		 * 1 - PFP
		 */
#              define PACKET3_DMA_DATA_SRC_CACHE_POLICY(x) ((x) << 13)
		/* 0 - LRU
		 * 1 - Stream
		 * 2 - Bypass
		 */
#              define PACKET3_DMA_DATA_SRC_VOLATILE (1 << 15)
#              define PACKET3_DMA_DATA_DST_SEL(x)  ((x) << 20)
		/* 0 - DST_ADDR using DAS
		 * 1 - GDS
		 * 3 - DST_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_DST_CACHE_POLICY(x) ((x) << 25)
		/* 0 - LRU
		 * 1 - Stream
		 * 2 - Bypass
		 */
#              define PACKET3_DMA_DATA_DST_VOLATILE (1 << 27)
#              define PACKET3_DMA_DATA_SRC_SEL(x)  ((x) << 29)
		/* 0 - SRC_ADDR using SAS
		 * 1 - GDS
		 * 2 - DATA
		 * 3 - SRC_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_CP_SYNC     (1 << 31)
/* COMMAND */
#              define PACKET3_DMA_DATA_DIS_WC      (1 << 21)
#              define PACKET3_DMA_DATA_CMD_SRC_SWAP(x) ((x) << 22)
		/* 0 - none
		 * 1 - 8 in 16
		 * 2 - 8 in 32
		 * 3 - 8 in 64
		 */
#              define PACKET3_DMA_DATA_CMD_DST_SWAP(x) ((x) << 24)
		/* 0 - none
		 * 1 - 8 in 16
		 * 2 - 8 in 32
		 * 3 - 8 in 64
		 */
#              define PACKET3_DMA_DATA_CMD_SAS     (1 << 26)
		/* 0 - memory
		 * 1 - register
		 */
#              define PACKET3_DMA_DATA_CMD_DAS     (1 << 27)
		/* 0 - memory
		 * 1 - register
		 */
#              define PACKET3_DMA_DATA_CMD_SAIC    (1 << 28)
#              define PACKET3_DMA_DATA_CMD_DAIC    (1 << 29)
#              define PACKET3_DMA_DATA_CMD_RAW_WAIT  (1 << 30)

#define SDMA_PACKET_SI(op, b, t, s, cnt)	((((op) & 0xF) << 28) |	\
						(((b) & 0x1) << 26) |		\
						(((t) & 0x1) << 23) |		\
						(((s) & 0x1) << 22) |		\
						(((cnt) & 0xFFFFF) << 0))
#define	SDMA_OPCODE_COPY_SI	3
#define SDMA_OPCODE_CONSTANT_FILL_SI	13
#define SDMA_NOP_SI  0xf
#define GFX_COMPUTE_NOP_SI 0x80000000
#define	PACKET3_DMA_DATA_SI	0x41
#              define PACKET3_DMA_DATA_SI_ENGINE(x)     ((x) << 27)
		/* 0 - ME
		 * 1 - PFP
		 */
#              define PACKET3_DMA_DATA_SI_DST_SEL(x)  ((x) << 20)
		/* 0 - DST_ADDR using DAS
		 * 1 - GDS
		 * 3 - DST_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_SI_SRC_SEL(x)  ((x) << 29)
		/* 0 - SRC_ADDR using SAS
		 * 1 - GDS
		 * 2 - DATA
		 * 3 - SRC_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_SI_CP_SYNC     (1 << 31)

int suite_basic_tests_init(void)
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

	r = amdgpu_query_gpu_info(device_handle, &gpu_info);
	if (r)
		return CUE_SINIT_FAILED;

	family_id = gpu_info.family_id;

	return CUE_SUCCESS;
}

int suite_basic_tests_clean(void)
{
	int r = amdgpu_device_deinitialize(device_handle);

	if (r == 0)
		return CUE_SUCCESS;
	else
		return CUE_SCLEAN_FAILED;
}

static void amdgpu_query_info_test(void)
{
	struct amdgpu_gpu_info gpu_info = {0};
	uint32_t version, feature;
	int r;

	r = amdgpu_query_gpu_info(device_handle, &gpu_info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_query_firmware_version(device_handle, AMDGPU_INFO_FW_VCE, 0,
					  0, &version, &feature);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_memory_alloc(void)
{
	amdgpu_bo_handle bo;
	amdgpu_va_handle va_handle;
	uint64_t bo_mc;
	int r;

	/* Test visible VRAM */
	bo = gpu_mem_alloc(device_handle,
			4096, 4096,
			AMDGPU_GEM_DOMAIN_VRAM,
			AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED,
			&bo_mc, &va_handle);

	r = gpu_mem_free(bo, va_handle, bo_mc, 4096);
	CU_ASSERT_EQUAL(r, 0);

	/* Test invisible VRAM */
	bo = gpu_mem_alloc(device_handle,
			4096, 4096,
			AMDGPU_GEM_DOMAIN_VRAM,
			AMDGPU_GEM_CREATE_NO_CPU_ACCESS,
			&bo_mc, &va_handle);

	r = gpu_mem_free(bo, va_handle, bo_mc, 4096);
	CU_ASSERT_EQUAL(r, 0);

	/* Test GART Cacheable */
	bo = gpu_mem_alloc(device_handle,
			4096, 4096,
			AMDGPU_GEM_DOMAIN_GTT,
			0, &bo_mc, &va_handle);

	r = gpu_mem_free(bo, va_handle, bo_mc, 4096);
	CU_ASSERT_EQUAL(r, 0);

	/* Test GART USWC */
	bo = gpu_mem_alloc(device_handle,
			4096, 4096,
			AMDGPU_GEM_DOMAIN_GTT,
			AMDGPU_GEM_CREATE_CPU_GTT_USWC,
			&bo_mc, &va_handle);

	r = gpu_mem_free(bo, va_handle, bo_mc, 4096);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_gfx_separate_ibs(void)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle, ib_result_ce_handle;
	void *ib_result_cpu, *ib_result_ce_cpu;
	uint64_t ib_result_mc_address, ib_result_ce_mc_address;
	struct amdgpu_cs_request ibs_request = {0};
	struct amdgpu_cs_ib_info ib_info[2];
	struct amdgpu_cs_fence fence_status = {0};
	uint32_t *ptr;
	uint32_t expired;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle, va_handle_ce;
	int r, i = 0;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_handle, &ib_result_cpu,
				    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_ce_handle, &ib_result_ce_cpu,
				    &ib_result_ce_mc_address, &va_handle_ce);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_get_bo_list(device_handle, ib_result_handle,
			       ib_result_ce_handle, &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	memset(ib_info, 0, 2 * sizeof(struct amdgpu_cs_ib_info));

	/* IT_SET_CE_DE_COUNTERS */
	ptr = ib_result_ce_cpu;
	if (family_id != AMDGPU_FAMILY_SI) {
		ptr[i++] = 0xc0008900;
		ptr[i++] = 0;
	}
	ptr[i++] = 0xc0008400;
	ptr[i++] = 1;
	ib_info[0].ib_mc_address = ib_result_ce_mc_address;
	ib_info[0].size = i;
	ib_info[0].flags = AMDGPU_IB_FLAG_CE;

	/* IT_WAIT_ON_CE_COUNTER */
	ptr = ib_result_cpu;
	ptr[0] = 0xc0008600;
	ptr[1] = 0x00000001;
	ib_info[1].ib_mc_address = ib_result_mc_address;
	ib_info[1].size = 2;

	ibs_request.ip_type = AMDGPU_HW_IP_GFX;
	ibs_request.number_of_ibs = 2;
	ibs_request.ibs = ib_info;
	ibs_request.resources = bo_list;
	ibs_request.fence_info.handle = NULL;

	r = amdgpu_cs_submit(context_handle, 0,&ibs_request, 1);

	CU_ASSERT_EQUAL(r, 0);

	fence_status.context = context_handle;
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.fence = ibs_request.seq_no;

	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_ce_handle, va_handle_ce,
				     ib_result_ce_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);

}

static void amdgpu_command_submission_gfx_shared_ib(void)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle;
	void *ib_result_cpu;
	uint64_t ib_result_mc_address;
	struct amdgpu_cs_request ibs_request = {0};
	struct amdgpu_cs_ib_info ib_info[2];
	struct amdgpu_cs_fence fence_status = {0};
	uint32_t *ptr;
	uint32_t expired;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle;
	int r, i = 0;

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

	memset(ib_info, 0, 2 * sizeof(struct amdgpu_cs_ib_info));

	/* IT_SET_CE_DE_COUNTERS */
	ptr = ib_result_cpu;
	if (family_id != AMDGPU_FAMILY_SI) {
		ptr[i++] = 0xc0008900;
		ptr[i++] = 0;
	}
	ptr[i++] = 0xc0008400;
	ptr[i++] = 1;
	ib_info[0].ib_mc_address = ib_result_mc_address;
	ib_info[0].size = i;
	ib_info[0].flags = AMDGPU_IB_FLAG_CE;

	ptr = (uint32_t *)ib_result_cpu + 4;
	ptr[0] = 0xc0008600;
	ptr[1] = 0x00000001;
	ib_info[1].ib_mc_address = ib_result_mc_address + 16;
	ib_info[1].size = 2;

	ibs_request.ip_type = AMDGPU_HW_IP_GFX;
	ibs_request.number_of_ibs = 2;
	ibs_request.ibs = ib_info;
	ibs_request.resources = bo_list;
	ibs_request.fence_info.handle = NULL;

	r = amdgpu_cs_submit(context_handle, 0, &ibs_request, 1);

	CU_ASSERT_EQUAL(r, 0);

	fence_status.context = context_handle;
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.fence = ibs_request.seq_no;

	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_gfx_cp_write_data(void)
{
	amdgpu_command_submission_write_linear_helper(AMDGPU_HW_IP_GFX);
}

static void amdgpu_command_submission_gfx_cp_const_fill(void)
{
	amdgpu_command_submission_const_fill_helper(AMDGPU_HW_IP_GFX);
}

static void amdgpu_command_submission_gfx_cp_copy_data(void)
{
	amdgpu_command_submission_copy_linear_helper(AMDGPU_HW_IP_GFX);
}

static void amdgpu_command_submission_gfx(void)
{
	/* write data using the CP */
	amdgpu_command_submission_gfx_cp_write_data();
	/* const fill using the CP */
	amdgpu_command_submission_gfx_cp_const_fill();
	/* copy data using the CP */
	amdgpu_command_submission_gfx_cp_copy_data();
	/* separate IB buffers for multi-IB submission */
	amdgpu_command_submission_gfx_separate_ibs();
	/* shared IB buffer for multi-IB submission */
	amdgpu_command_submission_gfx_shared_ib();
}

static void amdgpu_semaphore_test(void)
{
	amdgpu_context_handle context_handle[2];
	amdgpu_semaphore_handle sem;
	amdgpu_bo_handle ib_result_handle[2];
	void *ib_result_cpu[2];
	uint64_t ib_result_mc_address[2];
	struct amdgpu_cs_request ibs_request[2] = {0};
	struct amdgpu_cs_ib_info ib_info[2] = {0};
	struct amdgpu_cs_fence fence_status = {0};
	uint32_t *ptr;
	uint32_t expired;
	uint32_t sdma_nop, gfx_nop;
	amdgpu_bo_list_handle bo_list[2];
	amdgpu_va_handle va_handle[2];
	int r, i;

	if (family_id == AMDGPU_FAMILY_SI) {
		sdma_nop = SDMA_PACKET_SI(SDMA_NOP_SI, 0, 0, 0, 0);
		gfx_nop = GFX_COMPUTE_NOP_SI;
	} else {
		sdma_nop = SDMA_PKT_HEADER_OP(SDMA_NOP);
		gfx_nop = GFX_COMPUTE_NOP;
	}

	r = amdgpu_cs_create_semaphore(&sem);
	CU_ASSERT_EQUAL(r, 0);
	for (i = 0; i < 2; i++) {
		r = amdgpu_cs_ctx_create(device_handle, &context_handle[i]);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
					    AMDGPU_GEM_DOMAIN_GTT, 0,
					    &ib_result_handle[i], &ib_result_cpu[i],
					    &ib_result_mc_address[i], &va_handle[i]);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_get_bo_list(device_handle, ib_result_handle[i],
				       NULL, &bo_list[i]);
		CU_ASSERT_EQUAL(r, 0);
	}

	/* 1. same context different engine */
	ptr = ib_result_cpu[0];
	ptr[0] = sdma_nop;
	ib_info[0].ib_mc_address = ib_result_mc_address[0];
	ib_info[0].size = 1;

	ibs_request[0].ip_type = AMDGPU_HW_IP_DMA;
	ibs_request[0].number_of_ibs = 1;
	ibs_request[0].ibs = &ib_info[0];
	ibs_request[0].resources = bo_list[0];
	ibs_request[0].fence_info.handle = NULL;
	r = amdgpu_cs_submit(context_handle[0], 0,&ibs_request[0], 1);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_cs_signal_semaphore(context_handle[0], AMDGPU_HW_IP_DMA, 0, 0, sem);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_wait_semaphore(context_handle[0], AMDGPU_HW_IP_GFX, 0, 0, sem);
	CU_ASSERT_EQUAL(r, 0);
	ptr = ib_result_cpu[1];
	ptr[0] = gfx_nop;
	ib_info[1].ib_mc_address = ib_result_mc_address[1];
	ib_info[1].size = 1;

	ibs_request[1].ip_type = AMDGPU_HW_IP_GFX;
	ibs_request[1].number_of_ibs = 1;
	ibs_request[1].ibs = &ib_info[1];
	ibs_request[1].resources = bo_list[1];
	ibs_request[1].fence_info.handle = NULL;

	r = amdgpu_cs_submit(context_handle[0], 0,&ibs_request[1], 1);
	CU_ASSERT_EQUAL(r, 0);

	fence_status.context = context_handle[0];
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.fence = ibs_request[1].seq_no;
	r = amdgpu_cs_query_fence_status(&fence_status,
					 500000000, 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	/* 2. same engine different context */
	ptr = ib_result_cpu[0];
	ptr[0] = gfx_nop;
	ib_info[0].ib_mc_address = ib_result_mc_address[0];
	ib_info[0].size = 1;

	ibs_request[0].ip_type = AMDGPU_HW_IP_GFX;
	ibs_request[0].number_of_ibs = 1;
	ibs_request[0].ibs = &ib_info[0];
	ibs_request[0].resources = bo_list[0];
	ibs_request[0].fence_info.handle = NULL;
	r = amdgpu_cs_submit(context_handle[0], 0,&ibs_request[0], 1);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_cs_signal_semaphore(context_handle[0], AMDGPU_HW_IP_GFX, 0, 0, sem);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_wait_semaphore(context_handle[1], AMDGPU_HW_IP_GFX, 0, 0, sem);
	CU_ASSERT_EQUAL(r, 0);
	ptr = ib_result_cpu[1];
	ptr[0] = gfx_nop;
	ib_info[1].ib_mc_address = ib_result_mc_address[1];
	ib_info[1].size = 1;

	ibs_request[1].ip_type = AMDGPU_HW_IP_GFX;
	ibs_request[1].number_of_ibs = 1;
	ibs_request[1].ibs = &ib_info[1];
	ibs_request[1].resources = bo_list[1];
	ibs_request[1].fence_info.handle = NULL;
	r = amdgpu_cs_submit(context_handle[1], 0,&ibs_request[1], 1);

	CU_ASSERT_EQUAL(r, 0);

	fence_status.context = context_handle[1];
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.fence = ibs_request[1].seq_no;
	r = amdgpu_cs_query_fence_status(&fence_status,
					 500000000, 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	for (i = 0; i < 2; i++) {
		r = amdgpu_bo_unmap_and_free(ib_result_handle[i], va_handle[i],
					     ib_result_mc_address[i], 4096);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_bo_list_destroy(bo_list[i]);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_cs_ctx_free(context_handle[i]);
		CU_ASSERT_EQUAL(r, 0);
	}

	r = amdgpu_cs_destroy_semaphore(sem);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_compute_nop(void)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle;
	void *ib_result_cpu;
	uint64_t ib_result_mc_address;
	struct amdgpu_cs_request ibs_request;
	struct amdgpu_cs_ib_info ib_info;
	struct amdgpu_cs_fence fence_status;
	uint32_t *ptr;
	uint32_t expired;
	int i, r, instance;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle;
	struct drm_amdgpu_info_hw_ip info;

	r = amdgpu_query_hw_ip_info(device_handle, AMDGPU_HW_IP_COMPUTE, 0, &info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	for (instance = 0; (1 << instance) & info.available_rings; instance++) {
		r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
					    AMDGPU_GEM_DOMAIN_GTT, 0,
					    &ib_result_handle, &ib_result_cpu,
					    &ib_result_mc_address, &va_handle);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_get_bo_list(device_handle, ib_result_handle, NULL,
				       &bo_list);
		CU_ASSERT_EQUAL(r, 0);

		ptr = ib_result_cpu;
		memset(ptr, 0, 16);
		ptr[0]=PACKET3(PACKET3_NOP, 14);

		memset(&ib_info, 0, sizeof(struct amdgpu_cs_ib_info));
		ib_info.ib_mc_address = ib_result_mc_address;
		ib_info.size = 16;

		memset(&ibs_request, 0, sizeof(struct amdgpu_cs_request));
		ibs_request.ip_type = AMDGPU_HW_IP_COMPUTE;
		ibs_request.ring = instance;
		ibs_request.number_of_ibs = 1;
		ibs_request.ibs = &ib_info;
		ibs_request.resources = bo_list;
		ibs_request.fence_info.handle = NULL;

		memset(&fence_status, 0, sizeof(struct amdgpu_cs_fence));
		r = amdgpu_cs_submit(context_handle, 0,&ibs_request, 1);
		CU_ASSERT_EQUAL(r, 0);

		fence_status.context = context_handle;
		fence_status.ip_type = AMDGPU_HW_IP_COMPUTE;
		fence_status.ip_instance = 0;
		fence_status.ring = instance;
		fence_status.fence = ibs_request.seq_no;

		r = amdgpu_cs_query_fence_status(&fence_status,
						 AMDGPU_TIMEOUT_INFINITE,
						 0, &expired);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_bo_list_destroy(bo_list);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
					     ib_result_mc_address, 4096);
		CU_ASSERT_EQUAL(r, 0);
	}

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_compute_cp_write_data(void)
{
	amdgpu_command_submission_write_linear_helper(AMDGPU_HW_IP_COMPUTE);
}

static void amdgpu_command_submission_compute_cp_const_fill(void)
{
	amdgpu_command_submission_const_fill_helper(AMDGPU_HW_IP_COMPUTE);
}

static void amdgpu_command_submission_compute_cp_copy_data(void)
{
	amdgpu_command_submission_copy_linear_helper(AMDGPU_HW_IP_COMPUTE);
}

static void amdgpu_command_submission_compute(void)
{
	/* write data using the CP */
	amdgpu_command_submission_compute_cp_write_data();
	/* const fill using the CP */
	amdgpu_command_submission_compute_cp_const_fill();
	/* copy data using the CP */
	amdgpu_command_submission_compute_cp_copy_data();
	/* nop test */
	amdgpu_command_submission_compute_nop();
}

/*
 * caller need create/release:
 * pm4_src, resources, ib_info, and ibs_request
 * submit command stream described in ibs_request and wait for this IB accomplished
 */
static void amdgpu_test_exec_cs_helper(amdgpu_context_handle context_handle,
				       unsigned ip_type,
				       int instance, int pm4_dw, uint32_t *pm4_src,
				       int res_cnt, amdgpu_bo_handle *resources,
				       struct amdgpu_cs_ib_info *ib_info,
				       struct amdgpu_cs_request *ibs_request)
{
	int r;
	uint32_t expired;
	uint32_t *ring_ptr;
	amdgpu_bo_handle ib_result_handle;
	void *ib_result_cpu;
	uint64_t ib_result_mc_address;
	struct amdgpu_cs_fence fence_status = {0};
	amdgpu_bo_handle *all_res = alloca(sizeof(resources[0]) * (res_cnt + 1));
	amdgpu_va_handle va_handle;

	/* prepare CS */
	CU_ASSERT_NOT_EQUAL(pm4_src, NULL);
	CU_ASSERT_NOT_EQUAL(resources, NULL);
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);
	CU_ASSERT_TRUE(pm4_dw <= 1024);

	/* allocate IB */
	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_handle, &ib_result_cpu,
				    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* copy PM4 packet to ring from caller */
	ring_ptr = ib_result_cpu;
	memcpy(ring_ptr, pm4_src, pm4_dw * sizeof(*pm4_src));

	ib_info->ib_mc_address = ib_result_mc_address;
	ib_info->size = pm4_dw;

	ibs_request->ip_type = ip_type;
	ibs_request->ring = instance;
	ibs_request->number_of_ibs = 1;
	ibs_request->ibs = ib_info;
	ibs_request->fence_info.handle = NULL;

	memcpy(all_res, resources, sizeof(resources[0]) * res_cnt);
	all_res[res_cnt] = ib_result_handle;

	r = amdgpu_bo_list_create(device_handle, res_cnt+1, all_res,
				  NULL, &ibs_request->resources);
	CU_ASSERT_EQUAL(r, 0);

	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	/* submit CS */
	r = amdgpu_cs_submit(context_handle, 0, ibs_request, 1);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(ibs_request->resources);
	CU_ASSERT_EQUAL(r, 0);

	fence_status.ip_type = ip_type;
	fence_status.ip_instance = 0;
	fence_status.ring = ibs_request->ring;
	fence_status.context = context_handle;
	fence_status.fence = ibs_request->seq_no;

	/* wait for IB accomplished */
	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_write_linear_helper(unsigned ip_type)
{
	const int sdma_write_length = 128;
	const int pm4_dw = 256;
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo;
	amdgpu_bo_handle *resources;
	uint32_t *pm4;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	uint64_t bo_mc;
	volatile uint32_t *bo_cpu;
	int i, j, r, loop;
	uint64_t gtt_flags[2] = {0, AMDGPU_GEM_CREATE_CPU_GTT_USWC};
	amdgpu_va_handle va_handle;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* prepare resource */
	resources = calloc(1, sizeof(amdgpu_bo_handle));
	CU_ASSERT_NOT_EQUAL(resources, NULL);

	loop = 0;
	while(loop < 2) {
		/* allocate UC bo for sDMA use */
		r = amdgpu_bo_alloc_and_map(device_handle,
					    sdma_write_length * sizeof(uint32_t),
					    4096, AMDGPU_GEM_DOMAIN_GTT,
					    gtt_flags[loop], &bo, (void**)&bo_cpu,
					    &bo_mc, &va_handle);
		CU_ASSERT_EQUAL(r, 0);

		/* clear bo */
		memset((void*)bo_cpu, 0, sdma_write_length * sizeof(uint32_t));


		resources[0] = bo;

		/* fulfill PM4: test DMA write-linear */
		i = j = 0;
		if (ip_type == AMDGPU_HW_IP_DMA) {
			if (family_id == AMDGPU_FAMILY_SI)
				pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_WRITE, 0, 0, 0,
							  sdma_write_length);
			else
				pm4[i++] = SDMA_PACKET(SDMA_OPCODE_WRITE,
						       SDMA_WRITE_SUB_OPCODE_LINEAR, 0);
			pm4[i++] = 0xffffffff & bo_mc;
			pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
			if (family_id >= AMDGPU_FAMILY_AI)
				pm4[i++] = sdma_write_length - 1;
			else if (family_id != AMDGPU_FAMILY_SI)
				pm4[i++] = sdma_write_length;
			while(j++ < sdma_write_length)
				pm4[i++] = 0xdeadbeaf;
		} else if ((ip_type == AMDGPU_HW_IP_GFX) ||
			   (ip_type == AMDGPU_HW_IP_COMPUTE)) {
			pm4[i++] = PACKET3(PACKET3_WRITE_DATA, 2 + sdma_write_length);
			pm4[i++] = WRITE_DATA_DST_SEL(5) | WR_CONFIRM;
			pm4[i++] = 0xfffffffc & bo_mc;
			pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
			while(j++ < sdma_write_length)
				pm4[i++] = 0xdeadbeaf;
		}

		amdgpu_test_exec_cs_helper(context_handle,
					   ip_type, 0,
					   i, pm4,
					   1, resources,
					   ib_info, ibs_request);

		/* verify if SDMA test result meets with expected */
		i = 0;
		while(i < sdma_write_length) {
			CU_ASSERT_EQUAL(bo_cpu[i++], 0xdeadbeaf);
		}

		r = amdgpu_bo_unmap_and_free(bo, va_handle, bo_mc,
					     sdma_write_length * sizeof(uint32_t));
		CU_ASSERT_EQUAL(r, 0);
		loop++;
	}
	/* clean resources */
	free(resources);
	free(ibs_request);
	free(ib_info);
	free(pm4);

	/* end of test */
	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_sdma_write_linear(void)
{
	amdgpu_command_submission_write_linear_helper(AMDGPU_HW_IP_DMA);
}

static void amdgpu_command_submission_const_fill_helper(unsigned ip_type)
{
	const int sdma_write_length = 1024 * 1024;
	const int pm4_dw = 256;
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo;
	amdgpu_bo_handle *resources;
	uint32_t *pm4;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	uint64_t bo_mc;
	volatile uint32_t *bo_cpu;
	int i, j, r, loop;
	uint64_t gtt_flags[2] = {0, AMDGPU_GEM_CREATE_CPU_GTT_USWC};
	amdgpu_va_handle va_handle;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* prepare resource */
	resources = calloc(1, sizeof(amdgpu_bo_handle));
	CU_ASSERT_NOT_EQUAL(resources, NULL);

	loop = 0;
	while(loop < 2) {
		/* allocate UC bo for sDMA use */
		r = amdgpu_bo_alloc_and_map(device_handle,
					    sdma_write_length, 4096,
					    AMDGPU_GEM_DOMAIN_GTT,
					    gtt_flags[loop], &bo, (void**)&bo_cpu,
					    &bo_mc, &va_handle);
		CU_ASSERT_EQUAL(r, 0);

		/* clear bo */
		memset((void*)bo_cpu, 0, sdma_write_length);

		resources[0] = bo;

		/* fulfill PM4: test DMA const fill */
		i = j = 0;
		if (ip_type == AMDGPU_HW_IP_DMA) {
			if (family_id == AMDGPU_FAMILY_SI) {
				pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_CONSTANT_FILL_SI, 0, 0, 0,
							  sdma_write_length / 4);
				pm4[i++] = 0xfffffffc & bo_mc;
				pm4[i++] = 0xdeadbeaf;
				pm4[i++] = (0xffffffff00000000 & bo_mc) >> 16;
			} else {
				pm4[i++] = SDMA_PACKET(SDMA_OPCODE_CONSTANT_FILL, 0,
						       SDMA_CONSTANT_FILL_EXTRA_SIZE(2));
				pm4[i++] = 0xffffffff & bo_mc;
				pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
				pm4[i++] = 0xdeadbeaf;
				if (family_id >= AMDGPU_FAMILY_AI)
					pm4[i++] = sdma_write_length - 1;
				else
					pm4[i++] = sdma_write_length;
			}
		} else if ((ip_type == AMDGPU_HW_IP_GFX) ||
			   (ip_type == AMDGPU_HW_IP_COMPUTE)) {
			if (family_id == AMDGPU_FAMILY_SI) {
				pm4[i++] = PACKET3(PACKET3_DMA_DATA_SI, 4);
				pm4[i++] = 0xdeadbeaf;
				pm4[i++] = PACKET3_DMA_DATA_SI_ENGINE(0) |
					PACKET3_DMA_DATA_SI_DST_SEL(0) |
					PACKET3_DMA_DATA_SI_SRC_SEL(2) |
					PACKET3_DMA_DATA_SI_CP_SYNC;
				pm4[i++] = 0xffffffff & bo_mc;
				pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
				pm4[i++] = sdma_write_length;
			} else {
				pm4[i++] = PACKET3(PACKET3_DMA_DATA, 5);
				pm4[i++] = PACKET3_DMA_DATA_ENGINE(0) |
					PACKET3_DMA_DATA_DST_SEL(0) |
					PACKET3_DMA_DATA_SRC_SEL(2) |
					PACKET3_DMA_DATA_CP_SYNC;
				pm4[i++] = 0xdeadbeaf;
				pm4[i++] = 0;
				pm4[i++] = 0xfffffffc & bo_mc;
				pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
				pm4[i++] = sdma_write_length;
			}
		}

		amdgpu_test_exec_cs_helper(context_handle,
					   ip_type, 0,
					   i, pm4,
					   1, resources,
					   ib_info, ibs_request);

		/* verify if SDMA test result meets with expected */
		i = 0;
		while(i < (sdma_write_length / 4)) {
			CU_ASSERT_EQUAL(bo_cpu[i++], 0xdeadbeaf);
		}

		r = amdgpu_bo_unmap_and_free(bo, va_handle, bo_mc,
					     sdma_write_length);
		CU_ASSERT_EQUAL(r, 0);
		loop++;
	}
	/* clean resources */
	free(resources);
	free(ibs_request);
	free(ib_info);
	free(pm4);

	/* end of test */
	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_sdma_const_fill(void)
{
	amdgpu_command_submission_const_fill_helper(AMDGPU_HW_IP_DMA);
}

static void amdgpu_command_submission_copy_linear_helper(unsigned ip_type)
{
	const int sdma_write_length = 1024;
	const int pm4_dw = 256;
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo1, bo2;
	amdgpu_bo_handle *resources;
	uint32_t *pm4;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	uint64_t bo1_mc, bo2_mc;
	volatile unsigned char *bo1_cpu, *bo2_cpu;
	int i, j, r, loop1, loop2;
	uint64_t gtt_flags[2] = {0, AMDGPU_GEM_CREATE_CPU_GTT_USWC};
	amdgpu_va_handle bo1_va_handle, bo2_va_handle;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* prepare resource */
	resources = calloc(2, sizeof(amdgpu_bo_handle));
	CU_ASSERT_NOT_EQUAL(resources, NULL);

	loop1 = loop2 = 0;
	/* run 9 circle to test all mapping combination */
	while(loop1 < 2) {
		while(loop2 < 2) {
			/* allocate UC bo1for sDMA use */
			r = amdgpu_bo_alloc_and_map(device_handle,
						    sdma_write_length, 4096,
						    AMDGPU_GEM_DOMAIN_GTT,
						    gtt_flags[loop1], &bo1,
						    (void**)&bo1_cpu, &bo1_mc,
						    &bo1_va_handle);
			CU_ASSERT_EQUAL(r, 0);

			/* set bo1 */
			memset((void*)bo1_cpu, 0xaa, sdma_write_length);

			/* allocate UC bo2 for sDMA use */
			r = amdgpu_bo_alloc_and_map(device_handle,
						    sdma_write_length, 4096,
						    AMDGPU_GEM_DOMAIN_GTT,
						    gtt_flags[loop2], &bo2,
						    (void**)&bo2_cpu, &bo2_mc,
						    &bo2_va_handle);
			CU_ASSERT_EQUAL(r, 0);

			/* clear bo2 */
			memset((void*)bo2_cpu, 0, sdma_write_length);

			resources[0] = bo1;
			resources[1] = bo2;

			/* fulfill PM4: test DMA copy linear */
			i = j = 0;
			if (ip_type == AMDGPU_HW_IP_DMA) {
				if (family_id == AMDGPU_FAMILY_SI) {
					pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_COPY_SI, 0, 0, 0,
								  sdma_write_length);
					pm4[i++] = 0xffffffff & bo2_mc;
					pm4[i++] = 0xffffffff & bo1_mc;
					pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
					pm4[i++] = (0xffffffff00000000 & bo1_mc) >> 32;
				} else {
					pm4[i++] = SDMA_PACKET(SDMA_OPCODE_COPY, SDMA_COPY_SUB_OPCODE_LINEAR, 0);
					if (family_id >= AMDGPU_FAMILY_AI)
						pm4[i++] = sdma_write_length - 1;
					else
						pm4[i++] = sdma_write_length;
					pm4[i++] = 0;
					pm4[i++] = 0xffffffff & bo1_mc;
					pm4[i++] = (0xffffffff00000000 & bo1_mc) >> 32;
					pm4[i++] = 0xffffffff & bo2_mc;
					pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
				}

			} else if ((ip_type == AMDGPU_HW_IP_GFX) ||
				   (ip_type == AMDGPU_HW_IP_COMPUTE)) {
				if (family_id == AMDGPU_FAMILY_SI) {
					pm4[i++] = PACKET3(PACKET3_DMA_DATA_SI, 4);
					pm4[i++] = 0xfffffffc & bo1_mc;
					pm4[i++] = PACKET3_DMA_DATA_SI_ENGINE(0) |
						PACKET3_DMA_DATA_SI_DST_SEL(0) |
						PACKET3_DMA_DATA_SI_SRC_SEL(0) |
						PACKET3_DMA_DATA_SI_CP_SYNC |
						(0xffff00000000 & bo1_mc) >> 32;
					pm4[i++] = 0xfffffffc & bo2_mc;
					pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
					pm4[i++] = sdma_write_length;
				} else {
					pm4[i++] = PACKET3(PACKET3_DMA_DATA, 5);
					pm4[i++] = PACKET3_DMA_DATA_ENGINE(0) |
						PACKET3_DMA_DATA_DST_SEL(0) |
						PACKET3_DMA_DATA_SRC_SEL(0) |
						PACKET3_DMA_DATA_CP_SYNC;
					pm4[i++] = 0xfffffffc & bo1_mc;
					pm4[i++] = (0xffffffff00000000 & bo1_mc) >> 32;
					pm4[i++] = 0xfffffffc & bo2_mc;
					pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
					pm4[i++] = sdma_write_length;
				}
			}

			amdgpu_test_exec_cs_helper(context_handle,
						   ip_type, 0,
						   i, pm4,
						   2, resources,
						   ib_info, ibs_request);

			/* verify if SDMA test result meets with expected */
			i = 0;
			while(i < sdma_write_length) {
				CU_ASSERT_EQUAL(bo2_cpu[i++], 0xaa);
			}
			r = amdgpu_bo_unmap_and_free(bo1, bo1_va_handle, bo1_mc,
						     sdma_write_length);
			CU_ASSERT_EQUAL(r, 0);
			r = amdgpu_bo_unmap_and_free(bo2, bo2_va_handle, bo2_mc,
						     sdma_write_length);
			CU_ASSERT_EQUAL(r, 0);
			loop2++;
		}
		loop1++;
	}
	/* clean resources */
	free(resources);
	free(ibs_request);
	free(ib_info);
	free(pm4);

	/* end of test */
	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_sdma_copy_linear(void)
{
	amdgpu_command_submission_copy_linear_helper(AMDGPU_HW_IP_DMA);
}

static void amdgpu_command_submission_sdma(void)
{
	amdgpu_command_submission_sdma_write_linear();
	amdgpu_command_submission_sdma_const_fill();
	amdgpu_command_submission_sdma_copy_linear();
}

static void amdgpu_command_submission_multi_fence_wait_all(bool wait_all)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle, ib_result_ce_handle;
	void *ib_result_cpu, *ib_result_ce_cpu;
	uint64_t ib_result_mc_address, ib_result_ce_mc_address;
	struct amdgpu_cs_request ibs_request[2] = {0};
	struct amdgpu_cs_ib_info ib_info[2];
	struct amdgpu_cs_fence fence_status[2] = {0};
	uint32_t *ptr;
	uint32_t expired;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle, va_handle_ce;
	int r;
	int i = 0, ib_cs_num = 2;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_handle, &ib_result_cpu,
				    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_ce_handle, &ib_result_ce_cpu,
				    &ib_result_ce_mc_address, &va_handle_ce);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_get_bo_list(device_handle, ib_result_handle,
			       ib_result_ce_handle, &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	memset(ib_info, 0, 2 * sizeof(struct amdgpu_cs_ib_info));

	/* IT_SET_CE_DE_COUNTERS */
	ptr = ib_result_ce_cpu;
	if (family_id != AMDGPU_FAMILY_SI) {
		ptr[i++] = 0xc0008900;
		ptr[i++] = 0;
	}
	ptr[i++] = 0xc0008400;
	ptr[i++] = 1;
	ib_info[0].ib_mc_address = ib_result_ce_mc_address;
	ib_info[0].size = i;
	ib_info[0].flags = AMDGPU_IB_FLAG_CE;

	/* IT_WAIT_ON_CE_COUNTER */
	ptr = ib_result_cpu;
	ptr[0] = 0xc0008600;
	ptr[1] = 0x00000001;
	ib_info[1].ib_mc_address = ib_result_mc_address;
	ib_info[1].size = 2;

	for (i = 0; i < ib_cs_num; i++) {
		ibs_request[i].ip_type = AMDGPU_HW_IP_GFX;
		ibs_request[i].number_of_ibs = 2;
		ibs_request[i].ibs = ib_info;
		ibs_request[i].resources = bo_list;
		ibs_request[i].fence_info.handle = NULL;
	}

	r = amdgpu_cs_submit(context_handle, 0,ibs_request, ib_cs_num);

	CU_ASSERT_EQUAL(r, 0);

	for (i = 0; i < ib_cs_num; i++) {
		fence_status[i].context = context_handle;
		fence_status[i].ip_type = AMDGPU_HW_IP_GFX;
		fence_status[i].fence = ibs_request[i].seq_no;
	}

	r = amdgpu_cs_wait_fences(fence_status, ib_cs_num, wait_all,
				AMDGPU_TIMEOUT_INFINITE,
				&expired, NULL);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_ce_handle, va_handle_ce,
				     ib_result_ce_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_multi_fence(void)
{
	amdgpu_command_submission_multi_fence_wait_all(true);
	amdgpu_command_submission_multi_fence_wait_all(false);
}

static void amdgpu_userptr_test(void)
{
	int i, r, j;
	uint32_t *pm4 = NULL;
	uint64_t bo_mc;
	void *ptr = NULL;
	int pm4_dw = 256;
	int sdma_write_length = 4;
	amdgpu_bo_handle handle;
	amdgpu_context_handle context_handle;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	amdgpu_bo_handle buf_handle;
	amdgpu_va_handle va_handle;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	posix_memalign(&ptr, sysconf(_SC_PAGE_SIZE), BUFFER_SIZE);
	CU_ASSERT_NOT_EQUAL(ptr, NULL);
	memset(ptr, 0, BUFFER_SIZE);

	r = amdgpu_create_bo_from_user_mem(device_handle,
					   ptr, BUFFER_SIZE, &buf_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_va_range_alloc(device_handle,
				  amdgpu_gpu_va_range_general,
				  BUFFER_SIZE, 1, 0, &bo_mc,
				  &va_handle, 0);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_va_op(buf_handle, 0, BUFFER_SIZE, bo_mc, 0, AMDGPU_VA_OP_MAP);
	CU_ASSERT_EQUAL(r, 0);

	handle = buf_handle;

	j = i = 0;

	if (family_id == AMDGPU_FAMILY_SI)
		pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_WRITE, 0, 0, 0,
				sdma_write_length);
	else
		pm4[i++] = SDMA_PACKET(SDMA_OPCODE_WRITE,
				SDMA_WRITE_SUB_OPCODE_LINEAR, 0);
	pm4[i++] = 0xffffffff & bo_mc;
	pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
	if (family_id >= AMDGPU_FAMILY_AI)
		pm4[i++] = sdma_write_length - 1;
	else if (family_id != AMDGPU_FAMILY_SI)
		pm4[i++] = sdma_write_length;

	while (j++ < sdma_write_length)
		pm4[i++] = 0xdeadbeaf;

	if (!fork()) {
		pm4[0] = 0x0;
		exit(0);
	}

	amdgpu_test_exec_cs_helper(context_handle,
				   AMDGPU_HW_IP_DMA, 0,
				   i, pm4,
				   1, &handle,
				   ib_info, ibs_request);
	i = 0;
	while (i < sdma_write_length) {
		CU_ASSERT_EQUAL(((int*)ptr)[i++], 0xdeadbeaf);
	}
	free(ibs_request);
	free(ib_info);
	free(pm4);

	r = amdgpu_bo_va_op(buf_handle, 0, BUFFER_SIZE, bo_mc, 0, AMDGPU_VA_OP_UNMAP);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_va_range_free(va_handle);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_bo_free(buf_handle);
	CU_ASSERT_EQUAL(r, 0);
	free(ptr);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);

	wait(NULL);
}
