/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <libevdev/libevdev.h>

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <libevdev/libevdev_event.h>
#include <libevdev/libevdev_log.h>
#include <libevdev/libevdev_util.h>

/* Number of events to attempt to read from kernel on each SIGIO */
#define NUM_EVENTS          16

#ifndef EVIOCGMTSLOTS
#define EVIOCGMTSLOTS(len)  _IOC(_IOC_READ, 'E', 0x0a, len)
#endif

/* Set clockid to be used for timestamps */
#ifndef EVIOCSCLOCKID
#define EVIOCSCLOCKID  _IOW('E', 0xa0, int)
#endif

static int EvdevWriteBitmask(FILE* fp, const char* name,
                             unsigned long* bitmask, size_t num_bytes);
static int EvdevReadBitmask(FILE* fp, const char* expected_name,
                            unsigned long* bitmask, size_t num_bytes);

#ifndef EVDEV_HOLLOW

static void Absinfo_Print(EvdevPtr device, struct input_absinfo*);
static const char* Event_Property_To_String(int type);
static EvdevClass EvdevProbeClass(EvdevInfoPtr info);
static const char* EvdevClassToString(EvdevClass cls);

int EvdevOpen(EvdevPtr evdev, const char* device) {
  evdev->fd = open(device, O_RDWR | O_NONBLOCK, 0);
  return evdev->fd;
}

int EvdevClose(EvdevPtr evdev) {
  close(evdev->fd);
  evdev->fd = -1;
  return evdev->fd;
}

int EvdevRead(EvdevPtr evdev) {
  struct input_event ev[NUM_EVENTS];
  EventStatePtr evstate = evdev->evstate;
  int i;
  int len;
  int status = Success;
  bool sync_evdev_state = false;

  do {
    len = read(evdev->fd, &ev, sizeof(ev));
    if (len < 0) {
      if (errno != EINTR && errno != EAGAIN)
        status = errno;
      break;
    }

    /* Read as many whole struct input_event objects as we can into the
       circular buffer */
    for (i = 0; i < len / sizeof(*ev); i++) {
      evstate->debug_buf[evstate->debug_buf_tail] = ev[i];
      evstate->debug_buf_tail =
          (evstate->debug_buf_tail + 1) % DEBUG_BUF_SIZE;
    }

    /* kernel always delivers complete events, so len must be sizeof *ev */
    if (len % sizeof(*ev))
      break;

    /* Process events ... */
    for (i = 0; i < len / sizeof(ev[0]); i++) {
      if (sync_evdev_state)
        break;
      if (timercmp(&ev[i].time, &evdev->before_sync_time, <)) {
        /* Ignore events before last sync time */
        continue;
      } else if (timercmp(&ev[i].time, &evdev->after_sync_time, >)) {
        /* Event_Process returns TRUE if SYN_DROPPED detected */
        sync_evdev_state |= Event_Process(evdev, &ev[i]);
      } else {
        /* If the event occurred during sync, then sync again */
        sync_evdev_state = true;
      }
    }

  } while (len == sizeof(ev));
  /* Keep reading if kernel supplied NUM_EVENTS events. */

  if (sync_evdev_state)
    Event_Sync_State(evdev);

  return status;
}

