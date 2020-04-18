/**
 * \file xf86drm.c
 * User-level interface to DRM device
 *
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 * \author Kevin E. Martin <martin@valinux.com>
 */

/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <dirent.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#define stat_t struct stat
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdarg.h>
#ifdef MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#endif
#ifdef MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif
#include <math.h>

/* Not all systems have MAP_FAILED defined */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#include "xf86drm.h"
#include "libdrm_macros.h"

#include "util_math.h"

#ifdef __OpenBSD__
#define DRM_PRIMARY_MINOR_NAME  "drm"
#define DRM_CONTROL_MINOR_NAME  "drmC"
#define DRM_RENDER_MINOR_NAME   "drmR"
#else
#define DRM_PRIMARY_MINOR_NAME  "card"
#define DRM_CONTROL_MINOR_NAME  "controlD"
#define DRM_RENDER_MINOR_NAME   "renderD"
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#define DRM_MAJOR 145
#endif

#ifdef __NetBSD__
#define DRM_MAJOR 34
#endif

#ifdef __OpenBSD__
#ifdef __i386__
#define DRM_MAJOR 88
#else
#define DRM_MAJOR 87
#endif
#endif /* __OpenBSD__ */

#ifndef DRM_MAJOR
#define DRM_MAJOR 226 /* Linux */
#endif

#ifdef __OpenBSD__
struct drm_pciinfo {
	uint16_t	domain;
	uint8_t		bus;
	uint8_t		dev;
	uint8_t		func;
	uint16_t	vendor_id;
	uint16_t	device_id;
	uint16_t	subvendor_id;
	uint16_t	subdevice_id;
	uint8_t		revision_id;
};

#define DRM_IOCTL_GET_PCIINFO	DRM_IOR(0x15, struct drm_pciinfo)
#endif

#define DRM_MSG_VERBOSITY 3

#define memclear(s) memset(&s, 0, sizeof(s))

static drmServerInfoPtr drm_server_info;

void drmSetServerInfo(drmServerInfoPtr info)
{
    drm_server_info = info;
}

/**
 * Output a message to stderr.
 *
 * \param format printf() like format string.
 *
 * \internal
 * This function is a wrapper around vfprintf().
 */

static int DRM_PRINTFLIKE(1, 0)
drmDebugPrint(const char *format, va_list ap)
{
    return vfprintf(stderr, format, ap);
}

void
drmMsg(const char *format, ...)
{
    va_list ap;
    const char *env;
    if (((env = getenv("LIBGL_DEBUG")) && strstr(env, "verbose")) ||
        (drm_server_info && drm_server_info->debug_print))
    {
        va_start(ap, format);
        if (drm_server_info) {
            drm_server_info->debug_print(format,ap);
        } else {
            drmDebugPrint(format, ap);
        }
        va_end(ap);
    }
}

static void *drmHashTable = NULL; /* Context switch callbacks */

void *drmGetHashTable(void)
{
    return drmHashTable;
}

void *drmMalloc(int size)
{
    return calloc(1, size);
}

void drmFree(void *pt)
{
    free(pt);
}

/**
 * Call ioctl, restarting if it is interupted
 */
