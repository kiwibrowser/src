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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdint.h>

#include "drm.h"
#include "xf86drmMode.h"
#include "xf86drm.h"

#include "CUnit/Basic.h"

#include "amdgpu_test.h"

/**
 *  Open handles for amdgpu devices
 *
 */
int drm_amdgpu[MAX_CARDS_SUPPORTED];

/** Open render node to test */
int open_render_node = 0;	/* By default run most tests on primary node */

/** The table of all known test suites to run */
static CU_SuiteInfo suites[] = {
	{
		.pName = "Basic Tests",
		.pInitFunc = suite_basic_tests_init,
		.pCleanupFunc = suite_basic_tests_clean,
		.pTests = basic_tests,
	},
	{
		.pName = "BO Tests",
		.pInitFunc = suite_bo_tests_init,
		.pCleanupFunc = suite_bo_tests_clean,
		.pTests = bo_tests,
	},
	{
		.pName = "CS Tests",
		.pInitFunc = suite_cs_tests_init,
		.pCleanupFunc = suite_cs_tests_clean,
		.pTests = cs_tests,
	},
	{
		.pName = "VCE Tests",
		.pInitFunc = suite_vce_tests_init,
		.pCleanupFunc = suite_vce_tests_clean,
		.pTests = vce_tests,
	},
	{
		.pName = "VCN Tests",
		.pInitFunc = suite_vcn_tests_init,
		.pCleanupFunc = suite_vcn_tests_clean,
		.pTests = vcn_tests,
	},
	{
		.pName = "UVD ENC Tests",
		.pInitFunc = suite_uvd_enc_tests_init,
		.pCleanupFunc = suite_uvd_enc_tests_clean,
		.pTests = uvd_enc_tests,
	},
	{
		.pName = "Deadlock Tests",
		.pInitFunc = suite_deadlock_tests_init,
		.pCleanupFunc = suite_deadlock_tests_clean,
		.pTests = deadlock_tests,
	},
	CU_SUITE_INFO_NULL,
};


/** Display information about all  suites and their tests */
static void display_test_suites(void)
{
	int iSuite;
	int iTest;

	printf("Suites\n");

	for (iSuite = 0; suites[iSuite].pName != NULL; iSuite++) {
		printf("Suite id = %d: Name '%s'\n",
				iSuite + 1, suites[iSuite].pName);

		for (iTest = 0; suites[iSuite].pTests[iTest].pName != NULL;
			iTest++) {
			printf("	Test id %d: Name: '%s'\n", iTest + 1,
					suites[iSuite].pTests[iTest].pName);
		}
	}
}


/** Help string for command line parameters */
static const char usage[] =
	"Usage: %s [-hlpr] [<-s <suite id>> [-t <test id>]] "
	"[-b <pci_bus_id> [-d <pci_device_id>]]\n"
	"where:\n"
	"       l - Display all suites and their tests\n"
	"       r - Run the tests on render node\n"
	"       b - Specify device's PCI bus id to run tests\n"
	"       d - Specify device's PCI device id to run tests (optional)\n"
	"       p - Display information of AMDGPU devices in system\n"
	"       h - Display this help\n";
/** Specified options strings for getopt */
static const char options[]   = "hlrps:t:b:d:";

/* Open AMD devices.
 * Return the number of AMD device openned.
 */
