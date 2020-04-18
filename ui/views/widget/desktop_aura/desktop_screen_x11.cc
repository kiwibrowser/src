// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/desktop_screen_x11.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/protected_memory_cfi.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/layout.h"
#include "ui/display/display.h"
#include "ui/display/display_finder.h"
#include "ui/display/screen.h"
#include "ui/display/util/display_util.h"
#include "ui/display/util/x11/edid_parser_x11.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/events/platform/x11/x11_event_source.h"
#include "ui/gfx/font_render_params.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/icc_profile.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/switches.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/views/linux_ui/linux_ui.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_x11.h"
#include "ui/views/widget/desktop_aura/x11_topmost_window_finder.h"

#include <dlfcn.h>

namespace {

// static
gfx::ICCProfile GetICCProfileForMonitor(int monitor) {
  gfx::ICCProfile icc_profile;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kHeadless))
    return icc_profile;
  std::string atom_name;
  if (monitor == 0) {
    atom_name = "_ICC_PROFILE";
  } else {
    atom_name = base::StringPrintf("_ICC_PROFILE_%d", monitor);
  }
  Atom property = gfx::GetAtom(atom_name.c_str());
  if (property != x11::None) {
    Atom prop_type = x11::None;
    int prop_format = 0;
    unsigned long nitems = 0;
    unsigned long nbytes = 0;
    char* property_data = NULL;
    if (XGetWindowProperty(
            gfx::GetXDisplay(), DefaultRootWindow(gfx::GetXDisplay()), property,
            0, 0x1FFFFFFF /* MAXINT32 / 4 */, x11::False, AnyPropertyType,
            &prop_type, &prop_format, &nitems, &nbytes,
            reinterpret_cast<unsigned char**>(&property_data)) ==
        x11::Success) {
      icc_profile = gfx::ICCProfile::FromData(property_data, nitems);
      XFree(property_data);
    }
  }
  return icc_profile;
}

double GetDeviceScaleFactor() {
  float device_scale_factor = 1.0f;
  if (views::LinuxUI::instance()) {
    device_scale_factor =
      views::LinuxUI::instance()->GetDeviceScaleFactor();
  } else if (display::Display::HasForceDeviceScaleFactor()) {
    device_scale_factor = display::Display::GetForcedDeviceScaleFactor();
  }
  return device_scale_factor;
}

gfx::Point PixelToDIPPoint(const gfx::Point& pixel_point) {
  return gfx::ScaleToFlooredPoint(pixel_point, 1.0f / GetDeviceScaleFactor());
}

gfx::Point DIPToPixelPoint(const gfx::Point& dip_point) {
  return gfx::ScaleToFlooredPoint(dip_point, GetDeviceScaleFactor());
}

std::vector<display::Display> GetFallbackDisplayList() {
  ::XDisplay* display = gfx::GetXDisplay();
  ::Screen* screen = DefaultScreenOfDisplay(display);
  int width = WidthOfScreen(screen);
  int height = HeightOfScreen(screen);
  gfx::Size physical_size(WidthMMOfScreen(screen), HeightMMOfScreen(screen));

  gfx::Rect bounds_in_pixels(0, 0, width, height);
  display::Display gfx_display(0, bounds_in_pixels);
  if (!display::Display::HasForceDeviceScaleFactor() &&
      !display::IsDisplaySizeBlackListed(physical_size)) {
    const float device_scale_factor = GetDeviceScaleFactor();
    DCHECK_LE(1.0f, device_scale_factor);
    gfx_display.SetScaleAndBounds(device_scale_factor, bounds_in_pixels);
  }

  return std::vector<display::Display>(1, gfx_display);
}

}  // namespace