int EvdevProbe(EvdevPtr device) {
  int len, i;
  int fd;
  EvdevInfoPtr info;

  fd = device->fd;
  info = &device->info;
  if (ioctl(fd, EVIOCGID, &info->id) < 0) {
       LOG_ERROR(device, "ioctl EVIOCGID failed: %s\n", strerror(errno));
       return !Success;
  }

  if (ioctl(fd, EVIOCGNAME(sizeof(info->name) - 1),
            info->name) < 0) {
      LOG_ERROR(device, "ioctl EVIOCGNAME failed: %s\n", strerror(errno));
      return !Success;
  }

  len = ioctl(fd, EVIOCGPROP(sizeof(info->prop_bitmask)),
              info->prop_bitmask);
  if (len < 0) {
      LOG_ERROR(device, "ioctl EVIOCGPROP failed: %s\n", strerror(errno));
      return !Success;
  }
  for (i = 0; i < len*8; i++) {
      if (TestBit(i, info->prop_bitmask))
          LOG_DEBUG(device, "Has Property: %d (%s)\n", i,
                    Event_Property_To_String(i));
  }

  len = ioctl(fd, EVIOCGBIT(0, sizeof(info->bitmask)),
              info->bitmask);
  if (len < 0) {
      LOG_ERROR(device, "ioctl EVIOCGBIT failed: %s\n",
                strerror(errno));
      return !Success;
  }
  for (i = 0; i < len*8; i++) {
      if (TestBit(i, info->bitmask))
          LOG_DEBUG(device, "Has Event Type %d = %s\n", i,
                    Event_Type_To_String(i));
  }

  len = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(info->key_bitmask)),
              info->key_bitmask);
  if (len < 0) {
      LOG_ERROR(device, "ioctl EVIOCGBIT(EV_KEY) failed: %s\n",
                strerror(errno));
      return !Success;
  }
  for (i = 0; i < len*8; i++) {
      if (TestBit(i, info->key_bitmask))
          LOG_DEBUG(device, "Has KEY[%d] = %s\n", i,
                    Event_To_String(EV_KEY, i));
  }

  len = ioctl(fd, EVIOCGBIT(EV_LED, sizeof(info->led_bitmask)),
              info->led_bitmask);
  if (len < 0) {
      LOG_ERROR(device, "ioctl EVIOCGBIT(EV_LED) failed: %s\n",
                strerror(errno));
      return !Success;
  }
  for (i = 0; i < len*8; i++) {
      if (TestBit(i, info->led_bitmask))
          LOG_DEBUG(device, "Has LED[%d] = %s\n", i,
                    Event_To_String(EV_LED, i));
  }

  len = ioctl(fd, EVIOCGBIT(EV_REL, sizeof(info->rel_bitmask)),
              info->rel_bitmask);
  if (len < 0) {
      LOG_ERROR(device, "ioctl EVIOCGBIT(EV_REL) failed: %s\n",
                strerror(errno));
      return !Success;
  }
  for (i = 0; i < len*8; i++) {
      if (TestBit(i, info->rel_bitmask))
          LOG_DEBUG(device, "Has REL[%d] = %s\n", i,
                    Event_To_String(EV_REL, i));
  }

  len = ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(info->abs_bitmask)),
              info->abs_bitmask);
  if (len < 0) {
      LOG_ERROR(device, "ioctl EVIOCGBIT(EV_ABS) failed: %s\n",
                strerror(errno));
      return !Success;
  }

  for (i = ABS_X; i <= ABS_MAX; i++) {
      if (TestBit(i, info->abs_bitmask)) {
          struct input_absinfo* absinfo = &info->absinfo[i];
          LOG_DEBUG(device, "Has ABS[%d] = %s\n", i,
                    Event_To_String(EV_ABS, i));
          len = ioctl(fd, EVIOCGABS(i), absinfo);
          if (len < 0) {
              LOG_ERROR(device, "ioctl EVIOCGABS(%d) failed: %s\n", i,
                  strerror(errno));
              return !Success;
          }

          Absinfo_Print(device, absinfo);
      }
  }

  info->evdev_class = EvdevProbeClass(info);
  LOG_DEBUG(device, "Has evdev device class = %s\n",
            EvdevClassToString(info->evdev_class));
  if (info->evdev_class == EvdevClassUnknown) {
    LOG_ERROR(device, "Couldn't determine evdev class\n");
    return !Success;
  }

  return Success;
}

int EvdevProbeAbsinfo(EvdevPtr device, size_t key) {
  struct input_absinfo* absinfo;

  absinfo = &device->info.absinfo[key];
  if (ioctl(device->fd, EVIOCGABS(key), absinfo) < 0) {
      LOG_ERROR(device, "ioctl EVIOCGABS(%zu) failed: %s\n", key,
                strerror(errno));
      return !Success;
  } else {
      return Success;
  }
}