int
drmIoctl(int fd, unsigned long request, void *arg)
{
    int ret;

    do {
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

static unsigned long drmGetKeyFromFd(int fd)
{
    stat_t     st;

    st.st_rdev = 0;
    fstat(fd, &st);
    return st.st_rdev;
}

drmHashEntry *drmGetEntry(int fd)
{
    unsigned long key = drmGetKeyFromFd(fd);
    void          *value;
    drmHashEntry  *entry;

    if (!drmHashTable)
        drmHashTable = drmHashCreate();

    if (drmHashLookup(drmHashTable, key, &value)) {
        entry           = drmMalloc(sizeof(*entry));
        entry->fd       = fd;
        entry->f        = NULL;
        entry->tagTable = drmHashCreate();
        drmHashInsert(drmHashTable, key, entry);
    } else {
        entry = value;
    }
    return entry;
}

/**
 * Compare two busid strings
 *
 * \param first
 * \param second
 *
 * \return 1 if matched.
 *
 * \internal
 * This function compares two bus ID strings.  It understands the older
 * PCI:b:d:f format and the newer pci:oooo:bb:dd.f format.  In the format, o is
 * domain, b is bus, d is device, f is function.
 */
static int drmMatchBusID(const char *id1, const char *id2, int pci_domain_ok)
{
    /* First, check if the IDs are exactly the same */
    if (strcasecmp(id1, id2) == 0)
        return 1;

    /* Try to match old/new-style PCI bus IDs. */
    if (strncasecmp(id1, "pci", 3) == 0) {
        unsigned int o1, b1, d1, f1;
        unsigned int o2, b2, d2, f2;
        int ret;

        ret = sscanf(id1, "pci:%04x:%02x:%02x.%u", &o1, &b1, &d1, &f1);
        if (ret != 4) {
            o1 = 0;
            ret = sscanf(id1, "PCI:%u:%u:%u", &b1, &d1, &f1);
            if (ret != 3)
                return 0;
        }

        ret = sscanf(id2, "pci:%04x:%02x:%02x.%u", &o2, &b2, &d2, &f2);
        if (ret != 4) {
            o2 = 0;
            ret = sscanf(id2, "PCI:%u:%u:%u", &b2, &d2, &f2);
            if (ret != 3)
                return 0;
        }

        /* If domains aren't properly supported by the kernel interface,
         * just ignore them, which sucks less than picking a totally random
         * card with "open by name"
         */
        if (!pci_domain_ok)
            o1 = o2 = 0;

        if ((o1 != o2) || (b1 != b2) || (d1 != d2) || (f1 != f2))
            return 0;
        else
            return 1;
    }
    return 0;
}

/**
 * Handles error checking for chown call.
 *
 * \param path to file.
 * \param id of the new owner.
 * \param id of the new group.
 *
 * \return zero if success or -1 if failure.
 *
 * \internal
 * Checks for failure. If failure was caused by signal call chown again.
 * If any other failure happened then it will output error mesage using
 * drmMsg() call.
 */
#if !defined(UDEV)
static int chown_check_return(const char *path, uid_t owner, gid_t group)
{
        int rv;

        do {
            rv = chown(path, owner, group);
        } while (rv != 0 && errno == EINTR);

        if (rv == 0)
            return 0;

        drmMsg("Failed to change owner or group for file %s! %d: %s\n",
               path, errno, strerror(errno));
        return -1;
}
#endif

/**
 * Open the DRM device, creating it if necessary.
 *
 * \param dev major and minor numbers of the device.
 * \param minor minor number of the device.
 *
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * Assembles the device name from \p minor and opens it, creating the device
 * special file node with the major and minor numbers specified by \p dev and
 * parent directory if necessary and was called by root.
 */
static int drmOpenDevice(dev_t dev, int minor, int type)
{
    stat_t          st;
    const char      *dev_name;
    char            buf[64];
    int             fd;
    mode_t          devmode = DRM_DEV_MODE, serv_mode;
    gid_t           serv_group;
#if !defined(UDEV)
    int             isroot  = !geteuid();
    uid_t           user    = DRM_DEV_UID;
    gid_t           group   = DRM_DEV_GID;
#endif

    switch (type) {
    case DRM_NODE_PRIMARY:
        dev_name = DRM_DEV_NAME;
        break;
    case DRM_NODE_CONTROL:
        dev_name = DRM_CONTROL_DEV_NAME;
        break;
    case DRM_NODE_RENDER:
        dev_name = DRM_RENDER_DEV_NAME;
        break;
    default:
        return -EINVAL;
    };

    sprintf(buf, dev_name, DRM_DIR_NAME, minor);
    drmMsg("drmOpenDevice: node name is %s\n", buf);

    if (drm_server_info && drm_server_info->get_perms) {
        drm_server_info->get_perms(&serv_group, &serv_mode);
        devmode  = serv_mode ? serv_mode : DRM_DEV_MODE;
        devmode &= ~(S_IXUSR|S_IXGRP|S_IXOTH);
    }

#if !defined(UDEV)
    if (stat(DRM_DIR_NAME, &st)) {
        if (!isroot)
            return DRM_ERR_NOT_ROOT;
        mkdir(DRM_DIR_NAME, DRM_DEV_DIRMODE);
        chown_check_return(DRM_DIR_NAME, 0, 0); /* root:root */
        chmod(DRM_DIR_NAME, DRM_DEV_DIRMODE);
    }

    /* Check if the device node exists and create it if necessary. */
    if (stat(buf, &st)) {
        if (!isroot)
            return DRM_ERR_NOT_ROOT;
        remove(buf);
        mknod(buf, S_IFCHR | devmode, dev);
    }

    if (drm_server_info && drm_server_info->get_perms) {
        group = ((int)serv_group >= 0) ? serv_group : DRM_DEV_GID;
        chown_check_return(buf, user, group);
        chmod(buf, devmode);
    }
#else
    /* if we modprobed then wait for udev */
    {
        int udev_count = 0;
wait_for_udev:
        if (stat(DRM_DIR_NAME, &st)) {
            usleep(20);
            udev_count++;

            if (udev_count == 50)
                return -1;
            goto wait_for_udev;
        }

        if (stat(buf, &st)) {
            usleep(20);
            udev_count++;

            if (udev_count == 50)
                return -1;
            goto wait_for_udev;
        }
    }
#endif

    fd = open(buf, O_RDWR, 0);
    drmMsg("drmOpenDevice: open result is %d, (%s)\n",
           fd, fd < 0 ? strerror(errno) : "OK");
    if (fd >= 0)
        return fd;

#if !defined(UDEV)
    /* Check if the device node is not what we expect it to be, and recreate it
     * and try again if so.
     */
    if (st.st_rdev != dev) {
        if (!isroot)
            return DRM_ERR_NOT_ROOT;
        remove(buf);
        mknod(buf, S_IFCHR | devmode, dev);
        if (drm_server_info && drm_server_info->get_perms) {
            chown_check_return(buf, user, group);
            chmod(buf, devmode);
        }
    }
    fd = open(buf, O_RDWR, 0);
    drmMsg("drmOpenDevice: open result is %d, (%s)\n",
           fd, fd < 0 ? strerror(errno) : "OK");
    if (fd >= 0)
        return fd;

    drmMsg("drmOpenDevice: Open failed\n");
    remove(buf);
#endif
    return -errno;
}


/**
 * Open the DRM device
 *
 * \param minor device minor number.
 * \param create allow to create the device if set.
 *
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * Calls drmOpenDevice() if \p create is set, otherwise assembles the device
 * name from \p minor and opens it.
 */
static int drmOpenMinor(int minor, int create, int type)
{
    int  fd;
    char buf[64];
    const char *dev_name;

    if (create)
        return drmOpenDevice(makedev(DRM_MAJOR, minor), minor, type);

    switch (type) {
    case DRM_NODE_PRIMARY:
        dev_name = DRM_DEV_NAME;
        break;
    case DRM_NODE_CONTROL:
        dev_name = DRM_CONTROL_DEV_NAME;
        break;
    case DRM_NODE_RENDER:
        dev_name = DRM_RENDER_DEV_NAME;
        break;
    default:
        return -EINVAL;
    };

    sprintf(buf, dev_name, DRM_DIR_NAME, minor);
    if ((fd = open(buf, O_RDWR, 0)) >= 0)
        return fd;
    return -errno;
}


/**
 * Determine whether the DRM kernel driver has been loaded.
 *
 * \return 1 if the DRM driver is loaded, 0 otherwise.
 *
 * \internal
 * Determine the presence of the kernel driver by attempting to open the 0
 * minor and get version information.  For backward compatibility with older
 * Linux implementations, /proc/dri is also checked.
 */
int drmAvailable(void)
{
    drmVersionPtr version;
    int           retval = 0;
    int           fd;

    if ((fd = drmOpenMinor(0, 1, DRM_NODE_PRIMARY)) < 0) {
#ifdef __linux__
        /* Try proc for backward Linux compatibility */
        if (!access("/proc/dri/0", R_OK))
            return 1;
#endif
        return 0;
    }

    if ((version = drmGetVersion(fd))) {
        retval = 1;
        drmFreeVersion(version);
    }
    close(fd);

    return retval;
}

static int drmGetMinorBase(int type)
{
    switch (type) {
    case DRM_NODE_PRIMARY:
        return 0;
    case DRM_NODE_CONTROL:
        return 64;
    case DRM_NODE_RENDER:
        return 128;
    default:
        return -1;
    };
}

static int drmGetMinorType(int minor)
{
    int type = minor >> 6;

    if (minor < 0)
        return -1;

    switch (type) {
    case DRM_NODE_PRIMARY:
    case DRM_NODE_CONTROL:
    case DRM_NODE_RENDER:
        return type;
    default:
        return -1;
    }
}

static const char *drmGetMinorName(int type)
{
    switch (type) {
    case DRM_NODE_PRIMARY:
        return DRM_PRIMARY_MINOR_NAME;
    case DRM_NODE_CONTROL:
        return DRM_CONTROL_MINOR_NAME;
    case DRM_NODE_RENDER:
        return DRM_RENDER_MINOR_NAME;
    default:
        return NULL;
    }
}

/**
 * Open the device by bus ID.
 *
 * \param busid bus ID.
 * \param type device node type.
 *
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * This function attempts to open every possible minor (up to DRM_MAX_MINOR),
 * comparing the device bus ID with the one supplied.
 *
 * \sa drmOpenMinor() and drmGetBusid().
 */
static int drmOpenByBusid(const char *busid, int type)
{
    int        i, pci_domain_ok = 1;
    int        fd;
    const char *buf;
    drmSetVersion sv;
    int        base = drmGetMinorBase(type);

    if (base < 0)
        return -1;

    drmMsg("drmOpenByBusid: Searching for BusID %s\n", busid);
    for (i = base; i < base + DRM_MAX_MINOR; i++) {
        fd = drmOpenMinor(i, 1, type);
        drmMsg("drmOpenByBusid: drmOpenMinor returns %d\n", fd);
        if (fd >= 0) {
            /* We need to try for 1.4 first for proper PCI domain support
             * and if that fails, we know the kernel is busted
             */
            sv.drm_di_major = 1;
            sv.drm_di_minor = 4;
            sv.drm_dd_major = -1;        /* Don't care */
            sv.drm_dd_minor = -1;        /* Don't care */
            if (drmSetInterfaceVersion(fd, &sv)) {
#ifndef __alpha__
                pci_domain_ok = 0;
#endif
                sv.drm_di_major = 1;
                sv.drm_di_minor = 1;
                sv.drm_dd_major = -1;       /* Don't care */
                sv.drm_dd_minor = -1;       /* Don't care */
                drmMsg("drmOpenByBusid: Interface 1.4 failed, trying 1.1\n");
                drmSetInterfaceVersion(fd, &sv);
            }
            buf = drmGetBusid(fd);
            drmMsg("drmOpenByBusid: drmGetBusid reports %s\n", buf);
            if (buf && drmMatchBusID(buf, busid, pci_domain_ok)) {
                drmFreeBusid(buf);
                return fd;
            }
            if (buf)
                drmFreeBusid(buf);
            close(fd);
        }
    }
    return -1;
}


/**
 * Open the device by name.
 *
 * \param name driver name.
 * \param type the device node type.
 *
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * This function opens the first minor number that matches the driver name and
 * isn't already in use.  If it's in use it then it will already have a bus ID
 * assigned.
 *
 * \sa drmOpenMinor(), drmGetVersion() and drmGetBusid().
 */
static int drmOpenByName(const char *name, int type)
{
    int           i;
    int           fd;
    drmVersionPtr version;
    char *        id;
    int           base = drmGetMinorBase(type);

    if (base < 0)
        return -1;

    /*
     * Open the first minor number that matches the driver name and isn't
     * already in use.  If it's in use it will have a busid assigned already.
     */
    for (i = base; i < base + DRM_MAX_MINOR; i++) {
        if ((fd = drmOpenMinor(i, 1, type)) >= 0) {
            if ((version = drmGetVersion(fd))) {
                if (!strcmp(version->name, name)) {
                    drmFreeVersion(version);
                    id = drmGetBusid(fd);
                    drmMsg("drmGetBusid returned '%s'\n", id ? id : "NULL");
                    if (!id || !*id) {
                        if (id)
                            drmFreeBusid(id);
                        return fd;
                    } else {
                        drmFreeBusid(id);
                    }
                } else {
                    drmFreeVersion(version);
                }
            }
            close(fd);
        }
    }

#ifdef __linux__
    /* Backward-compatibility /proc support */
    for (i = 0; i < 8; i++) {
        char proc_name[64], buf[512];
        char *driver, *pt, *devstring;
        int  retcode;

        sprintf(proc_name, "/proc/dri/%d/name", i);
        if ((fd = open(proc_name, 0, 0)) >= 0) {
            retcode = read(fd, buf, sizeof(buf)-1);
            close(fd);
            if (retcode) {
                buf[retcode-1] = '\0';
                for (driver = pt = buf; *pt && *pt != ' '; ++pt)
                    ;
                if (*pt) { /* Device is next */
                    *pt = '\0';
                    if (!strcmp(driver, name)) { /* Match */
                        for (devstring = ++pt; *pt && *pt != ' '; ++pt)
                            ;
                        if (*pt) { /* Found busid */
                            return drmOpenByBusid(++pt, type);
                        } else { /* No busid */
                            return drmOpenDevice(strtol(devstring, NULL, 0),i, type);
                        }
                    }
                }
            }
        }
    }
#endif

    return -1;
}


/**
 * Open the DRM device.
 *
 * Looks up the specified name and bus ID, and opens the device found.  The
 * entry in /dev/dri is created if necessary and if called by root.
 *
 * \param name driver name. Not referenced if bus ID is supplied.
 * \param busid bus ID. Zero if not known.
 *
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * It calls drmOpenByBusid() if \p busid is specified or drmOpenByName()
 * otherwise.
 */
int drmOpen(const char *name, const char *busid)
{
    return drmOpenWithType(name, busid, DRM_NODE_PRIMARY);
}

/**
 * Open the DRM device with specified type.
 *
 * Looks up the specified name and bus ID, and opens the device found.  The
 * entry in /dev/dri is created if necessary and if called by root.
 *
 * \param name driver name. Not referenced if bus ID is supplied.
 * \param busid bus ID. Zero if not known.
 * \param type the device node type to open, PRIMARY, CONTROL or RENDER
 *
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * It calls drmOpenByBusid() if \p busid is specified or drmOpenByName()
 * otherwise.
 */
int drmOpenWithType(const char *name, const char *busid, int type)
{
    if (!drmAvailable() && name != NULL && drm_server_info &&
        drm_server_info->load_module) {
        /* try to load the kernel module */
        if (!drm_server_info->load_module(name)) {
            drmMsg("[drm] failed to load kernel module \"%s\"\n", name);
            return -1;
        }
    }

    if (busid) {
        int fd = drmOpenByBusid(busid, type);
        if (fd >= 0)
            return fd;
    }

    if (name)
        return drmOpenByName(name, type);

    return -1;
}

int drmOpenControl(int minor)
{
    return drmOpenMinor(minor, 0, DRM_NODE_CONTROL);
}

int drmOpenRender(int minor)
{
    return drmOpenMinor(minor, 0, DRM_NODE_RENDER);
}

/**
 * Free the version information returned by drmGetVersion().
 *
 * \param v pointer to the version information.
 *
 * \internal
 * It frees the memory pointed by \p %v as well as all the non-null strings
 * pointers in it.
 */
void drmFreeVersion(drmVersionPtr v)
{
    if (!v)
        return;
    drmFree(v->name);
    drmFree(v->date);
    drmFree(v->desc);
    drmFree(v);
}


/**
 * Free the non-public version information returned by the kernel.
 *
 * \param v pointer to the version information.
 *
 * \internal
 * Used by drmGetVersion() to free the memory pointed by \p %v as well as all
 * the non-null strings pointers in it.
 */
static void drmFreeKernelVersion(drm_version_t *v)
{
    if (!v)
        return;
    drmFree(v->name);
    drmFree(v->date);
    drmFree(v->desc);
    drmFree(v);
}


/**
 * Copy version information.
 *
 * \param d destination pointer.
 * \param s source pointer.
 *
 * \internal
 * Used by drmGetVersion() to translate the information returned by the ioctl
 * interface in a private structure into the public structure counterpart.
 */
static void drmCopyVersion(drmVersionPtr d, const drm_version_t *s)
{
    d->version_major      = s->version_major;
    d->version_minor      = s->version_minor;
    d->version_patchlevel = s->version_patchlevel;
    d->name_len           = s->name_len;
    d->name               = strdup(s->name);
    d->date_len           = s->date_len;
    d->date               = strdup(s->date);
    d->desc_len           = s->desc_len;
    d->desc               = strdup(s->desc);
}


/**
 * Query the driver version information.
 *
 * \param fd file descriptor.
 *
 * \return pointer to a drmVersion structure which should be freed with
 * drmFreeVersion().
 *
 * \note Similar information is available via /proc/dri.
 *
 * \internal
 * It gets the version information via successive DRM_IOCTL_VERSION ioctls,
 * first with zeros to get the string lengths, and then the actually strings.
 * It also null-terminates them since they might not be already.
 */
drmVersionPtr drmGetVersion(int fd)
{
    drmVersionPtr retval;
    drm_version_t *version = drmMalloc(sizeof(*version));

    if (drmIoctl(fd, DRM_IOCTL_VERSION, version)) {
        drmFreeKernelVersion(version);
        return NULL;
    }

    if (version->name_len)
        version->name    = drmMalloc(version->name_len + 1);
    if (version->date_len)
        version->date    = drmMalloc(version->date_len + 1);
    if (version->desc_len)
        version->desc    = drmMalloc(version->desc_len + 1);

    if (drmIoctl(fd, DRM_IOCTL_VERSION, version)) {
        drmMsg("DRM_IOCTL_VERSION: %s\n", strerror(errno));
        drmFreeKernelVersion(version);
        return NULL;
    }

    /* The results might not be null-terminated strings, so terminate them. */
    if (version->name_len) version->name[version->name_len] = '\0';
    if (version->date_len) version->date[version->date_len] = '\0';
    if (version->desc_len) version->desc[version->desc_len] = '\0';

    retval = drmMalloc(sizeof(*retval));
    drmCopyVersion(retval, version);
    drmFreeKernelVersion(version);
    return retval;
}


/**
 * Get version information for the DRM user space library.
 *
 * This version number is driver independent.
 *
 * \param fd file descriptor.
 *
 * \return version information.
 *
 * \internal
 * This function allocates and fills a drm_version structure with a hard coded
 * version number.
 */
drmVersionPtr drmGetLibVersion(int fd)
{
    drm_version_t *version = drmMalloc(sizeof(*version));

    /* Version history:
     *   NOTE THIS MUST NOT GO ABOVE VERSION 1.X due to drivers needing it
     *   revision 1.0.x = original DRM interface with no drmGetLibVersion
     *                    entry point and many drm<Device> extensions
     *   revision 1.1.x = added drmCommand entry points for device extensions
     *                    added drmGetLibVersion to identify libdrm.a version
     *   revision 1.2.x = added drmSetInterfaceVersion
     *                    modified drmOpen to handle both busid and name
     *   revision 1.3.x = added server + memory manager
     */
    version->version_major      = 1;
    version->version_minor      = 3;
    version->version_patchlevel = 0;

    return (drmVersionPtr)version;
}

int drmGetCap(int fd, uint64_t capability, uint64_t *value)
{
    struct drm_get_cap cap;
    int ret;

    memclear(cap);
    cap.capability = capability;

    ret = drmIoctl(fd, DRM_IOCTL_GET_CAP, &cap);
    if (ret)
        return ret;

    *value = cap.value;
    return 0;
}

int drmSetClientCap(int fd, uint64_t capability, uint64_t value)
{
    struct drm_set_client_cap cap;

    memclear(cap);
    cap.capability = capability;
    cap.value = value;

    return drmIoctl(fd, DRM_IOCTL_SET_CLIENT_CAP, &cap);
}

/**
 * Free the bus ID information.
 *
 * \param busid bus ID information string as given by drmGetBusid().
 *
 * \internal
 * This function is just frees the memory pointed by \p busid.
 */
void drmFreeBusid(const char *busid)
{
    drmFree((void *)busid);
}


/**
 * Get the bus ID of the device.
 *
 * \param fd file descriptor.
 *
 * \return bus ID string.
 *
 * \internal
 * This function gets the bus ID via successive DRM_IOCTL_GET_UNIQUE ioctls to
 * get the string length and data, passing the arguments in a drm_unique
 * structure.
 */
char *drmGetBusid(int fd)
{
    drm_unique_t u;

    memclear(u);

    if (drmIoctl(fd, DRM_IOCTL_GET_UNIQUE, &u))
        return NULL;
    u.unique = drmMalloc(u.unique_len + 1);
    if (drmIoctl(fd, DRM_IOCTL_GET_UNIQUE, &u)) {
        drmFree(u.unique);
        return NULL;
    }
    u.unique[u.unique_len] = '\0';

    return u.unique;
}


/**
 * Set the bus ID of the device.
 *
 * \param fd file descriptor.
 * \param busid bus ID string.
 *
 * \return zero on success, negative on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_SET_UNIQUE ioctl, passing
 * the arguments in a drm_unique structure.
 */
int drmSetBusid(int fd, const char *busid)
{
    drm_unique_t u;

    memclear(u);
    u.unique     = (char *)busid;
    u.unique_len = strlen(busid);

    if (drmIoctl(fd, DRM_IOCTL_SET_UNIQUE, &u)) {
        return -errno;
    }
    return 0;
}

int drmGetMagic(int fd, drm_magic_t * magic)
{
    drm_auth_t auth;

    memclear(auth);

    *magic = 0;
    if (drmIoctl(fd, DRM_IOCTL_GET_MAGIC, &auth))
        return -errno;
    *magic = auth.magic;
    return 0;
}

int drmAuthMagic(int fd, drm_magic_t magic)
{
    drm_auth_t auth;

    memclear(auth);
    auth.magic = magic;
    if (drmIoctl(fd, DRM_IOCTL_AUTH_MAGIC, &auth))
        return -errno;
    return 0;
}

/**
 * Specifies a range of memory that is available for mapping by a
 * non-root process.
 *
 * \param fd file descriptor.
 * \param offset usually the physical address. The actual meaning depends of
 * the \p type parameter. See below.
 * \param size of the memory in bytes.
 * \param type type of the memory to be mapped.
 * \param flags combination of several flags to modify the function actions.
 * \param handle will be set to a value that may be used as the offset
 * parameter for mmap().
 *
 * \return zero on success or a negative value on error.
 *
 * \par Mapping the frame buffer
 * For the frame buffer
 * - \p offset will be the physical address of the start of the frame buffer,
 * - \p size will be the size of the frame buffer in bytes, and
 * - \p type will be DRM_FRAME_BUFFER.
 *
 * \par
 * The area mapped will be uncached. If MTRR support is available in the
 * kernel, the frame buffer area will be set to write combining.
 *
 * \par Mapping the MMIO register area
 * For the MMIO register area,
 * - \p offset will be the physical address of the start of the register area,
 * - \p size will be the size of the register area bytes, and
 * - \p type will be DRM_REGISTERS.
 * \par
 * The area mapped will be uncached.
 *
 * \par Mapping the SAREA
 * For the SAREA,
 * - \p offset will be ignored and should be set to zero,
 * - \p size will be the desired size of the SAREA in bytes,
 * - \p type will be DRM_SHM.
 *
 * \par
 * A shared memory area of the requested size will be created and locked in
 * kernel memory. This area may be mapped into client-space by using the handle
 * returned.
 *
 * \note May only be called by root.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_ADD_MAP ioctl, passing
 * the arguments in a drm_map structure.
 */
int drmAddMap(int fd, drm_handle_t offset, drmSize size, drmMapType type,
              drmMapFlags flags, drm_handle_t *handle)
{
    drm_map_t map;

    memclear(map);
    map.offset  = offset;
    map.size    = size;
    map.type    = type;
    map.flags   = flags;
    if (drmIoctl(fd, DRM_IOCTL_ADD_MAP, &map))
        return -errno;
    if (handle)
        *handle = (drm_handle_t)(uintptr_t)map.handle;
    return 0;
}

int drmRmMap(int fd, drm_handle_t handle)
{
    drm_map_t map;

    memclear(map);
    map.handle = (void *)(uintptr_t)handle;

    if(drmIoctl(fd, DRM_IOCTL_RM_MAP, &map))
        return -errno;
    return 0;
}

/**
 * Make buffers available for DMA transfers.
 *
 * \param fd file descriptor.
 * \param count number of buffers.
 * \param size size of each buffer.
 * \param flags buffer allocation flags.
 * \param agp_offset offset in the AGP aperture
 *
 * \return number of buffers allocated, negative on error.
 *
 * \internal
 * This function is a wrapper around DRM_IOCTL_ADD_BUFS ioctl.
 *
 * \sa drm_buf_desc.
 */
int drmAddBufs(int fd, int count, int size, drmBufDescFlags flags,
               int agp_offset)
{
    drm_buf_desc_t request;

    memclear(request);
    request.count     = count;
    request.size      = size;
    request.flags     = flags;
    request.agp_start = agp_offset;

    if (drmIoctl(fd, DRM_IOCTL_ADD_BUFS, &request))
        return -errno;
    return request.count;
}

int drmMarkBufs(int fd, double low, double high)
{
    drm_buf_info_t info;
    int            i;

    memclear(info);

    if (drmIoctl(fd, DRM_IOCTL_INFO_BUFS, &info))
        return -EINVAL;

    if (!info.count)
        return -EINVAL;

    if (!(info.list = drmMalloc(info.count * sizeof(*info.list))))
        return -ENOMEM;

    if (drmIoctl(fd, DRM_IOCTL_INFO_BUFS, &info)) {
        int retval = -errno;
        drmFree(info.list);
        return retval;
    }

    for (i = 0; i < info.count; i++) {
        info.list[i].low_mark  = low  * info.list[i].count;
        info.list[i].high_mark = high * info.list[i].count;
        if (drmIoctl(fd, DRM_IOCTL_MARK_BUFS, &info.list[i])) {
            int retval = -errno;
            drmFree(info.list);
            return retval;
        }
    }
    drmFree(info.list);

    return 0;
}

/**
 * Free buffers.
 *
 * \param fd file descriptor.
 * \param count number of buffers to free.
 * \param list list of buffers to be freed.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \note This function is primarily used for debugging.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_FREE_BUFS ioctl, passing
 * the arguments in a drm_buf_free structure.
 */
int drmFreeBufs(int fd, int count, int *list)
{
    drm_buf_free_t request;

    memclear(request);
    request.count = count;
    request.list  = list;
    if (drmIoctl(fd, DRM_IOCTL_FREE_BUFS, &request))
        return -errno;
    return 0;
}


/**
 * Close the device.
 *
 * \param fd file descriptor.
 *
 * \internal
 * This function closes the file descriptor.
 */
int drmClose(int fd)
{
    unsigned long key    = drmGetKeyFromFd(fd);
    drmHashEntry  *entry = drmGetEntry(fd);

    drmHashDestroy(entry->tagTable);
    entry->fd       = 0;
    entry->f        = NULL;
    entry->tagTable = NULL;

    drmHashDelete(drmHashTable, key);
    drmFree(entry);

    return close(fd);
}


/**
 * Map a region of memory.
 *
 * \param fd file descriptor.
 * \param handle handle returned by drmAddMap().
 * \param size size in bytes. Must match the size used by drmAddMap().
 * \param address will contain the user-space virtual address where the mapping
 * begins.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper for mmap().
 */
int drmMap(int fd, drm_handle_t handle, drmSize size, drmAddressPtr address)
{
    static unsigned long pagesize_mask = 0;

    if (fd < 0)
        return -EINVAL;

    if (!pagesize_mask)
        pagesize_mask = getpagesize() - 1;

    size = (size + pagesize_mask) & ~pagesize_mask;

    *address = drm_mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, handle);
    if (*address == MAP_FAILED)
        return -errno;
    return 0;
}


/**
 * Unmap mappings obtained with drmMap().
 *
 * \param address address as given by drmMap().
 * \param size size in bytes. Must match the size used by drmMap().
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper for munmap().
 */
int drmUnmap(drmAddress address, drmSize size)
{
    return drm_munmap(address, size);
}

drmBufInfoPtr drmGetBufInfo(int fd)
{
    drm_buf_info_t info;
    drmBufInfoPtr  retval;
    int            i;

    memclear(info);

    if (drmIoctl(fd, DRM_IOCTL_INFO_BUFS, &info))
        return NULL;

    if (info.count) {
        if (!(info.list = drmMalloc(info.count * sizeof(*info.list))))
            return NULL;

        if (drmIoctl(fd, DRM_IOCTL_INFO_BUFS, &info)) {
            drmFree(info.list);
            return NULL;
        }

        retval = drmMalloc(sizeof(*retval));
        retval->count = info.count;
        retval->list  = drmMalloc(info.count * sizeof(*retval->list));
        for (i = 0; i < info.count; i++) {
            retval->list[i].count     = info.list[i].count;
            retval->list[i].size      = info.list[i].size;
            retval->list[i].low_mark  = info.list[i].low_mark;
            retval->list[i].high_mark = info.list[i].high_mark;
        }
        drmFree(info.list);
        return retval;
    }
    return NULL;
}

/**
 * Map all DMA buffers into client-virtual space.
 *
 * \param fd file descriptor.
 *
 * \return a pointer to a ::drmBufMap structure.
 *
 * \note The client may not use these buffers until obtaining buffer indices
 * with drmDMA().
 *
 * \internal
 * This function calls the DRM_IOCTL_MAP_BUFS ioctl and copies the returned
 * information about the buffers in a drm_buf_map structure into the
 * client-visible data structures.
 */
drmBufMapPtr drmMapBufs(int fd)
{
    drm_buf_map_t bufs;
    drmBufMapPtr  retval;
    int           i;

    memclear(bufs);
    if (drmIoctl(fd, DRM_IOCTL_MAP_BUFS, &bufs))
        return NULL;

    if (!bufs.count)
        return NULL;

    if (!(bufs.list = drmMalloc(bufs.count * sizeof(*bufs.list))))
        return NULL;

    if (drmIoctl(fd, DRM_IOCTL_MAP_BUFS, &bufs)) {
        drmFree(bufs.list);
        return NULL;
    }

    retval = drmMalloc(sizeof(*retval));
    retval->count = bufs.count;
    retval->list  = drmMalloc(bufs.count * sizeof(*retval->list));
    for (i = 0; i < bufs.count; i++) {
        retval->list[i].idx     = bufs.list[i].idx;
        retval->list[i].total   = bufs.list[i].total;
        retval->list[i].used    = 0;
        retval->list[i].address = bufs.list[i].address;
    }

    drmFree(bufs.list);
    return retval;
}


/**
 * Unmap buffers allocated with drmMapBufs().
 *
 * \return zero on success, or negative value on failure.
 *
 * \internal
 * Calls munmap() for every buffer stored in \p bufs and frees the
 * memory allocated by drmMapBufs().
 */
int drmUnmapBufs(drmBufMapPtr bufs)
{
    int i;

    for (i = 0; i < bufs->count; i++) {
        drm_munmap(bufs->list[i].address, bufs->list[i].total);
    }

    drmFree(bufs->list);
    drmFree(bufs);
    return 0;
}


#define DRM_DMA_RETRY  16

/**
 * Reserve DMA buffers.
 *
 * \param fd file descriptor.
 * \param request
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * Assemble the arguments into a drm_dma structure and keeps issuing the
 * DRM_IOCTL_DMA ioctl until success or until maximum number of retries.
 */
int drmDMA(int fd, drmDMAReqPtr request)
{
    drm_dma_t dma;
    int ret, i = 0;

    dma.context         = request->context;
    dma.send_count      = request->send_count;
    dma.send_indices    = request->send_list;
    dma.send_sizes      = request->send_sizes;
    dma.flags           = request->flags;
    dma.request_count   = request->request_count;
    dma.request_size    = request->request_size;
    dma.request_indices = request->request_list;
    dma.request_sizes   = request->request_sizes;
    dma.granted_count   = 0;

    do {
        ret = ioctl( fd, DRM_IOCTL_DMA, &dma );
    } while ( ret && errno == EAGAIN && i++ < DRM_DMA_RETRY );

    if ( ret == 0 ) {
        request->granted_count = dma.granted_count;
        return 0;
    } else {
        return -errno;
    }
}


/**
 * Obtain heavyweight hardware lock.
 *
 * \param fd file descriptor.
 * \param context context.
 * \param flags flags that determine the sate of the hardware when the function
 * returns.
 *
 * \return always zero.
 *
 * \internal
 * This function translates the arguments into a drm_lock structure and issue
 * the DRM_IOCTL_LOCK ioctl until the lock is successfully acquired.
 */
int drmGetLock(int fd, drm_context_t context, drmLockFlags flags)
{
    drm_lock_t lock;

    memclear(lock);
    lock.context = context;
    lock.flags   = 0;
    if (flags & DRM_LOCK_READY)      lock.flags |= _DRM_LOCK_READY;
    if (flags & DRM_LOCK_QUIESCENT)  lock.flags |= _DRM_LOCK_QUIESCENT;
    if (flags & DRM_LOCK_FLUSH)      lock.flags |= _DRM_LOCK_FLUSH;
    if (flags & DRM_LOCK_FLUSH_ALL)  lock.flags |= _DRM_LOCK_FLUSH_ALL;
    if (flags & DRM_HALT_ALL_QUEUES) lock.flags |= _DRM_HALT_ALL_QUEUES;
    if (flags & DRM_HALT_CUR_QUEUES) lock.flags |= _DRM_HALT_CUR_QUEUES;

    while (drmIoctl(fd, DRM_IOCTL_LOCK, &lock))
        ;
    return 0;
}

/**
 * Release the hardware lock.
 *
 * \param fd file descriptor.
 * \param context context.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_UNLOCK ioctl, passing the
 * argument in a drm_lock structure.
 */
int drmUnlock(int fd, drm_context_t context)
{
    drm_lock_t lock;

    memclear(lock);
    lock.context = context;
    return drmIoctl(fd, DRM_IOCTL_UNLOCK, &lock);
}

drm_context_t *drmGetReservedContextList(int fd, int *count)
{
    drm_ctx_res_t res;
    drm_ctx_t     *list;
    drm_context_t * retval;
    int           i;

    memclear(res);
    if (drmIoctl(fd, DRM_IOCTL_RES_CTX, &res))
        return NULL;

    if (!res.count)
        return NULL;

    if (!(list   = drmMalloc(res.count * sizeof(*list))))
        return NULL;
    if (!(retval = drmMalloc(res.count * sizeof(*retval))))
        goto err_free_list;

    res.contexts = list;
    if (drmIoctl(fd, DRM_IOCTL_RES_CTX, &res))
        goto err_free_context;

    for (i = 0; i < res.count; i++)
        retval[i] = list[i].handle;
    drmFree(list);

    *count = res.count;
    return retval;

err_free_list:
    drmFree(list);
err_free_context:
    drmFree(retval);
    return NULL;
}

void drmFreeReservedContextList(drm_context_t *pt)
{
    drmFree(pt);
}

/**
 * Create context.
 *
 * Used by the X server during GLXContext initialization. This causes
 * per-context kernel-level resources to be allocated.
 *
 * \param fd file descriptor.
 * \param handle is set on success. To be used by the client when requesting DMA
 * dispatch with drmDMA().
 *
 * \return zero on success, or a negative value on failure.
 *
 * \note May only be called by root.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_ADD_CTX ioctl, passing the
 * argument in a drm_ctx structure.
 */
int drmCreateContext(int fd, drm_context_t *handle)
{
    drm_ctx_t ctx;

    memclear(ctx);
    if (drmIoctl(fd, DRM_IOCTL_ADD_CTX, &ctx))
        return -errno;
    *handle = ctx.handle;
    return 0;
}

int drmSwitchToContext(int fd, drm_context_t context)
{
    drm_ctx_t ctx;

    memclear(ctx);
    ctx.handle = context;
    if (drmIoctl(fd, DRM_IOCTL_SWITCH_CTX, &ctx))
        return -errno;
    return 0;
}

int drmSetContextFlags(int fd, drm_context_t context, drm_context_tFlags flags)
{
    drm_ctx_t ctx;

    /*
     * Context preserving means that no context switches are done between DMA
     * buffers from one context and the next.  This is suitable for use in the
     * X server (which promises to maintain hardware context), or in the
     * client-side library when buffers are swapped on behalf of two threads.
     */
    memclear(ctx);
    ctx.handle = context;
    if (flags & DRM_CONTEXT_PRESERVED)
        ctx.flags |= _DRM_CONTEXT_PRESERVED;
    if (flags & DRM_CONTEXT_2DONLY)
        ctx.flags |= _DRM_CONTEXT_2DONLY;
    if (drmIoctl(fd, DRM_IOCTL_MOD_CTX, &ctx))
        return -errno;
    return 0;
}

int drmGetContextFlags(int fd, drm_context_t context,
                       drm_context_tFlagsPtr flags)
{
    drm_ctx_t ctx;

    memclear(ctx);
    ctx.handle = context;
    if (drmIoctl(fd, DRM_IOCTL_GET_CTX, &ctx))
        return -errno;
    *flags = 0;
    if (ctx.flags & _DRM_CONTEXT_PRESERVED)
        *flags |= DRM_CONTEXT_PRESERVED;
    if (ctx.flags & _DRM_CONTEXT_2DONLY)
        *flags |= DRM_CONTEXT_2DONLY;
    return 0;
}

/**
 * Destroy context.
 *
 * Free any kernel-level resources allocated with drmCreateContext() associated
 * with the context.
 *
 * \param fd file descriptor.
 * \param handle handle given by drmCreateContext().
 *
 * \return zero on success, or a negative value on failure.
 *
 * \note May only be called by root.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_RM_CTX ioctl, passing the
 * argument in a drm_ctx structure.
 */
int drmDestroyContext(int fd, drm_context_t handle)
{
    drm_ctx_t ctx;

    memclear(ctx);
    ctx.handle = handle;
    if (drmIoctl(fd, DRM_IOCTL_RM_CTX, &ctx))
        return -errno;
    return 0;
}

int drmCreateDrawable(int fd, drm_drawable_t *handle)
{
    drm_draw_t draw;

    memclear(draw);
    if (drmIoctl(fd, DRM_IOCTL_ADD_DRAW, &draw))
        return -errno;
    *handle = draw.handle;
    return 0;
}

int drmDestroyDrawable(int fd, drm_drawable_t handle)
{
    drm_draw_t draw;

    memclear(draw);
    draw.handle = handle;
    if (drmIoctl(fd, DRM_IOCTL_RM_DRAW, &draw))
        return -errno;
    return 0;
}

int drmUpdateDrawableInfo(int fd, drm_drawable_t handle,
                          drm_drawable_info_type_t type, unsigned int num,
                          void *data)
{
    drm_update_draw_t update;

    memclear(update);
    update.handle = handle;
    update.type = type;
    update.num = num;
    update.data = (unsigned long long)(unsigned long)data;

    if (drmIoctl(fd, DRM_IOCTL_UPDATE_DRAW, &update))
        return -errno;

    return 0;
}

/**
 * Acquire the AGP device.
 *
 * Must be called before any of the other AGP related calls.
 *
 * \param fd file descriptor.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ACQUIRE ioctl.
 */
int drmAgpAcquire(int fd)
{
    if (drmIoctl(fd, DRM_IOCTL_AGP_ACQUIRE, NULL))
        return -errno;
    return 0;
}


/**
 * Release the AGP device.
 *
 * \param fd file descriptor.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_RELEASE ioctl.
 */
int drmAgpRelease(int fd)
{
    if (drmIoctl(fd, DRM_IOCTL_AGP_RELEASE, NULL))
        return -errno;
    return 0;
}


/**
 * Set the AGP mode.
 *
 * \param fd file descriptor.
 * \param mode AGP mode.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ENABLE ioctl, passing the
 * argument in a drm_agp_mode structure.
 */
int drmAgpEnable(int fd, unsigned long mode)
{
    drm_agp_mode_t m;

    memclear(m);
    m.mode = mode;
    if (drmIoctl(fd, DRM_IOCTL_AGP_ENABLE, &m))
        return -errno;
    return 0;
}


/**
 * Allocate a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param size requested memory size in bytes. Will be rounded to page boundary.
 * \param type type of memory to allocate.
 * \param address if not zero, will be set to the physical address of the
 * allocated memory.
 * \param handle on success will be set to a handle of the allocated memory.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ALLOC ioctl, passing the
 * arguments in a drm_agp_buffer structure.
 */
int drmAgpAlloc(int fd, unsigned long size, unsigned long type,
                unsigned long *address, drm_handle_t *handle)
{
    drm_agp_buffer_t b;

    memclear(b);
    *handle = DRM_AGP_NO_HANDLE;
    b.size   = size;
    b.type   = type;
    if (drmIoctl(fd, DRM_IOCTL_AGP_ALLOC, &b))
        return -errno;
    if (address != 0UL)
        *address = b.physical;
    *handle = b.handle;
    return 0;
}


/**
 * Free a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_FREE ioctl, passing the
 * argument in a drm_agp_buffer structure.
 */
int drmAgpFree(int fd, drm_handle_t handle)
{
    drm_agp_buffer_t b;

    memclear(b);
    b.handle = handle;
    if (drmIoctl(fd, DRM_IOCTL_AGP_FREE, &b))
        return -errno;
    return 0;
}


/**
 * Bind a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 * \param offset offset in bytes. It will round to page boundary.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_BIND ioctl, passing the
 * argument in a drm_agp_binding structure.
 */
int drmAgpBind(int fd, drm_handle_t handle, unsigned long offset)
{
    drm_agp_binding_t b;

    memclear(b);
    b.handle = handle;
    b.offset = offset;
    if (drmIoctl(fd, DRM_IOCTL_AGP_BIND, &b))
        return -errno;
    return 0;
}


/**
 * Unbind a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_UNBIND ioctl, passing
 * the argument in a drm_agp_binding structure.
 */
int drmAgpUnbind(int fd, drm_handle_t handle)
{
    drm_agp_binding_t b;

    memclear(b);
    b.handle = handle;
    if (drmIoctl(fd, DRM_IOCTL_AGP_UNBIND, &b))
        return -errno;
    return 0;
}


/**
 * Get AGP driver major version number.
 *
 * \param fd file descriptor.
 *
 * \return major version number on success, or a negative value on failure..
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
int drmAgpVersionMajor(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return -errno;
    return i.agp_version_major;
}


/**
 * Get AGP driver minor version number.
 *
 * \param fd file descriptor.
 *
 * \return minor version number on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
int drmAgpVersionMinor(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return -errno;
    return i.agp_version_minor;
}


/**
 * Get AGP mode.
 *
 * \param fd file descriptor.
 *
 * \return mode on success, or zero on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpGetMode(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return 0;
    return i.mode;
}


/**
 * Get AGP aperture base.
 *
 * \param fd file descriptor.
 *
 * \return aperture base on success, zero on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpBase(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return 0;
    return i.aperture_base;
}


/**
 * Get AGP aperture size.
 *
 * \param fd file descriptor.
 *
 * \return aperture size on success, zero on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpSize(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return 0;
    return i.aperture_size;
}


/**
 * Get used AGP memory.
 *
 * \param fd file descriptor.
 *
 * \return memory used on success, or zero on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpMemoryUsed(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return 0;
    return i.memory_used;
}


/**
 * Get available AGP memory.
 *
 * \param fd file descriptor.
 *
 * \return memory available on success, or zero on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpMemoryAvail(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return 0;
    return i.memory_allowed;
}


/**
 * Get hardware vendor ID.
 *
 * \param fd file descriptor.
 *
 * \return vendor ID on success, or zero on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned int drmAgpVendorId(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return 0;
    return i.id_vendor;
}


/**
 * Get hardware device ID.
 *
 * \param fd file descriptor.
 *
 * \return zero on success, or zero on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned int drmAgpDeviceId(int fd)
{
    drm_agp_info_t i;

    memclear(i);

    if (drmIoctl(fd, DRM_IOCTL_AGP_INFO, &i))
        return 0;
    return i.id_device;
}

int drmScatterGatherAlloc(int fd, unsigned long size, drm_handle_t *handle)
{
    drm_scatter_gather_t sg;

    memclear(sg);

    *handle = 0;
    sg.size   = size;
    if (drmIoctl(fd, DRM_IOCTL_SG_ALLOC, &sg))
        return -errno;
    *handle = sg.handle;
    return 0;
}

int drmScatterGatherFree(int fd, drm_handle_t handle)
{
    drm_scatter_gather_t sg;

    memclear(sg);
    sg.handle = handle;
    if (drmIoctl(fd, DRM_IOCTL_SG_FREE, &sg))
        return -errno;
    return 0;
}

/**
 * Wait for VBLANK.
 *
 * \param fd file descriptor.
 * \param vbl pointer to a drmVBlank structure.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_WAIT_VBLANK ioctl.
 */
int drmWaitVBlank(int fd, drmVBlankPtr vbl)
{
    struct timespec timeout, cur;
    int ret;

    ret = clock_gettime(CLOCK_MONOTONIC, &timeout);
    if (ret < 0) {
        fprintf(stderr, "clock_gettime failed: %s\n", strerror(errno));
        goto out;
    }
    timeout.tv_sec++;

    do {
       ret = ioctl(fd, DRM_IOCTL_WAIT_VBLANK, vbl);
       vbl->request.type &= ~DRM_VBLANK_RELATIVE;
       if (ret && errno == EINTR) {
           clock_gettime(CLOCK_MONOTONIC, &cur);
           /* Timeout after 1s */
           if (cur.tv_sec > timeout.tv_sec + 1 ||
               (cur.tv_sec == timeout.tv_sec && cur.tv_nsec >=
                timeout.tv_nsec)) {
                   errno = EBUSY;
                   ret = -1;
                   break;
           }
       }
    } while (ret && errno == EINTR);

out:
    return ret;
}

int drmError(int err, const char *label)
{
    switch (err) {
    case DRM_ERR_NO_DEVICE:
        fprintf(stderr, "%s: no device\n", label);
        break;
    case DRM_ERR_NO_ACCESS:
        fprintf(stderr, "%s: no access\n", label);
        break;
    case DRM_ERR_NOT_ROOT:
        fprintf(stderr, "%s: not root\n", label);
        break;
    case DRM_ERR_INVALID:
        fprintf(stderr, "%s: invalid args\n", label);
        break;
    default:
        if (err < 0)
            err = -err;
        fprintf( stderr, "%s: error %d (%s)\n", label, err, strerror(err) );
        break;
    }

    return 1;
}

/**
 * Install IRQ handler.
 *
 * \param fd file descriptor.
 * \param irq IRQ number.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_CONTROL ioctl, passing the
 * argument in a drm_control structure.
 */
int drmCtlInstHandler(int fd, int irq)
{
    drm_control_t ctl;

    memclear(ctl);
    ctl.func  = DRM_INST_HANDLER;
    ctl.irq   = irq;
    if (drmIoctl(fd, DRM_IOCTL_CONTROL, &ctl))
        return -errno;
    return 0;
}


/**
 * Uninstall IRQ handler.
 *
 * \param fd file descriptor.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_CONTROL ioctl, passing the
 * argument in a drm_control structure.
 */
int drmCtlUninstHandler(int fd)
{
    drm_control_t ctl;

    memclear(ctl);
    ctl.func  = DRM_UNINST_HANDLER;
    ctl.irq   = 0;
    if (drmIoctl(fd, DRM_IOCTL_CONTROL, &ctl))
        return -errno;
    return 0;
}

int drmFinish(int fd, int context, drmLockFlags flags)
{
    drm_lock_t lock;

    memclear(lock);
    lock.context = context;
    if (flags & DRM_LOCK_READY)      lock.flags |= _DRM_LOCK_READY;
    if (flags & DRM_LOCK_QUIESCENT)  lock.flags |= _DRM_LOCK_QUIESCENT;
    if (flags & DRM_LOCK_FLUSH)      lock.flags |= _DRM_LOCK_FLUSH;
    if (flags & DRM_LOCK_FLUSH_ALL)  lock.flags |= _DRM_LOCK_FLUSH_ALL;
    if (flags & DRM_HALT_ALL_QUEUES) lock.flags |= _DRM_HALT_ALL_QUEUES;
    if (flags & DRM_HALT_CUR_QUEUES) lock.flags |= _DRM_HALT_CUR_QUEUES;
    if (drmIoctl(fd, DRM_IOCTL_FINISH, &lock))
        return -errno;
    return 0;
}

/**
 * Get IRQ from bus ID.
 *
 * \param fd file descriptor.
 * \param busnum bus number.
 * \param devnum device number.
 * \param funcnum function number.
 *
 * \return IRQ number on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_IRQ_BUSID ioctl, passing the
 * arguments in a drm_irq_busid structure.
 */
int drmGetInterruptFromBusID(int fd, int busnum, int devnum, int funcnum)
{
    drm_irq_busid_t p;

    memclear(p);
    p.busnum  = busnum;
    p.devnum  = devnum;
    p.funcnum = funcnum;
    if (drmIoctl(fd, DRM_IOCTL_IRQ_BUSID, &p))
        return -errno;
    return p.irq;
}

int drmAddContextTag(int fd, drm_context_t context, void *tag)
{
    drmHashEntry  *entry = drmGetEntry(fd);

    if (drmHashInsert(entry->tagTable, context, tag)) {
        drmHashDelete(entry->tagTable, context);
        drmHashInsert(entry->tagTable, context, tag);
    }
    return 0;
}

int drmDelContextTag(int fd, drm_context_t context)
{
    drmHashEntry  *entry = drmGetEntry(fd);

    return drmHashDelete(entry->tagTable, context);
}

void *drmGetContextTag(int fd, drm_context_t context)
{
    drmHashEntry  *entry = drmGetEntry(fd);
    void          *value;

    if (drmHashLookup(entry->tagTable, context, &value))
        return NULL;

    return value;
}

int drmAddContextPrivateMapping(int fd, drm_context_t ctx_id,
                                drm_handle_t handle)
{
    drm_ctx_priv_map_t map;

    memclear(map);
    map.ctx_id = ctx_id;
    map.handle = (void *)(uintptr_t)handle;

    if (drmIoctl(fd, DRM_IOCTL_SET_SAREA_CTX, &map))
        return -errno;
    return 0;
}

int drmGetContextPrivateMapping(int fd, drm_context_t ctx_id,
                                drm_handle_t *handle)
{
    drm_ctx_priv_map_t map;

    memclear(map);
    map.ctx_id = ctx_id;

    if (drmIoctl(fd, DRM_IOCTL_GET_SAREA_CTX, &map))
        return -errno;
    if (handle)
        *handle = (drm_handle_t)(uintptr_t)map.handle;

    return 0;
}

int drmGetMap(int fd, int idx, drm_handle_t *offset, drmSize *size,
              drmMapType *type, drmMapFlags *flags, drm_handle_t *handle,
              int *mtrr)
{
    drm_map_t map;

    memclear(map);
    map.offset = idx;
    if (drmIoctl(fd, DRM_IOCTL_GET_MAP, &map))
        return -errno;
    *offset = map.offset;
    *size   = map.size;
    *type   = map.type;
    *flags  = map.flags;
    *handle = (unsigned long)map.handle;
    *mtrr   = map.mtrr;
    return 0;
}

int drmGetClient(int fd, int idx, int *auth, int *pid, int *uid,
                 unsigned long *magic, unsigned long *iocs)
{
    drm_client_t client;

    memclear(client);
    client.idx = idx;
    if (drmIoctl(fd, DRM_IOCTL_GET_CLIENT, &client))
        return -errno;
    *auth      = client.auth;
    *pid       = client.pid;
    *uid       = client.uid;
    *magic     = client.magic;
    *iocs      = client.iocs;
    return 0;
}

int drmGetStats(int fd, drmStatsT *stats)
{
    drm_stats_t s;
    unsigned    i;

    memclear(s);
    if (drmIoctl(fd, DRM_IOCTL_GET_STATS, &s))
        return -errno;

    stats->count = 0;
    memset(stats, 0, sizeof(*stats));
    if (s.count > sizeof(stats->data)/sizeof(stats->data[0]))
        return -1;

#define SET_VALUE                              \
    stats->data[i].long_format = "%-20.20s";   \
    stats->data[i].rate_format = "%8.8s";      \
    stats->data[i].isvalue     = 1;            \
    stats->data[i].verbose     = 0

#define SET_COUNT                              \
    stats->data[i].long_format = "%-20.20s";   \
    stats->data[i].rate_format = "%5.5s";      \
    stats->data[i].isvalue     = 0;            \
    stats->data[i].mult_names  = "kgm";        \
    stats->data[i].mult        = 1000;         \
    stats->data[i].verbose     = 0

#define SET_BYTE                               \
    stats->data[i].long_format = "%-20.20s";   \
    stats->data[i].rate_format = "%5.5s";      \
    stats->data[i].isvalue     = 0;            \
    stats->data[i].mult_names  = "KGM";        \
    stats->data[i].mult        = 1024;         \
    stats->data[i].verbose     = 0


    stats->count = s.count;
    for (i = 0; i < s.count; i++) {
        stats->data[i].value = s.data[i].value;
        switch (s.data[i].type) {
        case _DRM_STAT_LOCK:
            stats->data[i].long_name = "Lock";
            stats->data[i].rate_name = "Lock";
            SET_VALUE;
            break;
        case _DRM_STAT_OPENS:
            stats->data[i].long_name = "Opens";
            stats->data[i].rate_name = "O";
            SET_COUNT;
            stats->data[i].verbose   = 1;
            break;
        case _DRM_STAT_CLOSES:
            stats->data[i].long_name = "Closes";
            stats->data[i].rate_name = "Lock";
            SET_COUNT;
            stats->data[i].verbose   = 1;
            break;
        case _DRM_STAT_IOCTLS:
            stats->data[i].long_name = "Ioctls";
            stats->data[i].rate_name = "Ioc/s";
            SET_COUNT;
            break;
        case _DRM_STAT_LOCKS:
            stats->data[i].long_name = "Locks";
            stats->data[i].rate_name = "Lck/s";
            SET_COUNT;
            break;
        case _DRM_STAT_UNLOCKS:
            stats->data[i].long_name = "Unlocks";
            stats->data[i].rate_name = "Unl/s";
            SET_COUNT;
            break;
        case _DRM_STAT_IRQ:
            stats->data[i].long_name = "IRQs";
            stats->data[i].rate_name = "IRQ/s";
            SET_COUNT;
            break;
        case _DRM_STAT_PRIMARY:
            stats->data[i].long_name = "Primary Bytes";
            stats->data[i].rate_name = "PB/s";
            SET_BYTE;
            break;
        case _DRM_STAT_SECONDARY:
            stats->data[i].long_name = "Secondary Bytes";
            stats->data[i].rate_name = "SB/s";
            SET_BYTE;
            break;
        case _DRM_STAT_DMA:
            stats->data[i].long_name = "DMA";
            stats->data[i].rate_name = "DMA/s";
            SET_COUNT;
            break;
        case _DRM_STAT_SPECIAL:
            stats->data[i].long_name = "Special DMA";
            stats->data[i].rate_name = "dma/s";
            SET_COUNT;
            break;
        case _DRM_STAT_MISSED:
            stats->data[i].long_name = "Miss";
            stats->data[i].rate_name = "Ms/s";
            SET_COUNT;
            break;
        case _DRM_STAT_VALUE:
            stats->data[i].long_name = "Value";
            stats->data[i].rate_name = "Value";
            SET_VALUE;
            break;
        case _DRM_STAT_BYTE:
            stats->data[i].long_name = "Bytes";
            stats->data[i].rate_name = "B/s";
            SET_BYTE;
            break;
        case _DRM_STAT_COUNT:
        default:
            stats->data[i].long_name = "Count";
            stats->data[i].rate_name = "Cnt/s";
            SET_COUNT;
            break;
        }
    }
    return 0;
}

/**
 * Issue a set-version ioctl.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index
 * \param data source pointer of the data to be read and written.
 * \param size size of the data to be read and written.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * It issues a read-write ioctl given by
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmSetInterfaceVersion(int fd, drmSetVersion *version)
{
    int retcode = 0;
    drm_set_version_t sv;

    memclear(sv);
    sv.drm_di_major = version->drm_di_major;
    sv.drm_di_minor = version->drm_di_minor;
    sv.drm_dd_major = version->drm_dd_major;
    sv.drm_dd_minor = version->drm_dd_minor;

    if (drmIoctl(fd, DRM_IOCTL_SET_VERSION, &sv)) {
        retcode = -errno;
    }

    version->drm_di_major = sv.drm_di_major;
    version->drm_di_minor = sv.drm_di_minor;
    version->drm_dd_major = sv.drm_dd_major;
    version->drm_dd_minor = sv.drm_dd_minor;

    return retcode;
}

/**
 * Send a device-specific command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * It issues a ioctl given by
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandNone(int fd, unsigned long drmCommandIndex)
{
    unsigned long request;

    request = DRM_IO( DRM_COMMAND_BASE + drmCommandIndex);

    if (drmIoctl(fd, request, NULL)) {
        return -errno;
    }
    return 0;
}


/**
 * Send a device-specific read command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index
 * \param data destination pointer of the data to be read.
 * \param size size of the data to be read.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * It issues a read ioctl given by
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandRead(int fd, unsigned long drmCommandIndex, void *data,
                   unsigned long size)
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_READ, DRM_IOCTL_BASE,
        DRM_COMMAND_BASE + drmCommandIndex, size);

    if (drmIoctl(fd, request, data)) {
        return -errno;
    }
    return 0;
}


/**
 * Send a device-specific write command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index
 * \param data source pointer of the data to be written.
 * \param size size of the data to be written.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * It issues a write ioctl given by
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandWrite(int fd, unsigned long drmCommandIndex, void *data,
                    unsigned long size)
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_WRITE, DRM_IOCTL_BASE,
        DRM_COMMAND_BASE + drmCommandIndex, size);

    if (drmIoctl(fd, request, data)) {
        return -errno;
    }
    return 0;
}


/**
 * Send a device-specific read-write command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index
 * \param data source pointer of the data to be read and written.
 * \param size size of the data to be read and written.
 *
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * It issues a read-write ioctl given by
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data,
                        unsigned long size)
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_READ|DRM_IOC_WRITE, DRM_IOCTL_BASE,
        DRM_COMMAND_BASE + drmCommandIndex, size);

    if (drmIoctl(fd, request, data))
        return -errno;
    return 0;
}

#define DRM_MAX_FDS 16
static struct {
    char *BusID;
    int fd;
    int refcount;
    int type;
} connection[DRM_MAX_FDS];

static int nr_fds = 0;

int drmOpenOnce(void *unused,
                const char *BusID,
                int *newlyopened)
{
    return drmOpenOnceWithType(BusID, newlyopened, DRM_NODE_PRIMARY);
}

int drmOpenOnceWithType(const char *BusID, int *newlyopened, int type)
{
    int i;
    int fd;

    for (i = 0; i < nr_fds; i++)
        if ((strcmp(BusID, connection[i].BusID) == 0) &&
            (connection[i].type == type)) {
            connection[i].refcount++;
            *newlyopened = 0;
            return connection[i].fd;
        }

    fd = drmOpenWithType(NULL, BusID, type);
    if (fd < 0 || nr_fds == DRM_MAX_FDS)
        return fd;

    connection[nr_fds].BusID = strdup(BusID);
    connection[nr_fds].fd = fd;
    connection[nr_fds].refcount = 1;
    connection[nr_fds].type = type;
    *newlyopened = 1;

    if (0)
        fprintf(stderr, "saved connection %d for %s %d\n",
                nr_fds, connection[nr_fds].BusID,
                strcmp(BusID, connection[nr_fds].BusID));

    nr_fds++;

    return fd;
}

void drmCloseOnce(int fd)
{
    int i;

    for (i = 0; i < nr_fds; i++) {
        if (fd == connection[i].fd) {
            if (--connection[i].refcount == 0) {
                drmClose(connection[i].fd);
                free(connection[i].BusID);

                if (i < --nr_fds)
                    connection[i] = connection[nr_fds];

                return;
            }
        }
    }
}

int drmSetMaster(int fd)
{
        return drmIoctl(fd, DRM_IOCTL_SET_MASTER, NULL);
}

int drmDropMaster(int fd)
{
        return drmIoctl(fd, DRM_IOCTL_DROP_MASTER, NULL);
}

char *drmGetDeviceNameFromFd(int fd)
{
    char name[128];
    struct stat sbuf;
    dev_t d;
    int i;

    /* The whole drmOpen thing is a fiasco and we need to find a way
     * back to just using open(2).  For now, however, lets just make
     * things worse with even more ad hoc directory walking code to
     * discover the device file name. */

    fstat(fd, &sbuf);
    d = sbuf.st_rdev;

    for (i = 0; i < DRM_MAX_MINOR; i++) {
        snprintf(name, sizeof name, DRM_DEV_NAME, DRM_DIR_NAME, i);
        if (stat(name, &sbuf) == 0 && sbuf.st_rdev == d)
            break;
    }
    if (i == DRM_MAX_MINOR)
        return NULL;

    return strdup(name);
}