static int amdgpu_open_devices(int open_render_node)
{
	drmDevicePtr devices[MAX_CARDS_SUPPORTED];
	int ret;
	int i;
	int drm_node;
	int amd_index = 0;
	int drm_count;
	int fd;
	drmVersionPtr version;

	drm_count = drmGetDevices2(0, devices, MAX_CARDS_SUPPORTED);

	if (drm_count < 0) {
		fprintf(stderr,
			"drmGetDevices2() returned an error %d\n",
			drm_count);
		return 0;
	}

	for (i = 0; i < drm_count; i++) {
		/* If this is not PCI device, skip*/
		if (devices[i]->bustype != DRM_BUS_PCI)
			continue;

		/* If this is not AMD GPU vender ID, skip*/
		if (devices[i]->deviceinfo.pci->vendor_id != 0x1002)
			continue;

		if (open_render_node)
			drm_node = DRM_NODE_RENDER;
		else
			drm_node = DRM_NODE_PRIMARY;

		fd = -1;
		if (devices[i]->available_nodes & 1 << drm_node)
			fd = open(
				devices[i]->nodes[drm_node],
				O_RDWR | O_CLOEXEC);

		/* This node is not available. */
		if (fd < 0) continue;

		version = drmGetVersion(fd);
		if (!version) {
			fprintf(stderr,
				"Warning: Cannot get version for %s."
				"Error is %s\n",
				devices[i]->nodes[drm_node],
				strerror(errno));
			close(fd);
			continue;
		}

		if (strcmp(version->name, "amdgpu")) {
			/* This is not AMDGPU driver, skip.*/
			drmFreeVersion(version);
			close(fd);
			continue;
		}

		drmFreeVersion(version);

		drm_amdgpu[amd_index] = fd;
		amd_index++;
	}

	drmFreeDevices(devices, drm_count);
	return amd_index;
}

/* Close AMD devices.
 */
static void amdgpu_close_devices()
{
	int i;
	for (i = 0; i < MAX_CARDS_SUPPORTED; i++)
		if (drm_amdgpu[i] >=0)
			close(drm_amdgpu[i]);
}

/* Print AMD devices information */
static void amdgpu_print_devices()
{
	int i;
	drmDevicePtr device;

	/* Open the first AMD devcie to print driver information. */
	if (drm_amdgpu[0] >=0) {
		/* Display AMD driver version information.*/
		drmVersionPtr retval = drmGetVersion(drm_amdgpu[0]);

		if (retval == NULL) {
			perror("Cannot get version for AMDGPU device");
			return;
		}

		printf("Driver name: %s, Date: %s, Description: %s.\n",
			retval->name, retval->date, retval->desc);
		drmFreeVersion(retval);
	}

	/* Display information of AMD devices */
	printf("Devices:\n");
	for (i = 0; i < MAX_CARDS_SUPPORTED && drm_amdgpu[i] >=0; i++)
		if (drmGetDevice2(drm_amdgpu[i],
			DRM_DEVICE_GET_PCI_REVISION,
			&device) == 0) {
			if (device->bustype == DRM_BUS_PCI) {
				printf("PCI ");
				printf(" domain:%04x",
					device->businfo.pci->domain);
				printf(" bus:%02x",
					device->businfo.pci->bus);
				printf(" device:%02x",
					device->businfo.pci->dev);
				printf(" function:%01x",
					device->businfo.pci->func);
				printf(" vendor_id:%04x",
					device->deviceinfo.pci->vendor_id);
				printf(" device_id:%04x",
					device->deviceinfo.pci->device_id);
				printf(" subvendor_id:%04x",
					device->deviceinfo.pci->subvendor_id);
				printf(" subdevice_id:%04x",
					device->deviceinfo.pci->subdevice_id);
				printf(" revision_id:%02x",
					device->deviceinfo.pci->revision_id);
				printf("\n");
			}
			drmFreeDevice(&device);
		}
}

/* Find a match AMD device in PCI bus
 * Return the index of the device or -1 if not found
 */