int EvdevProbeMTSlot(EvdevPtr device, MTSlotInfoPtr req) {
  if (ioctl(device->fd, EVIOCGMTSLOTS((sizeof(*req))), req) < 0) {
      LOG_ERROR(device, "ioctl EVIOCGMTSLOTS(req.code=%d) failed: %s\n",
                req->code, strerror(errno));
      return !Success;
  } else {
      return Success;
  }
}

int EvdevProbeKeyState(EvdevPtr device) {
  int len = sizeof(device->key_state_bitmask);

  memset(device->key_state_bitmask, 0, len);
  if (ioctl(device->fd, EVIOCGKEY(len), device->key_state_bitmask) < 0) {
      LOG_ERROR(device, "ioctl EVIOCGKEY failed: %s\n", strerror(errno));
      return !Success;
  } else {
      return Success;
  }
}

int EvdevEnableMonotonic(EvdevPtr device) {
  unsigned int clk = CLOCK_MONOTONIC;
  return (ioctl(device->fd, EVIOCSCLOCKID, &clk) == 0) ? Success : !Success;
}

#endif // #ifndef EVDEV_HOLLOW

/*
 * Check if the device is a single-pressure one which reports ABS_PRESSURE only.
 */
int EvdevIsSinglePressureDevice(EvdevPtr device) {
    EvdevInfoPtr info = &device->info;

    return (!TestBit(ABS_MT_PRESSURE, info->abs_bitmask) &&
            TestBit(ABS_PRESSURE, info->abs_bitmask));
}

int EvdevWriteInfoToFile(FILE* fp, const EvdevInfoPtr info) {
  int ret;

  ret = fprintf(fp, "# device: %s\n", info->name);
  if (ret <= 0)
    return ret;

  ret = EvdevWriteBitmask(fp, "bit", info->bitmask, sizeof(info->bitmask));
  if (ret <= 0)
    return ret;

  ret = EvdevWriteBitmask(fp, "key", info->key_bitmask,
                          sizeof(info->key_bitmask));
  if (ret <= 0)
    return ret;

  ret = EvdevWriteBitmask(fp, "rel", info->rel_bitmask,
                          sizeof(info->rel_bitmask));
  if (ret <= 0)
    return ret;

  ret = EvdevWriteBitmask(fp, "abs", info->abs_bitmask,
                          sizeof(info->abs_bitmask));
  if (ret <= 0)
    return ret;

  ret = EvdevWriteBitmask(fp, "led", info->led_bitmask,
                          sizeof(info->led_bitmask));
  if (ret <= 0)
    return ret;

  ret = EvdevWriteBitmask(fp, "prp", info->prop_bitmask,
                          sizeof(info->prop_bitmask));
  if (ret <= 0)
    return ret;

  // when reading the log we need to know which absinfos
  // exist which is stored in the abs bitmask.
  // so we have to write absinfo after the bitmasks.
  for (int i = ABS_X; i <= ABS_MAX; i++) {
    if (TestBit(i, info->abs_bitmask)) {
      struct input_absinfo* abs = &info->absinfo[i];
      ret = fprintf(fp, "# absinfo: %d %d %d %d %d %d\n",
                    i, abs->minimum, abs->maximum,
                    abs->fuzz, abs->flat, abs->resolution);
      if (ret <= 0)
        return ret;
    }
  }

  return 1;
}

int EvdevWriteEventToFile(FILE* fp, const struct input_event* ev) {
  return fprintf(fp, "E: %ld.%06u %04x %04x %d\n",
                 (long)ev->time.tv_sec, (unsigned)ev->time.tv_usec,
                 ev->type, ev->code, ev->value);
}

