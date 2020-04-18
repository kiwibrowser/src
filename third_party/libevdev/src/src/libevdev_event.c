/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <libevdev/libevdev_event.h>

#include <errno.h>
#include <linux/input.h>
#include <stdbool.h>
#include <time.h>

#include <libevdev/libevdev.h>
#include <libevdev/libevdev_util.h>

#ifndef BTN_TOOL_QUINTTAP
#define BTN_TOOL_QUINTTAP  0x148  /* Five fingers on trackpad */
#endif

/* Set clockid to be used for timestamps */
#ifndef EVIOCSCLOCKID
#define EVIOCSCLOCKID  _IOW('E', 0xa0, int)
#endif

#ifndef EVIOCGMTSLOTS
#define EVIOCGMTSLOTS(len)  _IOC(_IOC_READ, 'E', 0x0a, len)
#endif

/* SYN_DROPPED added in kernel v2.6.38-rc4 */
#ifndef SYN_DROPPED
#define SYN_DROPPED  3
#endif

/* make VCSID as version number */
#ifndef VCSID
#define VCSID "Unknown"
#endif

static void Event_Clear_Ev_Rel_State(EvdevPtr);

static bool Event_Syn(EvdevPtr, struct input_event*);
static void Event_Syn_Report(EvdevPtr, struct input_event*);
static void Event_Syn_MT_Report(EvdevPtr, struct input_event*);

static void Event_Key(EvdevPtr, struct input_event*);

static void Event_Abs(EvdevPtr, struct input_event*);
static void Event_Abs_MT(EvdevPtr, struct input_event*);
static void Event_Abs_Update_Pressure(EvdevPtr, struct input_event*);

static void Event_Rel(EvdevPtr, struct input_event*);

static void Event_Get_Time(struct timeval*, bool);

static int Event_Is_Valid(struct input_event*);

const char*
Evdev_Get_Version() {
    return VCSID;
}

/**
 * Input Device Event Property accessors
 */
int
Event_Get_Left(EvdevPtr device)
{
    struct input_absinfo* absinfo = &device->info.absinfo[ABS_MT_POSITION_X];
    return absinfo->minimum;
}

int
Event_Get_Right(EvdevPtr device)
{
    struct input_absinfo* absinfo = &device->info.absinfo[ABS_MT_POSITION_X];
    return absinfo->maximum;
}

int
Event_Get_Top(EvdevPtr device)
{
    struct input_absinfo* absinfo = &device->info.absinfo[ABS_MT_POSITION_Y];
    return absinfo->minimum;
}

int
Event_Get_Bottom(EvdevPtr device)
{
    struct input_absinfo* absinfo = &device->info.absinfo[ABS_MT_POSITION_Y];
    return absinfo->maximum;
}

int
Event_Get_Res_Y(EvdevPtr device)
{
    struct input_absinfo* absinfo = &device->info.absinfo[ABS_MT_POSITION_Y];
    return absinfo->resolution;
}

int
Event_Get_Res_X(EvdevPtr device)
{
    struct input_absinfo* absinfo = &device->info.absinfo[ABS_MT_POSITION_X];
    return absinfo->resolution;
}

int
Event_Get_Orientation_Minimum(EvdevPtr device)
{
    struct input_absinfo* absinfo = &device->info.absinfo[ABS_MT_ORIENTATION];
    return absinfo->minimum;
}

int
Event_Get_Orientation_Maximum(EvdevPtr device)
{
    struct input_absinfo* absinfo = &device->info.absinfo[ABS_MT_ORIENTATION];
    return absinfo->maximum;
}

int
Event_Get_Button_Pad(EvdevPtr device)
{
    return TestBit(INPUT_PROP_BUTTONPAD, device->info.prop_bitmask);
}

int
Event_Get_Semi_MT(EvdevPtr device)
{
    return TestBit(INPUT_PROP_SEMI_MT, device->info.prop_bitmask);
}

int
Event_Get_T5R2(EvdevPtr device)
{
    EventStatePtr evstate = device->evstate;
    if (Event_Get_Semi_MT(device))
        return 0;
    return (Event_Get_Touch_Count_Max(device) > evstate->slot_count);
}