int drmGetNodeTypeFromFd(int fd)
{
    struct stat sbuf;
    int maj, min, type;

    if (fstat(fd, &sbuf))
        return -1;

    maj = major(sbuf.st_rdev);
    min = minor(sbuf.st_rdev);

    if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode)) {
        errno = EINVAL;
        return -1;
    }

    type = drmGetMinorType(min);
    if (type == -1)
        errno = ENODEV;
    return type;
}

int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd)
{
    struct drm_prime_handle args;
    int ret;

    memclear(args);
    args.fd = -1;
    args.handle = handle;
    args.flags = flags;
    ret = drmIoctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
    if (ret)
        return ret;

    *prime_fd = args.fd;
    return 0;
}

int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle)
{
    struct drm_prime_handle args;
    int ret;

    memclear(args);
    args.fd = prime_fd;
    ret = drmIoctl(fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &args);
    if (ret)
        return ret;

    *handle = args.handle;
    return 0;
}

static char *drmGetMinorNameForFD(int fd, int type)
{
#ifdef __linux__
    DIR *sysdir;
    struct dirent *pent, *ent;
    struct stat sbuf;
    const char *name = drmGetMinorName(type);
    int len;
    char dev_name[64], buf[64];
    long name_max;
    int maj, min;

    if (!name)
        return NULL;

    len = strlen(name);

    if (fstat(fd, &sbuf))
        return NULL;

    maj = major(sbuf.st_rdev);
    min = minor(sbuf.st_rdev);

    if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode))
        return NULL;

    snprintf(buf, sizeof(buf), "/sys/dev/char/%d:%d/device/drm", maj, min);

    sysdir = opendir(buf);
    if (!sysdir)
        return NULL;

    name_max = fpathconf(dirfd(sysdir), _PC_NAME_MAX);
    if (name_max == -1)
        goto out_close_dir;

    pent = malloc(offsetof(struct dirent, d_name) + name_max + 1);
    if (pent == NULL)
         goto out_close_dir;

    while (readdir_r(sysdir, pent, &ent) == 0 && ent != NULL) {
        if (strncmp(ent->d_name, name, len) == 0) {
            snprintf(dev_name, sizeof(dev_name), DRM_DIR_NAME "/%s",
                 ent->d_name);

            free(pent);
            closedir(sysdir);

            return strdup(dev_name);
        }
    }

    free(pent);