int EvdevReadInfoFromFile(FILE* fp, EvdevInfoPtr info) {
  int ret;

  ret = fscanf(fp, "# device: %1024[^\n]\n", info->name);
  if (ret <= 0)
    return ret;

  ret = EvdevReadBitmask(fp, "bit", info->bitmask, sizeof(info->bitmask));
  if (ret <= 0) {
    fprintf(stderr, "Parse device bit failed\n");
    return ret;
  }

  ret = EvdevReadBitmask(fp, "key", info->key_bitmask,
                         sizeof(info->key_bitmask));
  if (ret <= 0) {
    fprintf(stderr, "Parse device key failed\n");
    return ret;
  }

  ret = EvdevReadBitmask(fp, "rel", info->rel_bitmask,
                         sizeof(info->rel_bitmask));
  if (ret <= 0) {
    fprintf(stderr, "Parse device rel failed\n");
    return ret;
  }

  ret = EvdevReadBitmask(fp, "abs", info->abs_bitmask,
                         sizeof(info->abs_bitmask));
  if (ret <= 0) {
    fprintf(stderr, "Parse device abs failed\n");
    return ret;
  }

  ret = EvdevReadBitmask(fp, "led", info->led_bitmask,
                         sizeof(info->led_bitmask));
  if (ret <= 0) {
    fprintf(stderr, "Parse device led failed\n");
    return ret;
  }

  ret = EvdevReadBitmask(fp, "prp", info->prop_bitmask,
                         sizeof(info->prop_bitmask));
  if (ret <= 0) {
    fprintf(stderr, "Parse device prp failed\n");
    return ret;
  }

  for (int i = ABS_X; i <= ABS_MAX; i++) {
      if (TestBit(i, info->abs_bitmask)) {
          struct input_absinfo abs;
          int abs_index;
          ret = fscanf(fp, "# absinfo: %d %d %d %d %d %d\n",
                       &abs_index, &abs.minimum, &abs.maximum,
                       &abs.fuzz, &abs.flat, &abs.resolution);
          if (ret <= 0 || abs_index != i) {
            fprintf(stderr, "Parse device absinfo failed on %d\n", i);
            return -1;
          }
          info->absinfo[i] = abs;
      }
  }
  return 1;
}

int EvdevReadEventFromFile(FILE* fp, struct input_event* ev) {
  unsigned long sec;
  unsigned usec, type, code;
  int value;
  int ret = fscanf(fp, "E: %lu.%06u %04x %04x %d\n",
       &sec, &usec, &type, &code, &value);
  if (ret == EOF)
    return -1;
  if (ret <= 0) {
    int err = errno;
    fprintf(stderr, "EvdevReadEventFromFile failed: %d, errno: %d\n", ret, err);
    return ret;
  }

  ev->time.tv_sec = sec;
  ev->time.tv_usec = usec;
  ev->type = type;
  ev->code = code;
  ev->value = value;
  return ret;
}

static int EvdevReadBitmask(FILE* fp, const char* expected_name,
                              unsigned long* bitmask, size_t num_bytes) {
  unsigned char* bytes = (unsigned char*)bitmask;
  int ret;
  char name[64] = { 0 };

  ret = fscanf(fp, "# %63[^:]:", name);
  if (ret <= 0 || strcmp(name, expected_name)) {
    fprintf(stderr, "EvdevReadBitmask: %d <= 0 or [%s]!=[%s]\n",
            ret, name, expected_name);
    return -1;
  }
  memset(bitmask, 0, num_bytes);
  for (size_t i = 0; i < num_bytes; ++i) {
    // Make sure we don't go off the end of the line
    int next_char = fgetc(fp);
    if (next_char != EOF)
      ungetc(next_char, fp);
    if (next_char == '\n' || next_char == EOF)
      break;

    unsigned int tmp;
    ret = fscanf(fp, " %02X", &tmp);
    if (ret <= 0)
      bytes[i] = 0;
    else
      bytes[i] = (unsigned char)tmp;
  }
  // size(in bytes) of bitmask array may differs per platform, try to read
  // remaining bytes if exists
  do {
    if (fgets(name, sizeof(name), fp) == NULL)
      return -1;
  } while (name[strlen(name) - 1] != '\n');
  return 1;
}