int
Event_Get_Touch_Count_Max(EvdevPtr device)
{

    if (TestBit(BTN_TOOL_QUINTTAP, device->info.key_bitmask))
        return 5;
    if (TestBit(BTN_TOOL_QUADTAP, device->info.key_bitmask))
        return 4;
    if (TestBit(BTN_TOOL_TRIPLETAP, device->info.key_bitmask))
        return 3;
    if (TestBit(BTN_TOOL_DOUBLETAP, device->info.key_bitmask))
        return 2;
    return 1;
}

int
Event_Get_Touch_Count(EvdevPtr device)
{

    if (TestBit(BTN_TOUCH, device->key_state_bitmask)) {
        if (TestBit(BTN_TOOL_QUINTTAP, device->key_state_bitmask))
            return 5;
        if (TestBit(BTN_TOOL_QUADTAP, device->key_state_bitmask))
            return 4;
        if (TestBit(BTN_TOOL_TRIPLETAP, device->key_state_bitmask))
            return 3;
        if (TestBit(BTN_TOOL_DOUBLETAP, device->key_state_bitmask))
            return 2;
        if (TestBit(BTN_TOOL_FINGER, device->key_state_bitmask))
            return 1;
    }
    return 0;
}

int
Event_Get_Slot_Count(EvdevPtr device)
{
    EventStatePtr evstate = device->evstate;
    return evstate->slot_count;
}

int
Event_Get_Button_Left(EvdevPtr device)
{
    return Event_Get_Button(device, BTN_LEFT);
}

int
Event_Get_Button_Middle(EvdevPtr device)
{
    return Event_Get_Button(device, BTN_MIDDLE);
}

int
Event_Get_Button_Right(EvdevPtr device)
{
    return Event_Get_Button(device, BTN_RIGHT);
}

int
Event_Get_Button(EvdevPtr device, int button)
{
    return TestBit(button, device->key_state_bitmask);
}

#define CASE_RETURN(s) \
    case (s):\
        return #s


const char *
Event_To_String(int type, int code) {
    switch (type) {
    case EV_SYN:
        switch (code) {
        CASE_RETURN(SYN_REPORT);
        CASE_RETURN(SYN_MT_REPORT);
        default:
            break;
        }
        break;
    case EV_ABS:
        switch (code) {
        CASE_RETURN(ABS_X);
        CASE_RETURN(ABS_Y);
        CASE_RETURN(ABS_Z);
        CASE_RETURN(ABS_PRESSURE);
        CASE_RETURN(ABS_TOOL_WIDTH);
        CASE_RETURN(ABS_MT_TOUCH_MAJOR);
        CASE_RETURN(ABS_MT_TOUCH_MINOR);
        CASE_RETURN(ABS_MT_WIDTH_MAJOR);
        CASE_RETURN(ABS_MT_WIDTH_MINOR);
        CASE_RETURN(ABS_MT_ORIENTATION);
        CASE_RETURN(ABS_MT_POSITION_X);
        CASE_RETURN(ABS_MT_POSITION_Y);
        CASE_RETURN(ABS_MT_TOOL_TYPE);
        CASE_RETURN(ABS_MT_BLOB_ID);
        CASE_RETURN(ABS_MT_TRACKING_ID);
        CASE_RETURN(ABS_MT_PRESSURE);
        CASE_RETURN(ABS_MT_SLOT);
        default:
            break;
        }
        break;
    case EV_REL:
        switch (code) {
        CASE_RETURN(REL_X);
        CASE_RETURN(REL_Y);
        CASE_RETURN(REL_WHEEL);
        CASE_RETURN(REL_HWHEEL);
        default:
            break;
        }
        break;
    case EV_KEY:
        switch (code) {
        CASE_RETURN(BTN_LEFT);
        CASE_RETURN(BTN_RIGHT);
        CASE_RETURN(BTN_MIDDLE);
        CASE_RETURN(BTN_BACK);
        CASE_RETURN(BTN_FORWARD);
        CASE_RETURN(BTN_EXTRA);
        CASE_RETURN(BTN_SIDE);
        CASE_RETURN(BTN_TOUCH);
        CASE_RETURN(BTN_TOOL_FINGER);
        CASE_RETURN(BTN_TOOL_DOUBLETAP);
        CASE_RETURN(BTN_TOOL_TRIPLETAP);
        CASE_RETURN(BTN_TOOL_QUADTAP);
        CASE_RETURN(BTN_TOOL_QUINTTAP);
        default:
            break;
        }
        break;
    default:
        break;
    }
    return "?";
}
#undef CASE_RETURN

