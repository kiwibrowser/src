/**************************************************************************
 *
 * Copyright © 2009 VMware, Inc., Palo Alto, CA., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef _LIBKMS_H_
#define _LIBKMS_H_

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * \file
 *
 */

struct kms_driver;
struct kms_bo;

enum kms_attrib
{
	KMS_TERMINATE_PROP_LIST,
#define KMS_TERMINATE_PROP_LIST KMS_TERMINATE_PROP_LIST
	KMS_BO_TYPE,
#define KMS_BO_TYPE KMS_BO_TYPE
	KMS_WIDTH,
#define KMS_WIDTH KMS_WIDTH
	KMS_HEIGHT,
#define KMS_HEIGHT KMS_HEIGHT
	KMS_PITCH,
#define KMS_PITCH KMS_PITCH
	KMS_HANDLE,
#define KMS_HANDLE KMS_HANDLE
};

enum kms_bo_type
{
	KMS_BO_TYPE_SCANOUT_X8R8G8B8 = (1 << 0),
#define KMS_BO_TYPE_SCANOUT_X8R8G8B8 KMS_BO_TYPE_SCANOUT_X8R8G8B8
	KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8 =  (1 << 1),
#define KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8 KMS_BO_TYPE_CURSOR_64X64_A8R8G8B8
};

int kms_create(int fd, struct kms_driver **out);
int kms_get_prop(struct kms_driver *kms, unsigned key, unsigned *out);
int kms_destroy(struct kms_driver **kms);

int kms_bo_create(struct kms_driver *kms, const unsigned *attr, struct kms_bo **out);
int kms_bo_get_prop(struct kms_bo *bo, unsigned key, unsigned *out);
int kms_bo_map(struct kms_bo *bo, void **out);
int kms_bo_unmap(struct kms_bo *bo);
int kms_bo_destroy(struct kms_bo **bo);

#if defined(__cplusplus)
};
#endif

#endif
