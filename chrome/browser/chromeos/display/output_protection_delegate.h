// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_DELEGATE_H_
#define CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_DELEGATE_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/aura/window.h"
#include "ui/aura/window_observer.h"

namespace chromeos {

// A class to query output protection status and/or enable output protection.
// All methods except constructor should be invoked in UI thread.
class OutputProtectionDelegate : public aura::WindowObserver {
 public:
  typedef base::Callback<void(bool /* success */,
                              uint32_t /* link_mask */,
                              uint32_t /* protection_mask*/)>
      QueryStatusCallback;
  typedef base::Callback<void(bool /* success */)> SetProtectionCallback;

  OutputProtectionDelegate(int render_process_id, int render_frame_id);
  ~OutputProtectionDelegate() override;

  // aura::WindowObserver overrides.
  void OnWindowHierarchyChanged(
      const aura::WindowObserver::HierarchyChangeParams& params) override;
  void OnWindowDestroying(aura::Window* window) override;

  void QueryStatus(const QueryStatusCallback& callback);
  void SetProtection(uint32_t desired_method_mask,
                     const SetProtectionCallback& callback);

  // Display content protection controller interface.
  class Controller {
   public:
    Controller();
    virtual ~Controller();
    virtual void QueryStatus(int64_t display_id,
                             const QueryStatusCallback& callback) = 0;
    virtual void SetProtection(int64_t display_id,
                               uint32_t desired_method_mask,
                               const SetProtectionCallback& callback) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Controller);
  };

 private:
  bool InitializeControllerIfNecessary();

  // Used to lookup the WebContents associated with the render frame.
  int render_process_id_;
  int render_frame_id_;

  // Native window being observed.
  aura::Window* window_;

  // The display id which the renderer currently uses.
  int64_t display_id_;

  // The last desired method mask. Will enable this mask on new display if
  // renderer changes display.
  uint32_t desired_method_mask_;

  // The display content protection controller.
  std::unique_ptr<Controller> controller_;

  base::WeakPtrFactory<OutputProtectionDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OutputProtectionDelegate);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DISPLAY_OUTPUT_PROTECTION_DELEGATE_H_