const char *
Event_Type_To_String(int type) {
    switch (type) {
    case EV_SYN: return "SYN";
    case EV_KEY: return "KEY";
    case EV_REL: return "REL";
    case EV_ABS: return "ABS";
    case EV_MSC: return "MSC";
    case EV_SW: return "SW";
    case EV_LED: return "LED";
    case EV_SND: return "SND";
    case EV_REP: return "REP";
    case EV_FF: return "FF";
    case EV_PWR: return "PWR";
    default: return "?";
    }
}


/**
 * Probe Device Input Event Support
 */
int
Event_Init(EvdevPtr device)
{
    int i;
    EventStatePtr evstate;

    evstate = device->evstate;
    if (EvdevProbe(device) != Success) {
      return !Success;
    }

    for (i = ABS_X; i <= ABS_MAX; i++) {
        if (TestBit(i, device->info.abs_bitmask)) {
            struct input_absinfo* absinfo = &device->info.absinfo[i];
            if (i == ABS_MT_SLOT) {
                int rc;
                rc = MTB_Init(device, absinfo->minimum, absinfo->maximum,
                              absinfo->value);
                if (rc != Success)
                    return rc;
            } else if (IS_ABS_MT(i)) {
                evstate->mt_axes[MT_CODE(i)] = absinfo;
            }
        }
    }
    return Success;
}

void
Event_Free(EvdevPtr device)
{
    MT_Free(device);
}

void
Event_Open(EvdevPtr device)
{
    /* Select monotonic input event timestamps, if supported by kernel */
    device->info.is_monotonic = (EvdevEnableMonotonic(device) == Success);
    LOG_DEBUG(device, "Using %s input event time stamps\n",
              device->info.is_monotonic ? "monotonic" : "realtime");

    /* Synchronize all MT slots with kernel evdev driver */
    Event_Sync_State(device);
}

static void
Event_Get_Time(struct timeval *t, bool use_monotonic) {
    struct timespec now;
    clockid_t clockid = (use_monotonic) ? CLOCK_MONOTONIC : CLOCK_REALTIME;

    clock_gettime(clockid, &now);
    t->tv_sec = now.tv_sec;
    t->tv_usec = now.tv_nsec / 1000;
}

/**
 * Synchronize the current state with kernel evdev driver. For cmt, there are
 * only four components required to be synced: current touch count, the MT
 * slots information, current slot id and physical button states. However, as
 * pressure readings are missing in ABS_MT_PRESSURE field of MT slots for
 * semi_mt touchpad device (e.g. Cr48), we also need need to extract it with
 * extra EVIOCGABS query.
 */
void
Event_Sync_State(EvdevPtr device)
{
    int i;

    Event_Get_Time(&device->before_sync_time, device->info.is_monotonic);

    EvdevProbeKeyState(device);

    /* Get current pressure information for single-pressure device */
    if (EvdevIsSinglePressureDevice(device) == Success) {
        struct input_event ev;
        ev.code = ABS_PRESSURE;
        ev.value = device->info.absinfo[ABS_PRESSURE].value;
        Event_Abs_Update_Pressure(device, &ev);
    }

    /* TODO(cywang): Sync all ABS_ states for completeness */

    /* Get current MT information for each slot */
    for (i = _ABS_MT_FIRST; i <= _ABS_MT_LAST; i++) {
        MTSlotInfo req;

        if (!TestBit(i, device->info.abs_bitmask))
            continue;
        /*
         * TODO(cywang): Scale the size of slots in MTSlotInfo based on the
         *    evstate->slot_count.
         */

        req.code = i;
        if (EvdevProbeMTSlot(device, &req) != Success) {
            continue;
        }
        MT_Slot_Sync(device, &req);
    }

    /* Get current slot id for multi-touch devices*/
    if (TestBit(ABS_MT_SLOT, device->info.abs_bitmask) &&
        (EvdevProbeAbsinfo(device, ABS_MT_SLOT) == Success)) {
        MT_Slot_Set(device, device->info.absinfo[ABS_MT_SLOT].value);
    }

    Event_Get_Time(&device->after_sync_time, device->info.is_monotonic);

    /* Initialize EV_REL event state */
    Event_Clear_Ev_Rel_State(device);

    LOG_DEBUG(device, "Event_Sync_State: before %ld.%ld after %ld.%ld\n",
              (long)device->before_sync_time.tv_sec,
              (long)device->before_sync_time.tv_usec,
              (long)device->after_sync_time.tv_sec,
              (long)device->after_sync_time.tv_usec);
}