out_close_dir:
    closedir(sysdir);
#else
    struct stat sbuf;
    char buf[PATH_MAX + 1];
    const char *dev_name;
    unsigned int maj, min;
    int n, base;

    if (fstat(fd, &sbuf))
        return NULL;

    maj = major(sbuf.st_rdev);
    min = minor(sbuf.st_rdev);

    if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode))
        return NULL;

    switch (type) {
    case DRM_NODE_PRIMARY:
        dev_name = DRM_DEV_NAME;
        break;
    case DRM_NODE_CONTROL:
        dev_name = DRM_CONTROL_DEV_NAME;
        break;
    case DRM_NODE_RENDER:
        dev_name = DRM_RENDER_DEV_NAME;
        break;
    default:
        return NULL;
    };

    base = drmGetMinorBase(type);
    if (base < 0)
        return NULL;

    n = snprintf(buf, sizeof(buf), dev_name, DRM_DIR_NAME, min - base);
    if (n == -1 || n >= sizeof(buf))
        return NULL;

    return strdup(buf);
#endif
    return NULL;
}

char *drmGetPrimaryDeviceNameFromFd(int fd)
{
    return drmGetMinorNameForFD(fd, DRM_NODE_PRIMARY);
}

char *drmGetRenderDeviceNameFromFd(int fd)
{
    return drmGetMinorNameForFD(fd, DRM_NODE_RENDER);
}