static int amdgpu_find_device(uint8_t bus, uint16_t dev)
{
	int i;
	drmDevicePtr device;

	for (i = 0; i < MAX_CARDS_SUPPORTED && drm_amdgpu[i] >= 0; i++) {
		if (drmGetDevice2(drm_amdgpu[i],
			DRM_DEVICE_GET_PCI_REVISION,
			&device) == 0) {
			if (device->bustype == DRM_BUS_PCI)
				if ((bus == 0xFF || device->businfo.pci->bus == bus) &&
					device->deviceinfo.pci->device_id == dev) {
					drmFreeDevice(&device);
					return i;
				}

			drmFreeDevice(&device);
		}
	}

	return -1;
}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int main(int argc, char **argv)
{
	int c;			/* Character received from getopt */
	int i = 0;
	int suite_id = -1;	/* By default run everything */
	int test_id  = -1;	/* By default run all tests in the suite */
	int pci_bus_id = -1;    /* By default PC bus ID is not specified */
	int pci_device_id = 0;  /* By default PC device ID is zero */
	int display_devices = 0;/* By default not to display devices' info */
	CU_pSuite pSuite = NULL;
	CU_pTest  pTest  = NULL;
	int test_device_index;

	for (i = 0; i < MAX_CARDS_SUPPORTED; i++)
		drm_amdgpu[i] = -1;


	/* Parse command line string */
	opterr = 0;		/* Do not print error messages from getopt */
	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {
		case 'l':
			display_test_suites();
			exit(EXIT_SUCCESS);
		case 's':
			suite_id = atoi(optarg);
			break;
		case 't':
			test_id = atoi(optarg);
			break;
		case 'b':
			pci_bus_id = atoi(optarg);
			break;
		case 'd':
			sscanf(optarg, "%x", &pci_device_id);
			break;
		case 'p':
			display_devices = 1;
			break;
		case 'r':
			open_render_node = 1;
			break;
		case '?':
		case 'h':
			fprintf(stderr, usage, argv[0]);
			exit(EXIT_SUCCESS);
		default:
			fprintf(stderr, usage, argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (amdgpu_open_devices(open_render_node) <= 0) {
		perror("Cannot open AMDGPU device");
		exit(EXIT_FAILURE);
	}

	if (drm_amdgpu[0] < 0) {
		perror("Cannot open AMDGPU device");
		exit(EXIT_FAILURE);
	}

	if (display_devices) {
		amdgpu_print_devices();
		amdgpu_close_devices();
		exit(EXIT_SUCCESS);
	}

	if (pci_bus_id > 0 || pci_device_id) {
		/* A device was specified to run the test */
		test_device_index = amdgpu_find_device(pci_bus_id,
						       pci_device_id);

		if (test_device_index >= 0) {
			/* Most tests run on device of drm_amdgpu[0].
			 * Swap the chosen device to drm_amdgpu[0].
			 */
			i = drm_amdgpu[0];
			drm_amdgpu[0] = drm_amdgpu[test_device_index];
			drm_amdgpu[test_device_index] = i;
		} else {
			fprintf(stderr,
				"The specified GPU device does not exist.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* Initialize test suites to run */

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry()) {
		amdgpu_close_devices();
		return CU_get_error();
	}

	/* Register suites. */
	if (CU_register_suites(suites) != CUE_SUCCESS) {
		fprintf(stderr, "suite registration failed - %s\n",
				CU_get_error_msg());
		CU_cleanup_registry();
		amdgpu_close_devices();
		exit(EXIT_FAILURE);
	}

	/* Run tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);

	if (suite_id != -1) {	/* If user specify particular suite? */
		pSuite = CU_get_suite_by_index((unsigned int) suite_id,
						CU_get_registry());

		if (pSuite) {
			if (test_id != -1) {   /* If user specify test id */
				pTest = CU_get_test_by_index(
						(unsigned int) test_id,
						pSuite);
				if (pTest)
					CU_basic_run_test(pSuite, pTest);
				else {
					fprintf(stderr, "Invalid test id: %d\n",
								test_id);
					CU_cleanup_registry();
					amdgpu_close_devices();
					exit(EXIT_FAILURE);
				}
			} else
				CU_basic_run_suite(pSuite);
		} else {
			fprintf(stderr, "Invalid suite id : %d\n",
					suite_id);
			CU_cleanup_registry();
			amdgpu_close_devices();
			exit(EXIT_FAILURE);
		}
	} else
		CU_basic_run_tests();

	CU_cleanup_registry();
	amdgpu_close_devices();
	return CU_get_error();
}