static void
Event_Print(EvdevPtr device, struct input_event* ev)
{
    switch (ev->type) {
    case EV_SYN:
        switch (ev->code) {
        case SYN_REPORT:
            LOG_DEBUG(device, "@ %ld.%06ld  ---------- SYN_REPORT -------\n",
                (long)ev->time.tv_sec, (long)ev->time.tv_usec);
            return;
        case SYN_MT_REPORT:
            LOG_DEBUG(device, "@ %ld.%06ld  ........ SYN_MT_REPORT ......\n",
                (long)ev->time.tv_sec, (long)ev->time.tv_usec);
            return;
        case SYN_DROPPED:
            LOG_WARNING(device, "@ %ld.%06ld  ++++++++ SYN_DROPPED ++++++++\n",
                (long)ev->time.tv_sec, (long)ev->time.tv_usec);
            return;
        default:
            LOG_WARNING(device, "@ %ld.%06ld  ?????? SYN_UNKNOWN (%d) ?????\n",
                (long)ev->time.tv_sec, (long)ev->time.tv_usec, ev->code);
            return;
        }
        break;
    case EV_ABS:
        if (ev->code == ABS_MT_SLOT) {
            LOG_DEBUG(device, "@ %ld.%06ld  .......... MT SLOT %d ........\n",
                (long)ev->time.tv_sec, (long)ev->time.tv_usec, ev->value);
            return;
        }
        break;
    default:
        break;
    }

    LOG_DEBUG(device, "@ %ld.%06ld %s[%d] (%s) = %d\n",
        (long)ev->time.tv_sec, (long)ev->time.tv_usec,
        Event_Type_To_String(ev->type), ev->code,
        Event_To_String(ev->type, ev->code), ev->value);
}

/**
 * Process Input Events.  It returns TRUE if SYN_DROPPED detected.
 */
bool
Event_Process(EvdevPtr device, struct input_event* ev)
{
    Event_Print(device, ev);
    if (Event_Is_Valid(ev)) {
        if (!(ev->type == EV_SYN && ev->code == SYN_REPORT))
            device->got_valid_event = 1;
    } else {
        return false;
    }

    switch (ev->type) {
    case EV_SYN:
        return Event_Syn(device, ev);

    case EV_KEY:
        Event_Key(device, ev);
        break;

    case EV_ABS:
        Event_Abs(device, ev);
        break;

    case EV_REL:
        Event_Rel(device, ev);
        break;

    default:
        break;
    }
    return false;
}

/**
 * Dump the log of input events to disk
 */
void
Event_Dump_Debug_Log(void* vinfo) {
    Event_Dump_Debug_Log_To(vinfo, "/var/log/xorg/cmt_input_events.dat");
}

void
Event_Dump_Debug_Log_To(void* vinfo, const char* filename)
{
    EvdevPtr device = (EvdevPtr) vinfo;
    size_t i;
    int ret;
    EventStatePtr evstate = device->evstate;

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        LOG_ERROR(device, "fopen() failed for debug log");
        return;
    }

    ret = EvdevWriteInfoToFile(fp, &device->info);
    if (ret <= 0) {
        LOG_ERROR(device, "EvdevWriteInfoToFile failed. Log without info.");
    }

    for (i = 0; i < DEBUG_BUF_SIZE; i++) {
        struct input_event *ev =
            &evstate->debug_buf[(evstate->debug_buf_tail + i) % DEBUG_BUF_SIZE];
        if (ev->time.tv_sec == 0 && ev->time.tv_usec == 0)
            continue;
        ret = EvdevWriteEventToFile(fp, ev);
        if (ret <= 0) {
            LOG_ERROR(device, "EvdevWriteEventToFile failed. Log is short.");
            break;
        }
    }
    fclose(fp);
}

/**
 * Clear Debug Buffer
 */
void
Event_Clear_Debug_Log(void* vinfo)
{
    EvdevPtr device = (EvdevPtr) vinfo;
    EventStatePtr evstate = device->evstate;

    memset(evstate->debug_buf, 0, sizeof(evstate->debug_buf));
    evstate->debug_buf_tail = 0;
}

/**
 * Clear EV_REL event state.  This function should be called after a EV_SYN
 * event is processed because EV_REL event state is not accumulative.
 */
