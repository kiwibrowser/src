// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_TYPES_NATIVE_DISPLAY_DELEGATE_H_
#define UI_DISPLAY_TYPES_NATIVE_DISPLAY_DELEGATE_H_

#include <stdint.h>

#include <vector>

#include "base/callback.h"
#include "ui/display/types/display_constants.h"
#include "ui/display/types/display_types_export.h"
#include "ui/display/types/fake_display_controller.h"

namespace gfx {
class Point;
}

namespace display {
class DisplayMode;
class DisplaySnapshot;
class NativeDisplayObserver;

struct GammaRampRGBEntry;

using GetDisplaysCallback =
    base::OnceCallback<void(const std::vector<DisplaySnapshot*>&)>;
using ConfigureCallback = base::OnceCallback<void(bool)>;
using GetHDCPStateCallback = base::OnceCallback<void(bool, HDCPState)>;
using SetHDCPStateCallback = base::OnceCallback<void(bool)>;
using DisplayControlCallback = base::OnceCallback<void(bool)>;

// Interface for classes that perform display configuration actions on behalf
// of DisplayConfigurator.
// Implementations may perform calls asynchronously. In the case of functions
// taking callbacks, the callbacks may be called asynchronously when the results
// are available. The implementations must provide a strong guarantee that the
// callbacks are always called.
class DISPLAY_TYPES_EXPORT NativeDisplayDelegate {
 public:
  virtual ~NativeDisplayDelegate();

  virtual void Initialize() = 0;

  // Take control of the display from any other controlling process.
  virtual void TakeDisplayControl(DisplayControlCallback callback) = 0;

  // Let others control the display.
  virtual void RelinquishDisplayControl(DisplayControlCallback callback) = 0;

  // Queries for a list of fresh displays and returns them via |callback|.
  // Note the query operation may be expensive and take over 60 milliseconds.
  virtual void GetDisplays(GetDisplaysCallback callback) = 0;

  // Configures the display represented by |output| to use |mode| and positions
  // the display to |origin| in the framebuffer. |mode| can be NULL, which
  // represents disabling the display. The callback will return the status of
  // the operation.
  virtual void Configure(const DisplaySnapshot& output,
                         const DisplayMode* mode,
                         const gfx::Point& origin,
                         ConfigureCallback callback) = 0;

  // Gets HDCP state of output.
  virtual void GetHDCPState(const DisplaySnapshot& output,
                            GetHDCPStateCallback callback) = 0;

  // Sets HDCP state of output.
  virtual void SetHDCPState(const DisplaySnapshot& output,
                            HDCPState state,
                            SetHDCPStateCallback callback) = 0;

  // Set the gamma tables and corection matrix for the display.
  virtual bool SetColorCorrection(
      const DisplaySnapshot& output,
      const std::vector<GammaRampRGBEntry>& degamma_lut,
      const std::vector<GammaRampRGBEntry>& gamma_lut,
      const std::vector<float>& correction_matrix) = 0;

  virtual void AddObserver(NativeDisplayObserver* observer) = 0;

  virtual void RemoveObserver(NativeDisplayObserver* observer) = 0;

  // Returns a fake display controller that can modify the fake display state.
  // Will return null if not needed, most likely because the delegate is
  // intended for use on device and doesn't need to fake the display state.
  virtual FakeDisplayController* GetFakeDisplayController() = 0;
};

}  // namespace display

#endif  // UI_DISPLAY_TYPES_NATIVE_DISPLAY_DELEGATE_H_
