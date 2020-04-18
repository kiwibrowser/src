// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/input.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XIproto.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)((array[LONG(bit)] >> OFF(bit)) & 1)

const char kDevInputEvent[] = "/dev/input";
const char kEventDevName[] = "event";
const char kTapPausedName[] = "Tap Paused";

using std::string;

namespace lid_touchpad_helper {

void PerrorAbort(const char* err) __attribute__((noreturn));

void PerrorAbort(const char* err) {
  perror(err);
  exit(1);
}

class XNotifier {
 public:
  XNotifier(Display* display, int tp_id) : display_(display), dev_(NULL) {
    if (!display_) {
      fprintf(stderr, "Unable to connect to X server.\n");
      exit(1);
    }
    dev_ = XOpenDevice(display_, tp_id);
    prop_tap_paused_ = XInternAtom(display_, kTapPausedName, True);
  }
  ~XNotifier() {
    if (dev_) {
      XCloseDevice(display_, dev_);
      dev_ = NULL;
    }
  }

  bool NotifyLidEvent(int lid_close) {
    if (!dev_)
      return false;
    unsigned char pause_tap = lid_close ? 1 : 0;
    XChangeDeviceProperty(display_, dev_, prop_tap_paused_, XA_INTEGER, 8,
                          PropModeReplace, &pause_tap, 1);
    XSync(display_, False);
    return true;
  }

 private:
  Display *display_;
  XDevice* dev_;
  Atom prop_tap_paused_;
};

bool SupportLidEvent(int fd) {
  unsigned long events[EV_MAX];
  memset(events, 0, sizeof(events));
  if (ioctl(fd, EVIOCGBIT(0, EV_MAX), events) < 0) {
    fprintf(stderr, "error in ioctl on fd %d for events list\n", fd);
    return false;
  }

  if (test_bit(EV_SW, events)) {
    unsigned long switch_events[NBITS(SW_LID + 1)];
    memset(switch_events, 0, sizeof(switch_events));
    if (ioctl(fd, EVIOCGBIT(EV_SW, SW_LID + 1), switch_events) < 0) {
      fprintf(stderr, "Error in ioctl on fd %d for switch_events", fd);
      return false;
    }
    if (test_bit(SW_LID, switch_events))
      return true;
  }

  return false;
}

int IsEventDevice(const struct dirent *dir) {
  return strncmp(kEventDevName, dir->d_name, sizeof(kEventDevName) - 1) == 0;
}

int GetLidFd() {
  struct dirent **namelist;
  int ret = -1;
  int ndev = scandir(kDevInputEvent, &namelist, IsEventDevice, alphasort);
  if (ndev <= 0)
    return ret;

  for (int i = 0; i < ndev; i++) {
    char fname[64];
    snprintf(fname, sizeof(fname),
            "%s/%s", kDevInputEvent, namelist[i]->d_name);
    int fd = open(fname, O_RDONLY);
    if (fd < 0)
      continue;
    if (SupportLidEvent(fd)) {
      ret = fd;
      break;
    }
    close(fd);
  }

  for (int i = 0; i < ndev; i++)
    free(namelist[i]);
  free(namelist);

  return ret;
}

bool IsCmtTouchpad(Display* display, int device_id) {
  Atom prop = XInternAtom(display, kTapPausedName, True);
  // Fail quick if server does not have Atom
  if (prop == None)
    return false;

  Atom actual_type;
  int actual_format;
  unsigned long num_items, bytes_after;
  unsigned char* data;

  if (XIGetProperty(display, device_id, prop, 0, 1000, False, AnyPropertyType,
                    &actual_type, &actual_format, &num_items, &bytes_after,
                    &data) != Success) {
    // XIGetProperty can generate BadAtom, BadValue, and BadWindow errors
    fprintf(stderr, "Mysterious X server error\n");
    return false;
  }
  if (actual_type == None)
    return false;
  XFree(data);
  return (actual_type == XA_INTEGER);
}

int GetTouchpadXId(Display* display) {
  int num_devices = 0;
  XIDeviceInfo* info = XIQueryDevice(display, XIAllDevices, &num_devices);
  if (!info)
    PerrorAbort("XIQueryDevice");
  int ret = -1;
  for (int i = 0; i < num_devices; i++) {
    XIDeviceInfo* device = &info[i];
    if (device->use != XISlavePointer)
      continue;
    if (!IsCmtTouchpad(display, device->deviceid))
      continue;
    ret = device->deviceid;
    break;
  }
  XIFreeDeviceInfo(info);
  return ret;
}

bool HandleInput(int fd, XNotifier* notifier) {
  const size_t kNumEvents = 16;  // Max events to read per read call.
  struct input_event events[kNumEvents];
  const size_t kEventSize = sizeof(events[0]);
  while (true) {
    ssize_t readlen = read(fd, events, sizeof(events));
    if (readlen < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return true;
      if (errno == ENODEV)
        return false;  // Device is gone. Try to reopen
      PerrorAbort("read");
    }
    if (readlen == 0) {
      // EOF. Reopen device.
      return false;
    }
    if (readlen % kEventSize) {
      fprintf(stderr, "Read %zd bytes, not a multiple of %zu.\n",
              readlen, kEventSize);
      exit(1);
    }
    for (size_t i = 0, cnt = readlen / kEventSize; i < cnt; i++) {
      const struct input_event& event = events[i];
      if (event.type == EV_SW && event.code == SW_LID) {
        if (!notifier->NotifyLidEvent(event.value))
          return false;
      }
    }
  }
  return true;
}

class MainLoop {
 public:
  MainLoop(Display* display)
      : display_(display),
        tp_id_(-1),
        lid_fd_(-1),
        x_fd_(ConnectionNumber(display_)),
        xiopcode_(GetXInputOpCode()) {}
  void Run() __attribute__ ((noreturn)) {
    XIEventMask evmask;
    unsigned char mask[XIMaskLen(XI_LASTEVENT)] = {0};

    XISetMask(mask, XI_HierarchyChanged);
    evmask.deviceid = XIAllDevices;
    evmask.mask_len = sizeof(mask);
    evmask.mask = mask;

    XISelectEvents(display_, DefaultRootWindow(display_), &evmask, 1);

    while (true) {
      if (DevicesNeedRefresh()) {
        sleep(1);
        RefreshDevices();
        continue;
      }
      int max_fd_plus_1 = std::max(x_fd_, lid_fd_) + 1;
      fd_set theset;
      FD_ZERO(&theset);
      FD_SET(x_fd_, &theset);
      FD_SET(lid_fd_, &theset);
      int rc = select(max_fd_plus_1, &theset, NULL, NULL, NULL);
      if (errno == EBADF) {
        close(lid_fd_);
        lid_fd_ = -1;
        continue;
      }
      if (rc < 0)
        PerrorAbort("select");
      if (FD_ISSET(x_fd_, &theset))
        ServiceXFd();
      else if (FD_ISSET(lid_fd_, &theset))
        ServiceLidFd();
    }
  }

