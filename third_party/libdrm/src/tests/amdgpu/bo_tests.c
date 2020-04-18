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

#include "CUnit/Basic.h"

#include "amdgpu_test.h"
#include "amdgpu_drm.h"

#define BUFFER_SIZE (4*1024)
#define BUFFER_ALIGN (4*1024)

static amdgpu_device_handle device_handle;
static uint32_t major_version;
static uint32_t minor_version;

static amdgpu_bo_handle buffer_handle;
static uint64_t virtual_mc_base_address;
static amdgpu_va_handle va_handle;

static void amdgpu_bo_export_import(void);
static void amdgpu_bo_metadata(void);
static void amdgpu_bo_map_unmap(void);

CU_TestInfo bo_tests[] = {
	{ "Export/Import",  amdgpu_bo_export_import },
#if 0
	{ "Metadata",  amdgpu_bo_metadata },
#endif
	{ "CPU map/unmap",  amdgpu_bo_map_unmap },
	CU_TEST_INFO_NULL,
};

int suite_bo_tests_init(void)
{
	struct amdgpu_bo_alloc_request req = {0};
	amdgpu_bo_handle buf_handle;
	uint64_t va;
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

	req.alloc_size = BUFFER_SIZE;
	req.phys_alignment = BUFFER_ALIGN;
	req.preferred_heap = AMDGPU_GEM_DOMAIN_GTT;

	r = amdgpu_bo_alloc(device_handle, &req, &buf_handle);
	if (r)
		return CUE_SINIT_FAILED;

	r = amdgpu_va_range_alloc(device_handle,
				  amdgpu_gpu_va_range_general,
				  BUFFER_SIZE, BUFFER_ALIGN, 0,
				  &va, &va_handle, 0);
	if (r)
		goto error_va_alloc;

	r = amdgpu_bo_va_op(buf_handle, 0, BUFFER_SIZE, va, 0, AMDGPU_VA_OP_MAP);
	if (r)
		goto error_va_map;

	buffer_handle = buf_handle;
	virtual_mc_base_address = va;

	return CUE_SUCCESS;

error_va_map:
	amdgpu_va_range_free(va_handle);

error_va_alloc:
	amdgpu_bo_free(buf_handle);
	return CUE_SINIT_FAILED;
}

int suite_bo_tests_clean(void)
{
	int r;

	r = amdgpu_bo_va_op(buffer_handle, 0, BUFFER_SIZE,
			    virtual_mc_base_address, 0,
			    AMDGPU_VA_OP_UNMAP);
	if (r)
		return CUE_SCLEAN_FAILED;

	r = amdgpu_va_range_free(va_handle);
	if (r)
		return CUE_SCLEAN_FAILED;

	r = amdgpu_bo_free(buffer_handle);
	if (r)
		return CUE_SCLEAN_FAILED;

	r = amdgpu_device_deinitialize(device_handle);
	if (r)
		return CUE_SCLEAN_FAILED;

	return CUE_SUCCESS;
}

static void amdgpu_bo_export_import_do_type(enum amdgpu_bo_handle_type type)
{
	struct amdgpu_bo_import_result res = {0};
	uint32_t shared_handle;
	int r;

	r = amdgpu_bo_export(buffer_handle, type, &shared_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_import(device_handle, type, shared_handle, &res);
	CU_ASSERT_EQUAL(r, 0);

	CU_ASSERT_EQUAL(res.buf_handle, buffer_handle);
	CU_ASSERT_EQUAL(res.alloc_size, BUFFER_SIZE);

	r = amdgpu_bo_free(res.buf_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_bo_export_import(void)
{
	if (open_render_node) {
		printf("(DRM render node is used. Skip export/Import test) ");
		return;
	}

	amdgpu_bo_export_import_do_type(amdgpu_bo_handle_type_gem_flink_name);
	amdgpu_bo_export_import_do_type(amdgpu_bo_handle_type_dma_buf_fd);
}

static void amdgpu_bo_metadata(void)
{
	struct amdgpu_bo_metadata meta = {0};
	struct amdgpu_bo_info info = {0};
	int r;

	meta.size_metadata = 1;
	meta.umd_metadata[0] = 0xdeadbeef;

	r = amdgpu_bo_set_metadata(buffer_handle, &meta);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_query_info(buffer_handle, &info);
	CU_ASSERT_EQUAL(r, 0);

	CU_ASSERT_EQUAL(info.metadata.size_metadata, 1);
	CU_ASSERT_EQUAL(info.metadata.umd_metadata[0], 0xdeadbeef);
}

static void amdgpu_bo_map_unmap(void)
{
	uint32_t *ptr;
	int i, r;

	r = amdgpu_bo_cpu_map(buffer_handle, (void **)&ptr);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_NOT_EQUAL(ptr, NULL);

	for (i = 0; i < (BUFFER_SIZE / 4); ++i)
		ptr[i] = 0xdeadbeef;

	r = amdgpu_bo_cpu_unmap(buffer_handle);
	CU_ASSERT_EQUAL(r, 0);
}
