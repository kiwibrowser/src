/*
 *  sync.c
 *
 *   Copyright 2012 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <fcntl.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sync/sync.h>


struct sw_sync_create_fence_data {
  __u32 value;
  char name[32];
  __s32 fence;
};

#define SW_SYNC_IOC_MAGIC 'W'
#define SW_SYNC_IOC_CREATE_FENCE _IOWR(SW_SYNC_IOC_MAGIC, 0, struct sw_sync_create_fence_data)
#define SW_SYNC_IOC_INC _IOW(SW_SYNC_IOC_MAGIC, 1, __u32)

int sync_wait(int fd, int timeout)
{
    struct pollfd fds;
    int ret;

    if (fd < 0) {
        errno = EINVAL;
        return -1;
    }

    fds.fd = fd;
    fds.events = POLLIN;

    do {
        ret = poll(&fds, 1, timeout);
        if (ret > 0) {
            if (fds.revents & (POLLERR | POLLNVAL)) {
                errno = EINVAL;
                return -1;
            }
            return 0;
        } else if (ret == 0) {
            errno = ETIME;
            return -1;
        }
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

    return ret;
}

int sync_merge(const char *name, int fd1, int fd2)
{
    struct sync_legacy_merge_data legacy_data;
    struct sync_merge_data data;
    int ret;

    data.fd2 = fd2;
    strlcpy(data.name, name, sizeof(data.name));
    data.flags = 0;
    data.pad = 0;

    ret = ioctl(fd1, SYNC_IOC_MERGE, &data);
    if (ret < 0 && errno == ENOTTY) {
        legacy_data.fd2 = fd2;
        strlcpy(legacy_data.name, name, sizeof(legacy_data.name));

        ret = ioctl(fd1, SYNC_IOC_LEGACY_MERGE, &legacy_data);
        if (ret < 0)
            return ret;

        return legacy_data.fence;
    } else if (ret < 0) {
        return ret;
    }

    return data.fence;
}

struct sync_fence_info_data *sync_fence_info(int fd)
{
    struct sync_fence_info_data *legacy_info;
    struct sync_pt_info *legacy_pt_info;
    struct sync_file_info *info;
    struct sync_fence_info *fence_info;
    int err, num_fences, i;

    legacy_info = malloc(4096);
    if (legacy_info == NULL)
        return NULL;

    legacy_info->len = 4096;
    err = ioctl(fd, SYNC_IOC_LEGACY_FENCE_INFO, legacy_info);
    if (err < 0 && errno != ENOTTY) {
        free(legacy_info);
        return NULL;
    } else if (err == 0) {
        return legacy_info;
    }

    info = calloc(1, sizeof(*info));
    if (info == NULL)
        goto free;

    err = ioctl(fd, SYNC_IOC_FILE_INFO, info);
    if (err < 0)
        goto free;

    num_fences = info->num_fences;

    if (num_fences) {
        info->flags = 0;
        info->num_fences = num_fences;
        info->sync_fence_info = (uint64_t) calloc(num_fences,
                                        sizeof(struct sync_fence_info));
        if ((void *)info->sync_fence_info == NULL)
            goto free;

        err = ioctl(fd, SYNC_IOC_FILE_INFO, info);
        if (err < 0) {
            free((void *)info->sync_fence_info);
            goto free;
        }
    }

    legacy_info->len = sizeof(*legacy_info) +
                        num_fences * sizeof(struct sync_fence_info);
    strlcpy(legacy_info->name, info->name, sizeof(legacy_info->name));
    legacy_info->status = info->status;

    legacy_pt_info = (struct sync_pt_info *)legacy_info->pt_info;
    fence_info = (struct sync_fence_info *)info->sync_fence_info;
    for (i = 0 ; i < num_fences ; i++) {
        legacy_pt_info[i].len = sizeof(*legacy_pt_info);
        strlcpy(legacy_pt_info[i].obj_name, fence_info[i].obj_name,
                sizeof(legacy_pt_info->obj_name));
        strlcpy(legacy_pt_info[i].driver_name, fence_info[i].driver_name,
                sizeof(legacy_pt_info->driver_name));
        legacy_pt_info[i].status = fence_info[i].status;
        legacy_pt_info[i].timestamp_ns = fence_info[i].timestamp_ns;
    }

    free((void *)info->sync_fence_info);
    free(info);
    return legacy_info;

free:
    free(legacy_info);
    free(info);
    return NULL;
}

struct sync_pt_info *sync_pt_info(struct sync_fence_info_data *info,
                                  struct sync_pt_info *itr)
{
    if (itr == NULL)
        itr = (struct sync_pt_info *) info->pt_info;
    else
        itr = (struct sync_pt_info *) ((__u8 *)itr + itr->len);

    if ((__u8 *)itr - (__u8 *)info >= (int)info->len)
        return NULL;

    return itr;
}

void sync_fence_info_free(struct sync_fence_info_data *info)
{
    free(info);
}


int sw_sync_timeline_create(void)
{
    int ret;

    ret = open("/sys/kernel/debug/sync/sw_sync", O_RDWR);
    if (ret < 0)
        ret = open("/dev/sw_sync", O_RDWR);

    return ret;
}

int sw_sync_timeline_inc(int fd, unsigned count)
{
    __u32 arg = count;

    return ioctl(fd, SW_SYNC_IOC_INC, &arg);
}

int sw_sync_fence_create(int fd, const char *name, unsigned value)
{
    struct sw_sync_create_fence_data data;
    int err;

    data.value = value;
    strlcpy(data.name, name, sizeof(data.name));

    err = ioctl(fd, SW_SYNC_IOC_CREATE_FENCE, &data);
    if (err < 0)
        return err;

    return data.fence;
}
