/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef _LIBEVDEV_MT_H_
#define _LIBEVDEV_MT_H_

#include <stdint.h>
#include <libevdev/libevdev_log.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * From include/linux/input.h
 * (as per linux-2.6.git:771d6d85667d68a17c24c452979f8d37cc628082)
 */
#define _ABS_MT_FIRST       ABS_MT_TOUCH_MAJOR
#define _ABS_MT_LAST        ABS_MT_DISTANCE
#define _ABS_MT_CNT         (_ABS_MT_LAST - _ABS_MT_FIRST + 1)

#define IS_ABS_MT(c)        (((c) >= _ABS_MT_FIRST) && ((c) <= _ABS_MT_LAST))
#define MT_CODE(c)          ((c) - _ABS_MT_FIRST)

#define MAX_SLOT_COUNT  64

typedef struct {
    uint32_t code;
    int32_t values[MAX_SLOT_COUNT];
} MTSlotInfo, *MTSlotInfoPtr;

typedef struct {
    int touch_major;        /* Major axis of touching ellipse */
    int touch_minor;        /* Minor axis (omit if circular) */
    int width_major;        /* Major axis of approaching ellipse */
    int width_minor;        /* Minor axis (omit if circular) */
    int orientation;        /* Ellipse orientation */
    int position_x;         /* Center X ellipse position */
    int position_y;         /* Center Y ellipse position */
    int tool_type;          /* Type of touching device */
    int blob_id;            /* Group a set of packets as a blob */
    int tracking_id;        /* Unique ID of initiated contact */
    int pressure;           /* Pressure on contact area */
    int distance;           /* Contact hover distance */
} MtSlotRec, *MtSlotPtr;

int MTB_Init(EvdevPtr, int, int, int);
void MT_Free(EvdevPtr);

void MT_Print_Slots(EvdevPtr);
void MT_Slot_Set(EvdevPtr, int);

int MT_Slot_Value_Get(MtSlotPtr, int);
void MT_Slot_Value_Set(MtSlotPtr, int, int);

void MT_Slot_Sync(EvdevPtr, MTSlotInfoPtr);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