#ifdef __linux__
static char * DRM_PRINTFLIKE(2, 3)
sysfs_uevent_get(const char *path, const char *fmt, ...)
{
    char filename[PATH_MAX + 1], *key, *line = NULL, *value = NULL;
    size_t size = 0, len;
    ssize_t num;
    va_list ap;
    FILE *fp;

    va_start(ap, fmt);
    num = vasprintf(&key, fmt, ap);
    va_end(ap);
    len = num;

    snprintf(filename, sizeof(filename), "%s/uevent", path);

    fp = fopen(filename, "r");
    if (!fp) {
        free(key);
        return NULL;
    }

    while ((num = getline(&line, &size, fp)) >= 0) {
        if ((strncmp(line, key, len) == 0) && (line[len] == '=')) {
            char *start = line + len + 1, *end = line + num - 1;

            if (*end != '\n')
                end++;

            value = strndup(start, end - start);
            break;
        }
    }

    free(line);
    fclose(fp);

    free(key);

    return value;
}
#endif

static int drmParseSubsystemType(int maj, int min)
{
#ifdef __linux__
    char path[PATH_MAX + 1];
    char link[PATH_MAX + 1] = "";
    char *name;

    snprintf(path, PATH_MAX, "/sys/dev/char/%d:%d/device/subsystem",
             maj, min);

    if (readlink(path, link, PATH_MAX) < 0)
        return -errno;

    name = strrchr(link, '/');
    if (!name)
        return -EINVAL;

    if (strncmp(name, "/pci", 4) == 0)
        return DRM_BUS_PCI;

    if (strncmp(name, "/usb", 4) == 0)
        return DRM_BUS_USB;

    if (strncmp(name, "/platform", 9) == 0)
        return DRM_BUS_PLATFORM;

    if (strncmp(name, "/host1x", 7) == 0)
        return DRM_BUS_HOST1X;

    return -EINVAL;
#elif defined(__OpenBSD__)
    return DRM_BUS_PCI;
#else
#warning "Missing implementation of drmParseSubsystemType"
    return -EINVAL;
#endif
}

