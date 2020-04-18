/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef _LIBEVDEV_LOG_H_
#define _LIBEVDEV_LOG_H_

#include <linux/input.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Evdev_;
typedef struct Evdev_ *EvdevPtr;

#define LOGLEVEL_DEBUG 0
#define LOGLEVEL_WARNING 1
#define LOGLEVEL_ERROR 2

#define LOG_DEBUG(evdev, format, ...) if ((evdev)->log) \
    (evdev)->log((evdev)->log_udata, LOGLEVEL_DEBUG, \
    "%s():%d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)


#define LOG_ERROR(evdev, format, ...) if ((evdev)->log) \
    (evdev)->log((evdev)->log_udata, LOGLEVEL_ERROR, \
    "%s():%d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define LOG_WARNING(evdev, format, ...) if ((evdev)->log) \
    (evdev)->log((evdev)->log_udata, LOGLEVEL_WARNING, \
    "%s():%d: " format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