static int EvdevWriteBitmask(FILE* fp, const char* name,
                              unsigned long* bitmask, size_t num_bytes) {
  int ret;

  unsigned char* bytes = (unsigned char*)bitmask;
  ret = fprintf(fp, "# %s:", name);
  if (ret <= 0)
    return ret;

  for (int i = 0; i < num_bytes; ++i) {
    ret = fprintf(fp, " %02X", bytes[i]);
    if (ret <= 0)
      return ret;
  }
  ret = fprintf(fp, "\n");
  return ret;
}

#ifndef EVDEV_HOLLOW

static const char*
Event_Property_To_String(int type) {
    switch (type) {
    case INPUT_PROP_POINTER: return "POINTER";      /* needs a pointer */
    case INPUT_PROP_DIRECT: return "DIRECT";        /* direct input devices */
    case INPUT_PROP_BUTTONPAD: return "BUTTONPAD";  /* has button under pad */
    case INPUT_PROP_SEMI_MT: return "SEMI_MT";      /* touch rectangle only */
    default: return "?";
    }
}

static void
Absinfo_Print(EvdevPtr device, struct input_absinfo* absinfo)
{
    LOG_DEBUG(device, "    min = %d\n", absinfo->minimum);
    LOG_DEBUG(device, "    max = %d\n", absinfo->maximum);
    if (absinfo->fuzz)
        LOG_DEBUG(device, "    fuzz = %d\n", absinfo->fuzz);
    if (absinfo->resolution)
        LOG_DEBUG(device, "    res = %d\n", absinfo->resolution);
}

/*
 * Heuristics for determining evdev device class; similar to those of
 * xf86-input-evdev.
 */
static EvdevClass EvdevProbeClass(EvdevInfoPtr info) {
  int bit;

  if (TestBit(REL_X, info->rel_bitmask) &&
      TestBit(REL_Y, info->rel_bitmask)) {
    if (TestBit(ABS_MT_POSITION_X, info->abs_bitmask) &&
        TestBit(ABS_MT_POSITION_Y, info->abs_bitmask))
      return EvdevClassMultitouchMouse;
    else
      return EvdevClassMouse;
  }

  if (TestBit(ABS_X, info->abs_bitmask) &&
      TestBit(ABS_Y, info->abs_bitmask)) {

    if (TestBit(BTN_TOOL_PEN, info->key_bitmask) ||
        TestBit(BTN_STYLUS, info->key_bitmask) ||
        TestBit(BTN_STYLUS2, info->key_bitmask))
      return EvdevClassTablet;

    if (TestBit(ABS_PRESSURE, info->abs_bitmask) ||
        TestBit(BTN_TOUCH, info->key_bitmask)) {
      if (TestBit(BTN_LEFT, info->key_bitmask) ||
          TestBit(BTN_MIDDLE, info->key_bitmask) ||
          TestBit(BTN_RIGHT, info->key_bitmask) ||
          TestBit(BTN_TOOL_FINGER, info->key_bitmask))
        return EvdevClassTouchpad;
      else
        return EvdevClassTouchscreen;
    }

    /* Some touchscreens use BTN_LEFT rather than BTN_TOUCH */
    if (TestBit(BTN_LEFT, info->key_bitmask))
      return EvdevClassTouchscreen;
  }

  for (bit = 0; bit < BTN_MISC; bit++)
    if (TestBit(bit, info->key_bitmask))
      return EvdevClassKeyboard;

  return EvdevClassUnknown;
}

static const char* EvdevClassToString(EvdevClass cls) {
  switch (cls) {
    case EvdevClassUnknown:     return "EvdevClassUnknown";
    case EvdevClassKeyboard:    return "EvdevClassKeyboard";
    case EvdevClassMouse:       return "EvdevClassMouse";
    case EvdevClassMultitouchMouse: return "EvdevClassMultitouchMouse";
    case EvdevClassTablet:      return "EvdevClassTablet";
    case EvdevClassTouchpad:    return "EvdevClassTouchpad";
    case EvdevClassTouchscreen: return "EvdevClassTouchscreen";
  }
  return "Unhandled Evdev Class";
}

#endif // #ifndef EVDEV_HOLLOW