static int drmParsePciBusInfo(int maj, int min, drmPciBusInfoPtr info)
{
#ifdef __linux__
    unsigned int domain, bus, dev, func;
    char path[PATH_MAX + 1], *value;
    int num;

    snprintf(path, sizeof(path), "/sys/dev/char/%d:%d/device", maj, min);

    value = sysfs_uevent_get(path, "PCI_SLOT_NAME");
    if (!value)
        return -ENOENT;

    num = sscanf(value, "%04x:%02x:%02x.%1u", &domain, &bus, &dev, &func);
    free(value);

    if (num != 4)
        return -EINVAL;

    info->domain = domain;
    info->bus = bus;
    info->dev = dev;
    info->func = func;

    return 0;
#elif defined(__OpenBSD__)
    struct drm_pciinfo pinfo;
    int fd, type;

    type = drmGetMinorType(min);
    if (type == -1)
        return -ENODEV;

    fd = drmOpenMinor(min, 0, type);
    if (fd < 0)
        return -errno;

    if (drmIoctl(fd, DRM_IOCTL_GET_PCIINFO, &pinfo)) {
        close(fd);
        return -errno;
    }
    close(fd);

    info->domain = pinfo.domain;
    info->bus = pinfo.bus;
    info->dev = pinfo.dev;
    info->func = pinfo.func;

    return 0;
#else
#warning "Missing implementation of drmParsePciBusInfo"
    return -EINVAL;
#endif
}

int drmDevicesEqual(drmDevicePtr a, drmDevicePtr b)
{
    if (a == NULL || b == NULL)
        return 0;

    if (a->bustype != b->bustype)
        return 0;

    switch (a->bustype) {
    case DRM_BUS_PCI:
        return memcmp(a->businfo.pci, b->businfo.pci, sizeof(drmPciBusInfo)) == 0;

    case DRM_BUS_USB:
        return memcmp(a->businfo.usb, b->businfo.usb, sizeof(drmUsbBusInfo)) == 0;

    case DRM_BUS_PLATFORM:
        return memcmp(a->businfo.platform, b->businfo.platform, sizeof(drmPlatformBusInfo)) == 0;

    case DRM_BUS_HOST1X:
        return memcmp(a->businfo.host1x, b->businfo.host1x, sizeof(drmHost1xBusInfo)) == 0;

    default:
        break;
    }

    return 0;
}

static int drmGetNodeType(const char *name)
{
    if (strncmp(name, DRM_PRIMARY_MINOR_NAME,
        sizeof(DRM_PRIMARY_MINOR_NAME) - 1) == 0)
        return DRM_NODE_PRIMARY;

    if (strncmp(name, DRM_CONTROL_MINOR_NAME,
        sizeof(DRM_CONTROL_MINOR_NAME ) - 1) == 0)
        return DRM_NODE_CONTROL;

    if (strncmp(name, DRM_RENDER_MINOR_NAME,
        sizeof(DRM_RENDER_MINOR_NAME) - 1) == 0)
        return DRM_NODE_RENDER;

    return -EINVAL;
}

static int drmGetMaxNodeName(void)
{
    return sizeof(DRM_DIR_NAME) +
           MAX3(sizeof(DRM_PRIMARY_MINOR_NAME),
                sizeof(DRM_CONTROL_MINOR_NAME),
                sizeof(DRM_RENDER_MINOR_NAME)) +
           3 /* length of the node number */;
}

#ifdef __linux__
static int parse_separate_sysfs_files(int maj, int min,
                                      drmPciDeviceInfoPtr device,
                                      bool ignore_revision)
{
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
    static const char *attrs[] = {
      "revision", /* Older kernels are missing the file, so check for it first */
      "vendor",
      "device",
      "subsystem_vendor",
      "subsystem_device",
    };
    char path[PATH_MAX + 1];
    unsigned int data[ARRAY_SIZE(attrs)];
    FILE *fp;
    int ret;

    for (unsigned i = ignore_revision ? 1 : 0; i < ARRAY_SIZE(attrs); i++) {
        snprintf(path, PATH_MAX, "/sys/dev/char/%d:%d/device/%s", maj, min,
                 attrs[i]);
        fp = fopen(path, "r");
        if (!fp)
            return -errno;

        ret = fscanf(fp, "%x", &data[i]);
        fclose(fp);
        if (ret != 1)
            return -errno;

    }

    device->revision_id = ignore_revision ? 0xff : data[0] & 0xff;
    device->vendor_id = data[1] & 0xffff;
    device->device_id = data[2] & 0xffff;
    device->subvendor_id = data[3] & 0xffff;
    device->subdevice_id = data[4] & 0xffff;

    return 0;
}

static int parse_config_sysfs_file(int maj, int min,
                                   drmPciDeviceInfoPtr device)
{
    char path[PATH_MAX + 1];
    unsigned char config[64];
    int fd, ret;

    snprintf(path, PATH_MAX, "/sys/dev/char/%d:%d/device/config", maj, min);
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -errno;

    ret = read(fd, config, sizeof(config));
    close(fd);
    if (ret < 0)
        return -errno;

    device->vendor_id = config[0] | (config[1] << 8);
    device->device_id = config[2] | (config[3] << 8);
    device->revision_id = config[8];
    device->subvendor_id = config[44] | (config[45] << 8);
    device->subdevice_id = config[46] | (config[47] << 8);

    return 0;
}
#endif

static int drmParsePciDeviceInfo(int maj, int min,
                                 drmPciDeviceInfoPtr device,
                                 uint32_t flags)
{
#ifdef __linux__
    if (!(flags & DRM_DEVICE_GET_PCI_REVISION))
        return parse_separate_sysfs_files(maj, min, device, true);

    if (parse_separate_sysfs_files(maj, min, device, false))
        return parse_config_sysfs_file(maj, min, device);

    return 0;
#elif defined(__OpenBSD__)
    struct drm_pciinfo pinfo;
    int fd, type;

    type = drmGetMinorType(min);
    if (type == -1)
        return -ENODEV;

    fd = drmOpenMinor(min, 0, type);
    if (fd < 0)
        return -errno;

    if (drmIoctl(fd, DRM_IOCTL_GET_PCIINFO, &pinfo)) {
        close(fd);
        return -errno;
    }
    close(fd);

    device->vendor_id = pinfo.vendor_id;
    device->device_id = pinfo.device_id;
    device->revision_id = pinfo.revision_id;
    device->subvendor_id = pinfo.subvendor_id;
    device->subdevice_id = pinfo.subdevice_id;

    return 0;
#else
#warning "Missing implementation of drmParsePciDeviceInfo"
    return -EINVAL;
#endif
}

static void drmFreePlatformDevice(drmDevicePtr device)
{
    if (device->deviceinfo.platform) {
        if (device->deviceinfo.platform->compatible) {
            char **compatible = device->deviceinfo.platform->compatible;

            while (*compatible) {
                free(*compatible);
                compatible++;
            }

            free(device->deviceinfo.platform->compatible);
        }
    }
}

static void drmFreeHost1xDevice(drmDevicePtr device)
{
    if (device->deviceinfo.host1x) {
        if (device->deviceinfo.host1x->compatible) {
            char **compatible = device->deviceinfo.host1x->compatible;

            while (*compatible) {
                free(*compatible);
                compatible++;
            }

            free(device->deviceinfo.host1x->compatible);
        }
    }
}

void drmFreeDevice(drmDevicePtr *device)
{
    if (device == NULL)
        return;

    if (*device) {
        switch ((*device)->bustype) {
        case DRM_BUS_PLATFORM:
            drmFreePlatformDevice(*device);
            break;

        case DRM_BUS_HOST1X:
            drmFreeHost1xDevice(*device);
            break;
        }
    }

    free(*device);
    *device = NULL;
}

