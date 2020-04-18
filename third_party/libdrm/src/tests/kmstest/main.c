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

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include "xf86drm.h"
#include "libkms.h"

#include "util/kms.h"

#define CHECK_RET_RETURN(ret, str) \
	if (ret < 0) { \
		printf("%s: %s (%s)\n", __func__, str, strerror(-ret)); \
		return ret; \
	}

static int test_bo(struct kms_driver *kms)
{
	struct kms_bo *bo;
	int ret;
	unsigned attrs[7] = {
		KMS_WIDTH, 1024,
		KMS_HEIGHT, 768,
		KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
		KMS_TERMINATE_PROP_LIST,
	};

	ret = kms_bo_create(kms, attrs, &bo);
	CHECK_RET_RETURN(ret, "Could not create bo");

	kms_bo_destroy(&bo);

	return 0;
}

static void usage(const char *program)
{
	fprintf(stderr, "Usage: %s [options]\n", program);
	fprintf(stderr, "\n");
	fprintf(stderr, "  -D DEVICE  open the given device\n");
	fprintf(stderr, "  -M MODULE  open the given module\n");
}

int main(int argc, char** argv)
{
	static const char optstr[] = "D:M:";
	struct kms_driver *kms;
	int c, fd, ret;
	char *device = NULL;
	char *module = NULL;

	while ((c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 'M':
			module = optarg;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	fd = util_open(device, module);
	CHECK_RET_RETURN(fd, "Could not open device");

	ret = kms_create(fd, &kms);
	CHECK_RET_RETURN(ret, "Failed to create kms driver");

	ret = test_bo(kms);
	if (ret)
		goto err;

	printf("%s: All ok!\n", __func__);

	kms_destroy(&kms);
	return 0;

err:
	kms_destroy(&kms);
	return ret;
}
