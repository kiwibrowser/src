// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FIRST_RUN_FIRST_RUN_HELPER_H_
#define ASH_FIRST_RUN_FIRST_RUN_HELPER_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/first_run_helper.mojom.h"
#include "ash/session/session_observer.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

class DesktopCleaner;

// Interface used by first-run tutorial to manipulate and retrieve information
// about shell elements.
class ASH_EXPORT FirstRunHelper : public mojom::FirstRunHelper,
                                  public SessionObserver {
 public:
  FirstRunHelper();
  ~FirstRunHelper() override;

  void BindRequest(mojom::FirstRunHelperRequest request);

  // mojom::FirstRunHelper:
  void Start(mojom::FirstRunHelperClientPtr client) override;
  void Stop() override;
  void GetAppListButtonBounds(GetAppListButtonBoundsCallback cb) override;
  void OpenTrayBubble(OpenTrayBubbleCallback cb) override;
  void CloseTrayBubble() override;
  void GetHelpButtonBounds(GetHelpButtonBoundsCallback cb) override;

  // SessionObserver:
  void OnLockStateChanged(bool locked) override;
  void OnChromeTerminating() override;

  void FlushForTesting();

 private:
  // Notifies the client to cancel the tutorial.
  void Cancel();

  // Bindings for clients of the mojo interface.
  mojo::BindingSet<mojom::FirstRunHelper> bindings_;

  // Client interface (e.g. chrome).
  mojom::FirstRunHelperClientPtr client_;

  std::unique_ptr<DesktopCleaner> cleaner_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunHelper);
};

}  // namespace ash

#endif  // ASH_FIRST_RUN_FIRST_RUN_HELPER_H_