void drmFreeDevices(drmDevicePtr devices[], int count)
{
    int i;

    if (devices == NULL)
        return;

    for (i = 0; i < count; i++)
        if (devices[i])
            drmFreeDevice(&devices[i]);
}

static drmDevicePtr drmDeviceAlloc(unsigned int type, const char *node,
                                   size_t bus_size, size_t device_size,
                                   char **ptrp)
{
    size_t max_node_length, extra, size;
    drmDevicePtr device;
    unsigned int i;
    char *ptr;

    max_node_length = ALIGN(drmGetMaxNodeName(), sizeof(void *));
    extra = DRM_NODE_MAX * (sizeof(void *) + max_node_length);

    size = sizeof(*device) + extra + bus_size + device_size;

    device = calloc(1, size);
    if (!device)
        return NULL;

    device->available_nodes = 1 << type;

    ptr = (char *)device + sizeof(*device);
    device->nodes = (char **)ptr;

    ptr += DRM_NODE_MAX * sizeof(void *);

    for (i = 0; i < DRM_NODE_MAX; i++) {
        device->nodes[i] = ptr;
        ptr += max_node_length;
    }

    memcpy(device->nodes[type], node, max_node_length);

    *ptrp = ptr;

    return device;
}

static int drmProcessPciDevice(drmDevicePtr *device,
                               const char *node, int node_type,
                               int maj, int min, bool fetch_deviceinfo,
                               uint32_t flags)
{
    drmDevicePtr dev;
    char *addr;
    int ret;

    dev = drmDeviceAlloc(node_type, node, sizeof(drmPciBusInfo),
                         sizeof(drmPciDeviceInfo), &addr);
    if (!dev)
        return -ENOMEM;

    dev->bustype = DRM_BUS_PCI;

    dev->businfo.pci = (drmPciBusInfoPtr)addr;

    ret = drmParsePciBusInfo(maj, min, dev->businfo.pci);
    if (ret)
        goto free_device;

    // Fetch the device info if the user has requested it
    if (fetch_deviceinfo) {
        addr += sizeof(drmPciBusInfo);
        dev->deviceinfo.pci = (drmPciDeviceInfoPtr)addr;

        ret = drmParsePciDeviceInfo(maj, min, dev->deviceinfo.pci, flags);
        if (ret)
            goto free_device;
    }

    *device = dev;

    return 0;

free_device:
    free(dev);
    return ret;
}

static int drmParseUsbBusInfo(int maj, int min, drmUsbBusInfoPtr info)
{
#ifdef __linux__
    char path[PATH_MAX + 1], *value;
    unsigned int bus, dev;
    int ret;

    snprintf(path, sizeof(path), "/sys/dev/char/%d:%d/device", maj, min);

    value = sysfs_uevent_get(path, "BUSNUM");
    if (!value)
        return -ENOENT;

    ret = sscanf(value, "%03u", &bus);
    free(value);

    if (ret <= 0)
        return -errno;

    value = sysfs_uevent_get(path, "DEVNUM");
    if (!value)
        return -ENOENT;

    ret = sscanf(value, "%03u", &dev);
    free(value);

    if (ret <= 0)
        return -errno;

    info->bus = bus;
    info->dev = dev;

    return 0;
#else
#warning "Missing implementation of drmParseUsbBusInfo"
    return -EINVAL;
#endif
}

static int drmParseUsbDeviceInfo(int maj, int min, drmUsbDeviceInfoPtr info)
{
#ifdef __linux__
    char path[PATH_MAX + 1], *value;
    unsigned int vendor, product;
    int ret;

    snprintf(path, sizeof(path), "/sys/dev/char/%d:%d/device", maj, min);

    value = sysfs_uevent_get(path, "PRODUCT");
    if (!value)
        return -ENOENT;

    ret = sscanf(value, "%x/%x", &vendor, &product);
    free(value);

    if (ret <= 0)
        return -errno;

    info->vendor = vendor;
    info->product = product;

    return 0;
#else
#warning "Missing implementation of drmParseUsbDeviceInfo"
    return -EINVAL;
#endif
}

static int drmProcessUsbDevice(drmDevicePtr *device, const char *node,
                               int node_type, int maj, int min,
                               bool fetch_deviceinfo, uint32_t flags)
{
    drmDevicePtr dev;
    char *ptr;
    int ret;

    dev = drmDeviceAlloc(node_type, node, sizeof(drmUsbBusInfo),
                         sizeof(drmUsbDeviceInfo), &ptr);
    if (!dev)
        return -ENOMEM;

    dev->bustype = DRM_BUS_USB;

    dev->businfo.usb = (drmUsbBusInfoPtr)ptr;

    ret = drmParseUsbBusInfo(maj, min, dev->businfo.usb);
    if (ret < 0)
        goto free_device;

    if (fetch_deviceinfo) {
        ptr += sizeof(drmUsbBusInfo);
        dev->deviceinfo.usb = (drmUsbDeviceInfoPtr)ptr;

        ret = drmParseUsbDeviceInfo(maj, min, dev->deviceinfo.usb);
        if (ret < 0)
            goto free_device;
    }

    *device = dev;

    return 0;

free_device:
    free(dev);
    return ret;
}

static int drmParsePlatformBusInfo(int maj, int min, drmPlatformBusInfoPtr info)
{
#ifdef __linux__
    char path[PATH_MAX + 1], *name;

    snprintf(path, sizeof(path), "/sys/dev/char/%d:%d/device", maj, min);

    name = sysfs_uevent_get(path, "OF_FULLNAME");
    if (!name)
        return -ENOENT;

    strncpy(info->fullname, name, DRM_PLATFORM_DEVICE_NAME_LEN);
    info->fullname[DRM_PLATFORM_DEVICE_NAME_LEN - 1] = '\0';
    free(name);

    return 0;
#else
#warning "Missing implementation of drmParsePlatformBusInfo"
    return -EINVAL;
#endif
}

static int drmParsePlatformDeviceInfo(int maj, int min,
                                      drmPlatformDeviceInfoPtr info)
{
#ifdef __linux__
    char path[PATH_MAX + 1], *value;
    unsigned int count, i;
    int err;

    snprintf(path, sizeof(path), "/sys/dev/char/%d:%d/device", maj, min);

    value = sysfs_uevent_get(path, "OF_COMPATIBLE_N");
    if (!value)
        return -ENOENT;

    sscanf(value, "%u", &count);
    free(value);

    info->compatible = calloc(count + 1, sizeof(*info->compatible));
    if (!info->compatible)
        return -ENOMEM;

    for (i = 0; i < count; i++) {
        value = sysfs_uevent_get(path, "OF_COMPATIBLE_%u", i);
        if (!value) {
            err = -ENOENT;
            goto free;
        }

        info->compatible[i] = value;
    }

    return 0;

free:
    while (i--)
        free(info->compatible[i]);

    free(info->compatible);
    return err;
#else
#warning "Missing implementation of drmParsePlatformDeviceInfo"
    return -EINVAL;
#endif
}

static int drmProcessPlatformDevice(drmDevicePtr *device,
                                    const char *node, int node_type,
                                    int maj, int min, bool fetch_deviceinfo,
                                    uint32_t flags)
{
    drmDevicePtr dev;
    char *ptr;
    int ret;

    dev = drmDeviceAlloc(node_type, node, sizeof(drmPlatformBusInfo),
                         sizeof(drmPlatformDeviceInfo), &ptr);
    if (!dev)
        return -ENOMEM;

    dev->bustype = DRM_BUS_PLATFORM;

    dev->businfo.platform = (drmPlatformBusInfoPtr)ptr;

    ret = drmParsePlatformBusInfo(maj, min, dev->businfo.platform);
    if (ret < 0)
        goto free_device;

    if (fetch_deviceinfo) {
        ptr += sizeof(drmPlatformBusInfo);
        dev->deviceinfo.platform = (drmPlatformDeviceInfoPtr)ptr;

        ret = drmParsePlatformDeviceInfo(maj, min, dev->deviceinfo.platform);
        if (ret < 0)
            goto free_device;
    }

    *device = dev;

    return 0;

free_device:
    free(dev);
    return ret;
}

static int drmParseHost1xBusInfo(int maj, int min, drmHost1xBusInfoPtr info)
{
#ifdef __linux__
    char path[PATH_MAX + 1], *name;

    snprintf(path, sizeof(path), "/sys/dev/char/%d:%d/device", maj, min);

    name = sysfs_uevent_get(path, "OF_FULLNAME");
    if (!name)
        return -ENOENT;

    strncpy(info->fullname, name, DRM_HOST1X_DEVICE_NAME_LEN);
    info->fullname[DRM_HOST1X_DEVICE_NAME_LEN - 1] = '\0';
    free(name);

    return 0;
#else
#warning "Missing implementation of drmParseHost1xBusInfo"
    return -EINVAL;
#endif
}

static int drmParseHost1xDeviceInfo(int maj, int min,
                                    drmHost1xDeviceInfoPtr info)
{
#ifdef __linux__
    char path[PATH_MAX + 1], *value;
    unsigned int count, i;
    int err;

    snprintf(path, sizeof(path), "/sys/dev/char/%d:%d/device", maj, min);

    value = sysfs_uevent_get(path, "OF_COMPATIBLE_N");
    if (!value)
        return -ENOENT;

    sscanf(value, "%u", &count);
    free(value);

    info->compatible = calloc(count + 1, sizeof(*info->compatible));
    if (!info->compatible)
        return -ENOMEM;

    for (i = 0; i < count; i++) {
        value = sysfs_uevent_get(path, "OF_COMPATIBLE_%u", i);
        if (!value) {
            err = -ENOENT;
            goto free;
        }

        info->compatible[i] = value;
    }

    return 0;

free:
    while (i--)
        free(info->compatible[i]);

    free(info->compatible);
    return err;
#else
#warning "Missing implementation of drmParseHost1xDeviceInfo"
    return -EINVAL;
#endif
}

static int drmProcessHost1xDevice(drmDevicePtr *device,
                                  const char *node, int node_type,
                                  int maj, int min, bool fetch_deviceinfo,
                                  uint32_t flags)
{
    drmDevicePtr dev;
    char *ptr;
    int ret;

    dev = drmDeviceAlloc(node_type, node, sizeof(drmHost1xBusInfo),
                         sizeof(drmHost1xDeviceInfo), &ptr);
    if (!dev)
        return -ENOMEM;

    dev->bustype = DRM_BUS_HOST1X;

    dev->businfo.host1x = (drmHost1xBusInfoPtr)ptr;

    ret = drmParseHost1xBusInfo(maj, min, dev->businfo.host1x);
    if (ret < 0)
        goto free_device;

    if (fetch_deviceinfo) {
        ptr += sizeof(drmHost1xBusInfo);
        dev->deviceinfo.host1x = (drmHost1xDeviceInfoPtr)ptr;

        ret = drmParseHost1xDeviceInfo(maj, min, dev->deviceinfo.host1x);
        if (ret < 0)
            goto free_device;
    }

    *device = dev;

    return 0;

free_device:
    free(dev);
    return ret;
}

/* Consider devices located on the same bus as duplicate and fold the respective
 * entries into a single one.
 *
 * Note: this leaves "gaps" in the array, while preserving the length.
 */
static void drmFoldDuplicatedDevices(drmDevicePtr local_devices[], int count)
{
    int node_type, i, j;

    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            if (drmDevicesEqual(local_devices[i], local_devices[j])) {
                local_devices[i]->available_nodes |= local_devices[j]->available_nodes;
                node_type = log2(local_devices[j]->available_nodes);
                memcpy(local_devices[i]->nodes[node_type],
                       local_devices[j]->nodes[node_type], drmGetMaxNodeName());
                drmFreeDevice(&local_devices[j]);
            }
        }
    }
}

/* Check that the given flags are valid returning 0 on success */
static int
drm_device_validate_flags(uint32_t flags)
{
        return (flags & ~DRM_DEVICE_GET_PCI_REVISION);
}

/**
 * Get information about the opened drm device
 *
 * \param fd file descriptor of the drm device
 * \param flags feature/behaviour bitmask
 * \param device the address of a drmDevicePtr where the information
 *               will be allocated in stored
 *
 * \return zero on success, negative error code otherwise.
 *
 * \note Unlike drmGetDevice it does not retrieve the pci device revision field
 * unless the DRM_DEVICE_GET_PCI_REVISION \p flag is set.
 */