static void
Event_Clear_Ev_Rel_State(EvdevPtr device)
{
    EventStatePtr evstate;

    evstate = device->evstate;
    evstate->rel_x = 0;
    evstate->rel_y = 0;
    evstate->rel_wheel = 0;
    evstate->rel_hwheel = 0;
}

/**
 * Process EV_SYN events.  It returns TRUE if SYN_DROPPED detected.
 */
static bool
Event_Syn(EvdevPtr device, struct input_event* ev)
{
    switch (ev->code) {
    case SYN_DROPPED:
        /*
         * Technically we don't need to call Event_Clear_Ev_Rel_State() here
         * because when SYN_DROPPED is detected, Event_Sync_State() will be
         * called and Event_Sync_State() will call Event_Clear_Ev_Rel_State()
         * to re-initialize EV_REL event state.
         */
        return true;
    case SYN_REPORT:
        Event_Syn_Report(device, ev);
        break;
    case SYN_MT_REPORT:
        Event_Syn_MT_Report(device, ev);
        break;
    }
    return false;
}

static void
Event_Syn_Report(EvdevPtr device, struct input_event* ev)
{
    EventStatePtr evstate = device->evstate;
    if (device->got_valid_event)
        device->syn_report(device->syn_report_udata, evstate, &ev->time);

    MT_Print_Slots(device);

    Event_Clear_Ev_Rel_State(device);
    device->got_valid_event = 0;
}

static void
Event_Syn_MT_Report(EvdevPtr device, struct input_event* ev)
{
    /* TODO(djkurtz): Handle MT-A */
}

static void
Event_Key(EvdevPtr device, struct input_event* ev)
{
    AssignBit(device->key_state_bitmask, ev->code, ev->value);
}

static void
Event_Abs_Update_Pressure(EvdevPtr device, struct input_event* ev)
{
    /*
     * Update all active slots with the same ABS_PRESSURE value if it is a
     * single-pressure device.
     */
    EventStatePtr evstate = device->evstate;

    for (int i = 0; i < evstate->slot_count; i++) {
        MtSlotPtr slot = &evstate->slots[i];
        slot->pressure = ev->value;
    }
}

static void
Event_Abs(EvdevPtr device, struct input_event* ev)
{
    if (ev->code == ABS_MT_SLOT)
        MT_Slot_Set(device, ev->value);
    else if (IS_ABS_MT(ev->code))
        Event_Abs_MT(device, ev);
    else if ((ev->code == ABS_PRESSURE) && EvdevIsSinglePressureDevice(device))
        Event_Abs_Update_Pressure(device, ev);
}

static void
Event_Abs_MT(EvdevPtr device, struct input_event* ev)
{
    EventStatePtr evstate = device->evstate;
    struct input_absinfo* axis = evstate->mt_axes[MT_CODE(ev->code)];
    MtSlotPtr slot = evstate->slot_current;

    if (axis == NULL) {
        LOG_WARNING(device, "ABS_MT[%02x] was not reported by this device\n",
                  ev->code);
        return;
    }

    /* Warn about out of range data, but don't ignore */
    if ((ev->code != ABS_MT_TRACKING_ID)
                    && ((ev->value < axis->minimum)
                        || (ev->value > axis->maximum))) {
      LOG_WARNING(device, "ABS_MT[%02x] = %d : value out of range [%d .. %d]\n",
                  ev->code, ev->value, axis->minimum, axis->maximum);
      /* Update the effective boundary as we already print out the warning */
      if (ev->value < axis->minimum)
          axis->minimum = ev->value;
      else if (ev->value > axis->maximum)
          axis->maximum = ev->value;
    }

    if (slot == NULL) {
        LOG_WARNING(device, "MT slot not set. Ignoring ABS_MT event\n");
        return;
    }

    MT_Slot_Value_Set(slot, ev->code, ev->value);
}

static void
Event_Rel(EvdevPtr device, struct input_event* ev)
{
    EventStatePtr evstate = device->evstate;

    switch (ev->code) {
    case REL_X:
        evstate->rel_x = ev->value;
        break;
    case REL_Y:
        evstate->rel_y = ev->value;
        break;
    case REL_WHEEL:
        evstate->rel_wheel = ev->value;
        break;
    case REL_HWHEEL:
        evstate->rel_hwheel = ev->value;
        break;
    }
}

static int Event_Is_Valid(struct input_event* ev)
{
    /* Key repeats are invalid. They're handled by X anyway */
    if (ev->type == EV_KEY &&
        ev->value == 2)
        return 0;
    return 1;
}