namespace views {

////////////////////////////////////////////////////////////////////////////////
// DesktopScreenX11, public:

DesktopScreenX11::DesktopScreenX11()
    : xdisplay_(gfx::GetXDisplay()),
      x_root_window_(DefaultRootWindow(xdisplay_)),
      xrandr_version_(0),
      xrandr_event_base_(0),
      primary_display_index_(0),
      weak_factory_(this) {
  if (views::LinuxUI::instance())
    views::LinuxUI::instance()->AddDeviceScaleFactorObserver(this);
  // We only support 1.3+. There were library changes before this and we should
  // use the new interface instead of the 1.2 one.
  int randr_version_major = 0;
  int randr_version_minor = 0;
  if (XRRQueryVersion(xdisplay_, &randr_version_major, &randr_version_minor)) {
    xrandr_version_ = randr_version_major * 100 + randr_version_minor;
  }
  // Need at least xrandr version 1.3.
  if (xrandr_version_ >= 103) {
    int error_base_ignored = 0;
    XRRQueryExtension(xdisplay_, &xrandr_event_base_, &error_base_ignored);

    if (ui::PlatformEventSource::GetInstance())
      ui::PlatformEventSource::GetInstance()->AddPlatformEventDispatcher(this);
    XRRSelectInput(xdisplay_,
                   x_root_window_,
                   RRScreenChangeNotifyMask |
                   RROutputChangeNotifyMask |
                   RRCrtcChangeNotifyMask);

    SetDisplaysInternal(BuildDisplaysFromXRandRInfo());
  } else {
    SetDisplaysInternal(GetFallbackDisplayList());
  }
}

DesktopScreenX11::~DesktopScreenX11() {
  if (views::LinuxUI::instance())
    views::LinuxUI::instance()->AddDeviceScaleFactorObserver(this);
  if (xrandr_version_ >= 103 && ui::PlatformEventSource::GetInstance())
    ui::PlatformEventSource::GetInstance()->RemovePlatformEventDispatcher(this);
}

////////////////////////////////////////////////////////////////////////////////
// DesktopScreenX11, display::Screen implementation:

gfx::Point DesktopScreenX11::GetCursorScreenPoint() {
  TRACE_EVENT0("views", "DesktopScreenX11::GetCursorScreenPoint()");

  if (ui::X11EventSource::HasInstance()) {
    auto point = ui::X11EventSource::GetInstance()
                     ->GetRootCursorLocationFromCurrentEvent();
    if (point)
      return PixelToDIPPoint(point.value());
  }

  ::Window root, child;
  int root_x, root_y, win_x, win_y;
  unsigned int mask;
  XQueryPointer(xdisplay_, x_root_window_, &root, &child, &root_x, &root_y,
                &win_x, &win_y, &mask);

  return PixelToDIPPoint(gfx::Point(root_x, root_y));
}

bool DesktopScreenX11::IsWindowUnderCursor(gfx::NativeWindow window) {
  return GetWindowAtScreenPoint(GetCursorScreenPoint()) == window;
}

gfx::NativeWindow DesktopScreenX11::GetWindowAtScreenPoint(
    const gfx::Point& point) {
  X11TopmostWindowFinder finder;
  return finder.FindLocalProcessWindowAt(
      DIPToPixelPoint(point), std::set<aura::Window*>());
}

int DesktopScreenX11::GetNumDisplays() const {
  return displays_.size();
}

const std::vector<display::Display>& DesktopScreenX11::GetAllDisplays() const {
  return displays_;
}

display::Display DesktopScreenX11::GetDisplayNearestWindow(
    gfx::NativeView window) const {
  if (!window)
    return GetPrimaryDisplay();

  // Getting screen bounds here safely is hard.
  //
  // You'd think we'd be able to just call window->GetBoundsInScreen(), but we
  // can't because |window| (and the associated WindowEventDispatcher*) can be
  // partially initialized at this point; WindowEventDispatcher initializations
  // call through into GetDisplayNearestWindow(). But the X11 resources are
  // created before we create the aura::WindowEventDispatcher. So we ask what
  // the DRWHX11 believes the window bounds are instead of going through the
  // aura::Window's screen bounds.
  aura::WindowTreeHost* host = window->GetHost();
  if (host) {
    DesktopWindowTreeHostX11* rwh = DesktopWindowTreeHostX11::GetHostForXID(
        host->GetAcceleratedWidget());
    if (rwh)
      return GetDisplayMatching(rwh->GetX11RootWindowBounds());
  }

  return GetPrimaryDisplay();
}

display::Display DesktopScreenX11::GetDisplayNearestPoint(
    const gfx::Point& point) const {
  if (displays_.size() <= 1)
    return GetPrimaryDisplay();
  for (std::vector<display::Display>::const_iterator it = displays_.begin();
       it != displays_.end(); ++it) {
    if (it->bounds().Contains(point))
      return *it;
  }
  return *FindDisplayNearestPoint(displays_, point);
}

display::Display DesktopScreenX11::GetDisplayMatching(
    const gfx::Rect& match_rect) const {
  int max_area = 0;
  const display::Display* matching = NULL;
  for (std::vector<display::Display>::const_iterator it = displays_.begin();
       it != displays_.end(); ++it) {
    gfx::Rect intersect = gfx::IntersectRects(it->bounds(), match_rect);
    int area = intersect.width() * intersect.height();
    if (area > max_area) {
      max_area = area;
      matching = &*it;
    }
  }
  // Fallback to the primary display if there is no matching display.
  return matching ? *matching : GetPrimaryDisplay();
}

display::Display DesktopScreenX11::GetPrimaryDisplay() const {
  DCHECK(!displays_.empty());
  return displays_[primary_display_index_];
}

void DesktopScreenX11::AddObserver(display::DisplayObserver* observer) {
  change_notifier_.AddObserver(observer);
}

void DesktopScreenX11::RemoveObserver(display::DisplayObserver* observer) {
  change_notifier_.RemoveObserver(observer);
}

bool DesktopScreenX11::CanDispatchEvent(const ui::PlatformEvent& event) {
  return event->type - xrandr_event_base_ == RRScreenChangeNotify ||
         event->type - xrandr_event_base_ == RRNotify ||
         (event->type == PropertyNotify &&
          event->xproperty.window == x_root_window_ &&
          event->xproperty.atom == gfx::GetAtom("_NET_WORKAREA"));
}

uint32_t DesktopScreenX11::DispatchEvent(const ui::PlatformEvent& event) {
  if (event->type - xrandr_event_base_ == RRScreenChangeNotify) {
    // Pass the event through to xlib.
    XRRUpdateConfiguration(event);
  } else if (event->type - xrandr_event_base_ == RRNotify ||
             (event->type == PropertyNotify &&
              event->xproperty.atom == gfx::GetAtom("_NET_WORKAREA"))) {
    RestartDelayedConfigurationTask();
  } else {
    NOTREACHED();
  }

  return ui::POST_DISPATCH_NONE;
}

void DesktopScreenX11::OnDeviceScaleFactorChanged() {
  RestartDelayedConfigurationTask();
}

// static
void DesktopScreenX11::UpdateDeviceScaleFactorForTest() {
  DesktopScreenX11* screen =
      static_cast<DesktopScreenX11*>(display::Screen::GetScreen());
  screen->UpdateDisplays();
}

////////////////////////////////////////////////////////////////////////////////
// DesktopScreenX11, private:

DesktopScreenX11::DesktopScreenX11(
    const std::vector<display::Display>& test_displays)
    : xdisplay_(gfx::GetXDisplay()),
      x_root_window_(DefaultRootWindow(xdisplay_)),
      xrandr_version_(0),
      xrandr_event_base_(0),
      displays_(test_displays),
      primary_display_index_(0),
      weak_factory_(this) {
  if (views::LinuxUI::instance())
    views::LinuxUI::instance()->AddDeviceScaleFactorObserver(this);
}

typedef XRRMonitorInfo* (*XRRGetMonitors)(::Display*, Window, bool, int*);
typedef void (*XRRFreeMonitors)(XRRMonitorInfo*);

PROTECTED_MEMORY_SECTION base::ProtectedMemory<XRRGetMonitors>
    g_XRRGetMonitors_ptr;
PROTECTED_MEMORY_SECTION base::ProtectedMemory<XRRFreeMonitors>
    g_XRRFreeMonitors_ptr;

std::vector<display::Display> DesktopScreenX11::BuildDisplaysFromXRandRInfo() {
  DCHECK(xrandr_version_ >= 103);
  std::vector<display::Display> displays;
  gfx::XScopedPtr<
      XRRScreenResources,
      gfx::XObjectDeleter<XRRScreenResources, void, XRRFreeScreenResources>>
      resources(XRRGetScreenResourcesCurrent(xdisplay_, x_root_window_));
  if (!resources) {
    LOG(ERROR) << "XRandR returned no displays. Falling back to Root Window.";
    return GetFallbackDisplayList();
  }

  std::map<RROutput, int> output_to_monitor;
  if (xrandr_version_ >= 105) {
    void* xrandr_lib = dlopen(NULL, RTLD_NOW);
    if (xrandr_lib) {
      static base::ProtectedMemory<XRRGetMonitors>::Initializer get_init(
          &g_XRRGetMonitors_ptr, reinterpret_cast<XRRGetMonitors>(
                                     dlsym(xrandr_lib, "XRRGetMonitors")));
      static base::ProtectedMemory<XRRFreeMonitors>::Initializer free_init(
          &g_XRRFreeMonitors_ptr, reinterpret_cast<XRRFreeMonitors>(
                                      dlsym(xrandr_lib, "XRRFreeMonitors")));
      if (*g_XRRGetMonitors_ptr && *g_XRRFreeMonitors_ptr) {
        int nmonitors = 0;
        XRRMonitorInfo* monitors = base::UnsanitizedCfiCall(
            g_XRRGetMonitors_ptr)(xdisplay_, x_root_window_, false, &nmonitors);
        for (int monitor = 0; monitor < nmonitors; monitor++) {
          for (int j = 0; j < monitors[monitor].noutput; j++) {
            output_to_monitor[monitors[monitor].outputs[j]] = monitor;
          }
        }
        base::UnsanitizedCfiCall(g_XRRFreeMonitors_ptr)(monitors);
      }
    }
  }

  primary_display_index_ = 0;
  RROutput primary_display_id = XRRGetOutputPrimary(xdisplay_, x_root_window_);

  int explicit_primary_display_index = -1;
  int monitor_order_primary_display_index = -1;

  bool has_work_area = false;
  gfx::Rect work_area_in_pixels;
  std::vector<int> value;
  if (ui::GetIntArrayProperty(x_root_window_, "_NET_WORKAREA", &value) &&
      value.size() >= 4) {
    work_area_in_pixels = gfx::Rect(value[0], value[1], value[2], value[3]);
    has_work_area = true;
  }

  // As per-display scale factor is not supported right now,
  // the X11 root window's scale factor is always used.
  const float device_scale_factor = GetDeviceScaleFactor();
  for (int i = 0; i < resources->noutput; ++i) {
    RROutput output_id = resources->outputs[i];
    gfx::XScopedPtr<XRROutputInfo,
                    gfx::XObjectDeleter<XRROutputInfo, void, XRRFreeOutputInfo>>
        output_info(XRRGetOutputInfo(xdisplay_, resources.get(), output_id));

    bool is_connected = (output_info->connection == RR_Connected);
    if (!is_connected)
      continue;

    bool is_primary_display = output_id == primary_display_id;

    if (output_info->crtc) {
      gfx::XScopedPtr<XRRCrtcInfo,
                      gfx::XObjectDeleter<XRRCrtcInfo, void, XRRFreeCrtcInfo>>
          crtc(XRRGetCrtcInfo(xdisplay_, resources.get(), output_info->crtc));

      int64_t display_id = -1;
      if (!display::EDIDParserX11(output_id).GetDisplayId(
              static_cast<uint8_t>(i), &display_id)) {
        // It isn't ideal, but if we can't parse the EDID data, fallback on the
        // display number.
        display_id = i;
      }

      gfx::Rect crtc_bounds(crtc->x, crtc->y, crtc->width, crtc->height);
      display::Display display(display_id, crtc_bounds);

      if (!display::Display::HasForceDeviceScaleFactor()) {
        display.SetScaleAndBounds(device_scale_factor, crtc_bounds);
      }

      if (has_work_area) {
        gfx::Rect intersection_in_pixels = crtc_bounds;
        if (is_primary_display) {
          intersection_in_pixels.Intersect(work_area_in_pixels);
        }
        // SetScaleAndBounds() above does the conversion from pixels to DIP for
        // us, but set_work_area does not, so we need to do it here.
        display.set_work_area(gfx::Rect(
            gfx::ScaleToFlooredPoint(intersection_in_pixels.origin(),
                                     1.0f / display.device_scale_factor()),
            gfx::ScaleToFlooredSize(intersection_in_pixels.size(),
                                    1.0f / display.device_scale_factor())));
      }

      switch (crtc->rotation) {
        case RR_Rotate_0:
          display.set_rotation(display::Display::ROTATE_0);
          break;
        case RR_Rotate_90:
          display.set_rotation(display::Display::ROTATE_90);
          break;
        case RR_Rotate_180:
          display.set_rotation(display::Display::ROTATE_180);
          break;
        case RR_Rotate_270:
          display.set_rotation(display::Display::ROTATE_270);
          break;
      }

      if (is_primary_display)
        explicit_primary_display_index = displays.size();

      auto monitor_iter = output_to_monitor.find(output_id);
      if (monitor_iter != output_to_monitor.end() && monitor_iter->second == 0)
        monitor_order_primary_display_index = displays.size();

      if (!display::Display::HasForceColorProfile()) {
        gfx::ICCProfile icc_profile = GetICCProfileForMonitor(
            monitor_iter == output_to_monitor.end() ? 0 : monitor_iter->second);
        icc_profile.HistogramDisplay(display.id());
        display.set_color_space(icc_profile.GetColorSpace());
      }

      displays.push_back(display);
    }
  }

  if (explicit_primary_display_index != -1) {
    primary_display_index_ = explicit_primary_display_index;
  } else if (monitor_order_primary_display_index != -1) {
    primary_display_index_ = monitor_order_primary_display_index;
  }

  if (displays.empty())
    return GetFallbackDisplayList();

  return displays;
}

void DesktopScreenX11::RestartDelayedConfigurationTask() {
  delayed_configuration_task_.Reset(base::Bind(
      &DesktopScreenX11::UpdateDisplays, weak_factory_.GetWeakPtr()));
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, delayed_configuration_task_.callback());
}

void DesktopScreenX11::UpdateDisplays() {
  std::vector<display::Display> old_displays = displays_;
  if (xrandr_version_ > 103)
    SetDisplaysInternal(BuildDisplaysFromXRandRInfo());
  else
    SetDisplaysInternal(GetFallbackDisplayList());
  change_notifier_.NotifyDisplaysChanged(old_displays, displays_);
}

void DesktopScreenX11::SetDisplaysInternal(
    const std::vector<display::Display>& displays) {
  displays_ = displays;
  gfx::SetFontRenderParamsDeviceScaleFactor(
      GetPrimaryDisplay().device_scale_factor());
}

////////////////////////////////////////////////////////////////////////////////

display::Screen* CreateDesktopScreen() {
  return new DesktopScreenX11;
}

}  // namespace views
