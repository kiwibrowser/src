/*
 *  sync_test.c
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

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sync/sync.h>
#include "sw_sync.h"

pthread_mutex_t printf_mutex = PTHREAD_MUTEX_INITIALIZER;

struct sync_thread_data {
    int thread_no;
    int fd[2];
};

void *sync_thread(void *data)
{
    struct sync_thread_data *sync_data = data;
    struct sync_fence_info_data *info;
    int err;
    int i;

    for (i = 0; i < 2; i++) {
        err = sync_wait(sync_data->fd[i], 10000);

        pthread_mutex_lock(&printf_mutex);
        if (err < 0) {
            printf("thread %d wait %d failed: %s\n", sync_data->thread_no,
                   i, strerror(errno));
        } else {
            printf("thread %d wait %d done\n", sync_data->thread_no, i);
        }
        info = sync_fence_info(sync_data->fd[i]);
        if (info) {
            struct sync_pt_info *pt_info = NULL;
            printf("  fence %s %d\n", info->name, info->status);

            while ((pt_info = sync_pt_info(info, pt_info))) {
                int ts_sec = pt_info->timestamp_ns / 1000000000LL;
                int ts_usec = (pt_info->timestamp_ns % 1000000000LL) / 1000LL;
                printf("    pt %s %s %d %d.%06d", pt_info->obj_name,
                       pt_info->driver_name, pt_info->status,
                       ts_sec, ts_usec);
                if (!strcmp(pt_info->driver_name, "sw_sync"))
                    printf(" val=%d\n", *(uint32_t *)pt_info->driver_data);
                else
                    printf("\n");
            }
            sync_fence_info_free(info);
        }
        pthread_mutex_unlock(&printf_mutex);
    }

    return NULL;
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
    struct sync_thread_data sync_data[4];
    pthread_t threads[4];
    int sync_timeline_fd;
    int i, j;
    char str[256];

    sync_timeline_fd = sw_sync_timeline_create();
    if (sync_timeline_fd < 0) {
        perror("can't create sw_sync_timeline:");
        return 1;
    }

    for (i = 0; i < 3; i++) {
        sync_data[i].thread_no = i;

        for (j = 0; j < 2; j++) {
            unsigned val = i + j * 3 + 1;
            snprintf(str, sizeof(str), "test_fence%d-%d", i, j);
            int fd = sw_sync_fence_create(sync_timeline_fd, str, val);
            if (fd < 0) {
                printf("can't create sync pt %d: %s", val, strerror(errno));
                return 1;
            }
            sync_data[i].fd[j] = fd;
            printf("sync_data[%d].fd[%d] = %d;\n", i, j, fd);

        }
    }

    sync_data[3].thread_no = 3;
    for (j = 0; j < 2; j++) {
        snprintf(str, sizeof(str), "merged_fence%d", j);
        sync_data[3].fd[j] = sync_merge(str, sync_data[0].fd[j], sync_data[1].fd[j]);
        if (sync_data[3].fd[j] < 0) {
            printf("can't merge sync pts %d and %d: %s\n",
                   sync_data[0].fd[j], sync_data[1].fd[j], strerror(errno));
            return 1;
        }
    }

    for (i = 0; i < 4; i++)
        pthread_create(&threads[i], NULL, sync_thread, &sync_data[i]);


    for (i = 0; i < 3; i++) {
        int err;
        printf("press enter to inc to %d\n", i+1);
        fgets(str, sizeof(str), stdin);
        err = sw_sync_timeline_inc(sync_timeline_fd, 1);
        if (err < 0) {
            perror("can't increment sync obj:");
            return 1;
        }
    }

    printf("press enter to close sync_timeline\n");
    fgets(str, sizeof(str), stdin);

    close(sync_timeline_fd);

    printf("press enter to end test\n");
    fgets(str, sizeof(str), stdin);

    for (i = 0; i < 3; i++) {
        void *val;
        pthread_join(threads[i], &val);
    }

    return 0;
}