int drmGetDevice2(int fd, uint32_t flags, drmDevicePtr *device)
{
#ifdef __OpenBSD__
    /*
     * DRI device nodes on OpenBSD are not in their own directory, they reside
     * in /dev along with a large number of statically generated /dev nodes.
     * Avoid stat'ing all of /dev needlessly by implementing this custom path.
     */
    drmDevicePtr     d;
    struct stat      sbuf;
    char             node[PATH_MAX + 1];
    const char      *dev_name;
    int              node_type, subsystem_type;
    int              maj, min, n, ret, base;

    if (fd == -1 || device == NULL)
        return -EINVAL;

    if (fstat(fd, &sbuf))
        return -errno;

    maj = major(sbuf.st_rdev);
    min = minor(sbuf.st_rdev);

    if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode))
        return -EINVAL;

    node_type = drmGetMinorType(min);
    if (node_type == -1)
        return -ENODEV;

    switch (node_type) {
    case DRM_NODE_PRIMARY:
        dev_name = DRM_DEV_NAME;
        break;
    case DRM_NODE_CONTROL:
        dev_name = DRM_CONTROL_DEV_NAME;
        break;
    case DRM_NODE_RENDER:
        dev_name = DRM_RENDER_DEV_NAME;
        break;
    default:
        return -EINVAL;
    };

    base = drmGetMinorBase(node_type);
    if (base < 0)
        return -EINVAL;

    n = snprintf(node, PATH_MAX, dev_name, DRM_DIR_NAME, min - base);
    if (n == -1 || n >= PATH_MAX)
      return -errno;
    if (stat(node, &sbuf))
        return -EINVAL;

    subsystem_type = drmParseSubsystemType(maj, min);
    if (subsystem_type != DRM_BUS_PCI)
        return -ENODEV;

    ret = drmProcessPciDevice(&d, node, node_type, maj, min, true, flags);
    if (ret)
        return ret;

    *device = d;

    return 0;
#else
    drmDevicePtr *local_devices;
    drmDevicePtr d;
    DIR *sysdir;
    struct dirent *dent;
    struct stat sbuf;
    char node[PATH_MAX + 1];
    int node_type, subsystem_type;
    int maj, min;
    int ret, i, node_count;
    int max_count = 16;
    dev_t find_rdev;

    if (drm_device_validate_flags(flags))
        return -EINVAL;

    if (fd == -1 || device == NULL)
        return -EINVAL;

    if (fstat(fd, &sbuf))
        return -errno;

    find_rdev = sbuf.st_rdev;
    maj = major(sbuf.st_rdev);
    min = minor(sbuf.st_rdev);

    if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode))
        return -EINVAL;

    subsystem_type = drmParseSubsystemType(maj, min);

    local_devices = calloc(max_count, sizeof(drmDevicePtr));
    if (local_devices == NULL)
        return -ENOMEM;

    sysdir = opendir(DRM_DIR_NAME);
    if (!sysdir) {
        ret = -errno;
        goto free_locals;
    }

    i = 0;
    while ((dent = readdir(sysdir))) {
        node_type = drmGetNodeType(dent->d_name);
        if (node_type < 0)
            continue;

        snprintf(node, PATH_MAX, "%s/%s", DRM_DIR_NAME, dent->d_name);
        if (stat(node, &sbuf))
            continue;

        maj = major(sbuf.st_rdev);
        min = minor(sbuf.st_rdev);

        if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode))
            continue;

        if (drmParseSubsystemType(maj, min) != subsystem_type)
            continue;

        switch (subsystem_type) {
        case DRM_BUS_PCI:
            ret = drmProcessPciDevice(&d, node, node_type, maj, min, true, flags);
            if (ret)
                continue;

            break;

        case DRM_BUS_USB:
            ret = drmProcessUsbDevice(&d, node, node_type, maj, min, true, flags);
            if (ret)
                continue;

            break;

        case DRM_BUS_PLATFORM:
            ret = drmProcessPlatformDevice(&d, node, node_type, maj, min, true, flags);
            if (ret)
                continue;

            break;

        case DRM_BUS_HOST1X:
            ret = drmProcessHost1xDevice(&d, node, node_type, maj, min, true, flags);
            if (ret)
                continue;

            break;

        default:
            continue;
        }

        if (i >= max_count) {
            drmDevicePtr *temp;

            max_count += 16;
            temp = realloc(local_devices, max_count * sizeof(drmDevicePtr));
            if (!temp)
                goto free_devices;
            local_devices = temp;
        }

        /* store target at local_devices[0] for ease to use below */
        if (find_rdev == sbuf.st_rdev && i) {
            local_devices[i] = local_devices[0];
            local_devices[0] = d;
        }
        else
            local_devices[i] = d;
        i++;
    }
    node_count = i;

    drmFoldDuplicatedDevices(local_devices, node_count);

    *device = local_devices[0];
    drmFreeDevices(&local_devices[1], node_count - 1);

    closedir(sysdir);
    free(local_devices);
    if (*device == NULL)
        return -ENODEV;
    return 0;

free_devices:
    drmFreeDevices(local_devices, i);
    closedir(sysdir);

free_locals:
    free(local_devices);
    return ret;
#endif
}

/**
 * Get information about the opened drm device
 *
 * \param fd file descriptor of the drm device
 * \param device the address of a drmDevicePtr where the information
 *               will be allocated in stored
 *
 * \return zero on success, negative error code otherwise.
 */
int drmGetDevice(int fd, drmDevicePtr *device)
{
    return drmGetDevice2(fd, DRM_DEVICE_GET_PCI_REVISION, device);
}

/**
 * Get drm devices on the system
 *
 * \param flags feature/behaviour bitmask
 * \param devices the array of devices with drmDevicePtr elements
 *                can be NULL to get the device number first
 * \param max_devices the maximum number of devices for the array
 *
 * \return on error - negative error code,
 *         if devices is NULL - total number of devices available on the system,
 *         alternatively the number of devices stored in devices[], which is
 *         capped by the max_devices.
 *
 * \note Unlike drmGetDevices it does not retrieve the pci device revision field
 * unless the DRM_DEVICE_GET_PCI_REVISION \p flag is set.
 */
int drmGetDevices2(uint32_t flags, drmDevicePtr devices[], int max_devices)
{
    drmDevicePtr *local_devices;
    drmDevicePtr device;
    DIR *sysdir;
    struct dirent *dent;
    struct stat sbuf;
    char node[PATH_MAX + 1];
    int node_type, subsystem_type;
    int maj, min;
    int ret, i, node_count, device_count;
    int max_count = 16;

    if (drm_device_validate_flags(flags))
        return -EINVAL;

    local_devices = calloc(max_count, sizeof(drmDevicePtr));
    if (local_devices == NULL)
        return -ENOMEM;

    sysdir = opendir(DRM_DIR_NAME);
    if (!sysdir) {
        ret = -errno;
        goto free_locals;
    }

    i = 0;
    while ((dent = readdir(sysdir))) {
        node_type = drmGetNodeType(dent->d_name);
        if (node_type < 0)
            continue;

        snprintf(node, PATH_MAX, "%s/%s", DRM_DIR_NAME, dent->d_name);
        if (stat(node, &sbuf))
            continue;

        maj = major(sbuf.st_rdev);
        min = minor(sbuf.st_rdev);

        if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode))
            continue;

        subsystem_type = drmParseSubsystemType(maj, min);

        if (subsystem_type < 0)
            continue;

        switch (subsystem_type) {
        case DRM_BUS_PCI:
            ret = drmProcessPciDevice(&device, node, node_type,
                                      maj, min, devices != NULL, flags);
            if (ret)
                continue;

            break;

        case DRM_BUS_USB:
            ret = drmProcessUsbDevice(&device, node, node_type, maj, min,
                                      devices != NULL, flags);
            if (ret)
                continue;

            break;

        case DRM_BUS_PLATFORM:
            ret = drmProcessPlatformDevice(&device, node, node_type, maj, min,
                                           devices != NULL, flags);
            if (ret)
                continue;

            break;

        case DRM_BUS_HOST1X:
            ret = drmProcessHost1xDevice(&device, node, node_type, maj, min,
                                         devices != NULL, flags);
            if (ret)
                continue;

            break;

        default:
            continue;
        }

        if (i >= max_count) {
            drmDevicePtr *temp;

            max_count += 16;
            temp = realloc(local_devices, max_count * sizeof(drmDevicePtr));
            if (!temp)
                goto free_devices;
            local_devices = temp;
        }

        local_devices[i] = device;
        i++;
    }
    node_count = i;

    drmFoldDuplicatedDevices(local_devices, node_count);

    device_count = 0;
    for (i = 0; i < node_count; i++) {
        if (!local_devices[i])
            continue;

        if ((devices != NULL) && (device_count < max_devices))
            devices[device_count] = local_devices[i];
        else
            drmFreeDevice(&local_devices[i]);

        device_count++;
    }

    closedir(sysdir);
    free(local_devices);
    return device_count;

free_devices:
    drmFreeDevices(local_devices, i);
    closedir(sysdir);

free_locals:
    free(local_devices);
    return ret;
}

/**
 * Get drm devices on the system
 *
 * \param devices the array of devices with drmDevicePtr elements
 *                can be NULL to get the device number first
 * \param max_devices the maximum number of devices for the array
 *
 * \return on error - negative error code,
 *         if devices is NULL - total number of devices available on the system,
 *         alternatively the number of devices stored in devices[], which is
 *         capped by the max_devices.
 */
int drmGetDevices(drmDevicePtr devices[], int max_devices)
{
    return drmGetDevices2(DRM_DEVICE_GET_PCI_REVISION, devices, max_devices);
}

char *drmGetDeviceNameFromFd2(int fd)
{
#ifdef __linux__
    struct stat sbuf;
    char path[PATH_MAX + 1], *value;
    unsigned int maj, min;

    if (fstat(fd, &sbuf))
        return NULL;

    maj = major(sbuf.st_rdev);
    min = minor(sbuf.st_rdev);

    if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode))
        return NULL;

    snprintf(path, sizeof(path), "/sys/dev/char/%d:%d", maj, min);

    value = sysfs_uevent_get(path, "DEVNAME");
    if (!value)
        return NULL;

    snprintf(path, sizeof(path), "/dev/%s", value);
    free(value);

    return strdup(path);
#else
    struct stat      sbuf;
    char             node[PATH_MAX + 1];
    const char      *dev_name;
    int              node_type;
    int              maj, min, n, base;

    if (fstat(fd, &sbuf))
        return NULL;

    maj = major(sbuf.st_rdev);
    min = minor(sbuf.st_rdev);

    if (maj != DRM_MAJOR || !S_ISCHR(sbuf.st_mode))
        return NULL;

    node_type = drmGetMinorType(min);
    if (node_type == -1)
        return NULL;

    switch (node_type) {
    case DRM_NODE_PRIMARY:
        dev_name = DRM_DEV_NAME;
        break;
    case DRM_NODE_CONTROL:
        dev_name = DRM_CONTROL_DEV_NAME;
        break;
    case DRM_NODE_RENDER:
        dev_name = DRM_RENDER_DEV_NAME;
        break;
    default:
        return NULL;
    };

    base = drmGetMinorBase(node_type);
    if (base < 0)
        return NULL;

    n = snprintf(node, PATH_MAX, dev_name, DRM_DIR_NAME, min - base);
    if (n == -1 || n >= PATH_MAX)
      return NULL;

    return strdup(node);
#endif
}

int drmSyncobjCreate(int fd, uint32_t flags, uint32_t *handle)
{
    struct drm_syncobj_create args;
    int ret;

    memclear(args);
    args.flags = flags;
    args.handle = 0;
    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_CREATE, &args);
    if (ret)
	return ret;
    *handle = args.handle;
    return 0;
}

int drmSyncobjDestroy(int fd, uint32_t handle)
{
    struct drm_syncobj_destroy args;

    memclear(args);
    args.handle = handle;
    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_DESTROY, &args);
}

int drmSyncobjHandleToFD(int fd, uint32_t handle, int *obj_fd)
{
    struct drm_syncobj_handle args;
    int ret;

    memclear(args);
    args.fd = -1;
    args.handle = handle;
    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &args);
    if (ret)
	return ret;
    *obj_fd = args.fd;
    return 0;
}

int drmSyncobjFDToHandle(int fd, int obj_fd, uint32_t *handle)
{
    struct drm_syncobj_handle args;
    int ret;

    memclear(args);
    args.fd = obj_fd;
    args.handle = 0;
    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, &args);
    if (ret)
	return ret;
    *handle = args.handle;
    return 0;
}

int drmSyncobjImportSyncFile(int fd, uint32_t handle, int sync_file_fd)
{
    struct drm_syncobj_handle args;

    memclear(args);
    args.fd = sync_file_fd;
    args.handle = handle;
    args.flags = DRM_SYNCOBJ_FD_TO_HANDLE_FLAGS_IMPORT_SYNC_FILE;
    return drmIoctl(fd, DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE, &args);
}

int drmSyncobjExportSyncFile(int fd, uint32_t handle, int *sync_file_fd)
{
    struct drm_syncobj_handle args;
    int ret;

    memclear(args);
    args.fd = -1;
    args.handle = handle;
    args.flags = DRM_SYNCOBJ_HANDLE_TO_FD_FLAGS_EXPORT_SYNC_FILE;
    ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD, &args);
    if (ret)
	return ret;
    *sync_file_fd = args.fd;
    return 0;
}

int drmSyncobjWait(int fd, uint32_t *handles, unsigned num_handles,
		   int64_t timeout_nsec, unsigned flags,
		   uint32_t *first_signaled)
{
	struct drm_syncobj_wait args;
	int ret;

	memclear(args);
	args.handles = (intptr_t)handles;
	args.timeout_nsec = timeout_nsec;
	args.count_handles = num_handles;
	args.flags = flags;

	ret = drmIoctl(fd, DRM_IOCTL_SYNCOBJ_WAIT, &args);
	if (ret < 0)
		return ret;

	if (first_signaled)
		*first_signaled = args.first_signaled;
	return ret;
}
