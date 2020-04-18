/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#ifndef _LIBEVDEV_EVENT_H_
#define _LIBEVDEV_EVENT_H_

#include <libevdev/libevdev_log.h>
#include <libevdev/libevdev_mt.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 1 MiB debug buffer of struct input_event objects */
#define DEBUG_BUF_SIZE      65536


typedef struct {
    int slot_min;
    int slot_count;
    MtSlotPtr slots;
    MtSlotPtr slot_current;

    int rel_x;
    int rel_y;
    int rel_wheel;
    int rel_hwheel;

    struct input_absinfo* mt_axes[_ABS_MT_CNT];

    /* Log of recent input_event structs for debugging */
    struct input_event debug_buf[DEBUG_BUF_SIZE];
    size_t debug_buf_tail;
} EventStateRec, *EventStatePtr;

int Event_Init(EvdevPtr);
void Event_Free(EvdevPtr);
void Event_Open(EvdevPtr);
bool Event_Process(EvdevPtr, struct input_event*);
void Event_Dump_Debug_Log(void *);
void Event_Dump_Debug_Log_To(void *, const char*);
void Event_Clear_Debug_Log(void *);

int Event_Get_Left(EvdevPtr);
int Event_Get_Right(EvdevPtr);
int Event_Get_Top(EvdevPtr);
int Event_Get_Bottom(EvdevPtr);
int Event_Get_Res_Y(EvdevPtr);
int Event_Get_Res_X(EvdevPtr);
int Event_Get_Orientation_Minimum(EvdevPtr);
int Event_Get_Orientation_Maximum(EvdevPtr);
int Event_Get_Button_Pad(EvdevPtr);
int Event_Get_Semi_MT(EvdevPtr);
int Event_Get_T5R2(EvdevPtr);
int Event_Get_Touch_Count(EvdevPtr);
int Event_Get_Touch_Count_Max(EvdevPtr);
int Event_Get_Slot_Count(EvdevPtr);
int Event_Get_Button_Left(EvdevPtr);
int Event_Get_Button_Middle(EvdevPtr);
int Event_Get_Button_Right(EvdevPtr);
int Event_Get_Button(EvdevPtr, int button);
void Event_Sync_State(EvdevPtr);
const char* Event_To_String(int type, int code);
const char* Event_Type_To_String(int type);
const char* Evdev_Get_Version();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
