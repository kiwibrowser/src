/*
 * Copyright (c) 2015 Emil Velikov <emil.l.velikov@gmail.com>
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>


static void
print_device_info(drmDevicePtr device, int i, bool print_revision)
{
    printf("device[%i]\n", i);
    printf("\tavailable_nodes %04x\n", device->available_nodes);
    printf("\tnodes\n");
    for (int j = 0; j < DRM_NODE_MAX; j++)
        if (device->available_nodes & 1 << j)
            printf("\t\tnodes[%d] %s\n", j, device->nodes[j]);

    printf("\tbustype %04x\n", device->bustype);
    printf("\tbusinfo\n");
    if (device->bustype == DRM_BUS_PCI) {
        printf("\t\tpci\n");
        printf("\t\t\tdomain\t%04x\n",device->businfo.pci->domain);
        printf("\t\t\tbus\t%02x\n", device->businfo.pci->bus);
        printf("\t\t\tdev\t%02x\n", device->businfo.pci->dev);
        printf("\t\t\tfunc\t%1u\n", device->businfo.pci->func);

        printf("\tdeviceinfo\n");
        printf("\t\tpci\n");
        printf("\t\t\tvendor_id\t%04x\n", device->deviceinfo.pci->vendor_id);
        printf("\t\t\tdevice_id\t%04x\n", device->deviceinfo.pci->device_id);
        printf("\t\t\tsubvendor_id\t%04x\n", device->deviceinfo.pci->subvendor_id);
        printf("\t\t\tsubdevice_id\t%04x\n", device->deviceinfo.pci->subdevice_id);
        if (print_revision)
            printf("\t\t\trevision_id\t%02x\n", device->deviceinfo.pci->revision_id);
        else
            printf("\t\t\trevision_id\tIGNORED\n");

    } else if (device->bustype == DRM_BUS_USB) {
        printf("\t\tusb\n");
        printf("\t\t\tbus\t%03u\n", device->businfo.usb->bus);
        printf("\t\t\tdev\t%03u\n", device->businfo.usb->dev);

        printf("\tdeviceinfo\n");
        printf("\t\tusb\n");
        printf("\t\t\tvendor\t%04x\n", device->deviceinfo.usb->vendor);
        printf("\t\t\tproduct\t%04x\n", device->deviceinfo.usb->product);
    } else if (device->bustype == DRM_BUS_PLATFORM) {
        char **compatible = device->deviceinfo.platform->compatible;

        printf("\t\tplatform\n");
        printf("\t\t\tfullname\t%s\n", device->businfo.platform->fullname);

        printf("\tdeviceinfo\n");
        printf("\t\tplatform\n");
        printf("\t\t\tcompatible\n");

        while (*compatible) {
            printf("\t\t\t\t%s\n", *compatible);
            compatible++;
        }
    } else if (device->bustype == DRM_BUS_HOST1X) {
        char **compatible = device->deviceinfo.platform->compatible;

        printf("\t\thost1x\n");
        printf("\t\t\tfullname\t%s\n", device->businfo.host1x->fullname);

        printf("\tdeviceinfo\n");
        printf("\t\tplatform\n");
        printf("\t\t\tcompatible\n");

        while (*compatible) {
            printf("\t\t\t\t%s\n", *compatible);
            compatible++;
        }
    } else {
        printf("Unknown/unhandled bustype\n");
    }
    printf("\n");
}

int
main(void)
{
    drmDevicePtr *devices;
    drmDevicePtr device;
    int fd, ret, max_devices;

    max_devices = drmGetDevices2(0, NULL, 0);

    if (max_devices <= 0) {
        printf("drmGetDevices2() has returned %d\n", max_devices);
        return -1;
    }

    devices = calloc(max_devices, sizeof(drmDevicePtr));
    if (devices == NULL) {
        printf("Failed to allocate memory for the drmDevicePtr array\n");
        return -1;
    }

    ret = drmGetDevices2(0, devices, max_devices);
    if (ret < 0) {
        printf("drmGetDevices2() returned an error %d\n", ret);
        free(devices);
        return -1;
    }

    for (int i = 0; i < ret; i++) {
        print_device_info(devices[i], i, false);

        for (int j = 0; j < DRM_NODE_MAX; j++) {
            if (devices[i]->available_nodes & 1 << j) {
                printf("Opening device %d node %s\n", i, devices[i]->nodes[j]);
                fd = open(devices[i]->nodes[j], O_RDONLY | O_CLOEXEC, 0);
                if (fd < 0) {
                    printf("Failed - %s (%d)\n", strerror(errno), errno);
                    continue;
                }

                if (drmGetDevice2(fd, DRM_DEVICE_GET_PCI_REVISION, &device) == 0) {
                    print_device_info(device, i, true);
                    drmFreeDevice(&device);
                }
                close(fd);
            }
        }
    }

    drmFreeDevices(devices, ret);
    free(devices);
    return 0;
}
