// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_MANAGER_FORWARDING_DISPLAY_DELEGATE_H_
#define UI_DISPLAY_MANAGER_FORWARDING_DISPLAY_DELEGATE_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "ui/display/manager/display_manager_export.h"
#include "ui/display/mojo/native_display_delegate.mojom.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/display/types/native_display_observer.h"

namespace display {

class DisplaySnapshot;

// NativeDisplayDelegate implementation that forwards calls to a real
// NativeDisplayDelegate in another process. Only forwards the methods
// implemented by Ozone DRM, other method won't do anything.
class DISPLAY_MANAGER_EXPORT ForwardingDisplayDelegate
    : public NativeDisplayDelegate,
      public mojom::NativeDisplayObserver {
 public:
  explicit ForwardingDisplayDelegate(mojom::NativeDisplayDelegatePtr delegate);
  ~ForwardingDisplayDelegate() override;

  // display::NativeDisplayDelegate:
  void Initialize() override;
  void TakeDisplayControl(DisplayControlCallback callback) override;
  void RelinquishDisplayControl(DisplayControlCallback callback) override;
  void GetDisplays(GetDisplaysCallback callback) override;
  void Configure(const DisplaySnapshot& output,
                 const DisplayMode* mode,
                 const gfx::Point& origin,
                 ConfigureCallback callback) override;
  void GetHDCPState(const DisplaySnapshot& output,
                    GetHDCPStateCallback callback) override;
  void SetHDCPState(const DisplaySnapshot& output,
                    HDCPState state,
                    SetHDCPStateCallback callback) override;
  bool SetColorCorrection(const DisplaySnapshot& output,
                          const std::vector<GammaRampRGBEntry>& degamma_lut,
                          const std::vector<GammaRampRGBEntry>& gamma_lut,
                          const std::vector<float>& correction_matrix) override;
  void AddObserver(display::NativeDisplayObserver* observer) override;
  void RemoveObserver(display::NativeDisplayObserver* observer) override;
  FakeDisplayController* GetFakeDisplayController() override;

  // display::mojom::NativeDisplayObserver:
  void OnConfigurationChanged() override;

 private:
  // Stores display snapshots and forwards pointers to |callback|.
  void StoreAndForwardDisplays(
      GetDisplaysCallback callback,
      std::vector<std::unique_ptr<DisplaySnapshot>> snapshots);

  // Forwards display snapshot pointers to |callback|.
  void ForwardDisplays(GetDisplaysCallback callback);

  // True if we should use |delegate_|. This will be false if synchronous
  // GetDisplays() and Configure() are required.
  bool use_delegate_ = false;

  mojom::NativeDisplayDelegatePtr delegate_;
  mojo::Binding<mojom::NativeDisplayObserver> binding_;

  // Display snapshots are owned here but accessed via raw pointers elsewhere.
  // Call OnDisplaySnapshotsInvalidated() on observers before invalidating them.
  std::vector<std::unique_ptr<DisplaySnapshot>> snapshots_;

  base::ObserverList<display::NativeDisplayObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(ForwardingDisplayDelegate);
};

}  // namespace display

#endif  // UI_DISPLAY_MANAGER_FORWARDING_DISPLAY_DELEGATE_H_