  void ServiceLidFd() {
    if (!HandleInput(lid_fd_, notifier_.get())) {
      close(lid_fd_);
      lid_fd_ = -1;
    }
  }

  void ServiceXFd() {
    while (XPending(display_)) {
      XEvent ev;
      XNextEvent(display_, &ev);
      if (ev.xcookie.type == GenericEvent &&
          ev.xcookie.extension == xiopcode_ &&
          XGetEventData(display_, &ev.xcookie)) {
        if (ev.xcookie.evtype == XI_HierarchyChanged) {
          // Rescan for touchpad
          tp_id_ = -1;
        }
        XFreeEventData(display_, &ev.xcookie);
      }
    }
  }

  void RefreshDevices() {
    if (lid_fd_ < 0)
      lid_fd_ = GetLidFd();
    if (tp_id_ < 0) {
      tp_id_ = GetTouchpadXId(display_);
      if (tp_id_ >= 0)
        notifier_.reset(new XNotifier(display_, tp_id_));
    }
  }

  bool DevicesNeedRefresh() {
    return tp_id_ < 0 || lid_fd_ < 0;
  }

  // Lifted from http://codereview.chromium.org/6975057/patch/16001/17005
  int GetXInputOpCode() {
    static const char kExtensionName[] = "XInputExtension";
    int xi_opcode = -1;
    int event;
    int error;

    if (!XQueryExtension(display_, kExtensionName, &xi_opcode, &event,
                         &error)) {
      printf("X Input extension not available: error=%d\n", error);
      return -1;
    }
    return xi_opcode;
  }

 private:
  Display* display_;
  std::unique_ptr<XNotifier> notifier_;
  int tp_id_;
  int lid_fd_;
  int x_fd_;
  int xiopcode_;
};

};  // namespace lid_touchpad_helper

int main(int argc, char** argv) {
  using namespace lid_touchpad_helper;
  bool FLAGS_foreground = false;
  string FLAGS_display;

  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"display", required_argument, 0, 0},
      {"foreground", no_argument, 0, 0},
      {"help", no_argument, 0, 0},
      {0, 0, 0, 0}
    };

    if (getopt_long_only(argc, argv, "", long_options, &option_index) == 0) {
      switch (option_index) {
        case 0:
          FLAGS_display.assign(optarg);
          break;
        case 1:
          FLAGS_foreground = true;
          break;
        case 2:
          printf("Chromium OS Lid Touchpad Helper\n\n");
          printf("\t--display\tX display. Default: use env var DISPLAY\n");
          printf("\t--foreground\tDon't daemon()ize; run in foreground.\n");
          printf("\t--help\t\tDisplay this help message.\n");
          exit(0);
        default:
          break;
      }
    } else {
      break;
    }

  }

  if (!FLAGS_foreground && daemon(0, 0) < 0)
    PerrorAbort("daemon");

  Display* dpy = XOpenDisplay(
      FLAGS_display.empty() ? NULL : FLAGS_display.c_str());
  if (!dpy) {
    printf("XOpenDisplay failed\n");
    exit(1);
  }

  // Check for Xinput 2
  int opcode, event, err;
  if (!XQueryExtension(dpy, "XInputExtension", &opcode, &event, &err)) {
    fprintf(stderr,
            "Failed to get XInputExtension.\n");
    return -1;
  }

  int major = 2, minor = 0;
  if (XIQueryVersion(dpy, &major, &minor) == BadRequest) {
    fprintf(stderr,
            "Server does not have XInput2.\n");
    return -1;
  }

  MainLoop looper(dpy);
  looper.Run();
  return 0;
}
