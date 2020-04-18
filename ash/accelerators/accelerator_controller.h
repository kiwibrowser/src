// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_ACCELERATOR_CONTROLLER_H_
#define ASH_ACCELERATORS_ACCELERATOR_CONTROLLER_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "ash/accelerators/accelerator_table.h"
#include "ash/accelerators/exit_warning_handler.h"
#include "ash/ash_export.h"
#include "ash/public/interfaces/accelerator_controller.mojom.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/accelerator_history.h"

namespace ui {
class AcceleratorManager;
class AcceleratorManagerDelegate;
}

namespace ash {

struct AcceleratorData;
class ExitWarningHandler;

// Identifier for the high contrast toggle accelerator notification.
ASH_EXPORT extern const char kHighContrastToggleAccelNotificationId[];

// AcceleratorController provides functions for registering or unregistering
// global keyboard accelerators, which are handled earlier than any windows. It
// also implements several handlers as an accelerator target.
class ASH_EXPORT AcceleratorController : public ui::AcceleratorTarget,
                                         public mojom::AcceleratorController {
 public:
  explicit AcceleratorController(
      ui::AcceleratorManagerDelegate* manager_delegate);
  ~AcceleratorController() override;

  // A list of possible ways in which an accelerator should be restricted before
  // processing. Any target registered with this controller should respect
  // restrictions by calling |GetCurrentAcceleratorRestriction| during
  // processing.
  enum AcceleratorProcessingRestriction {
    // Process the accelerator normally.
    RESTRICTION_NONE,

    // Don't process the accelerator.
    RESTRICTION_PREVENT_PROCESSING,

    // Don't process the accelerator and prevent propagation to other targets.
    RESTRICTION_PREVENT_PROCESSING_AND_PROPAGATION
  };

  // Registers global keyboard accelerators for the specified target. If
  // multiple targets are registered for any given accelerator, a target
  // registered later has higher priority.
  void Register(const std::vector<ui::Accelerator>& accelerators,
                ui::AcceleratorTarget* target);

  // Unregisters the specified keyboard accelerator for the specified target.
  void Unregister(const ui::Accelerator& accelerator,
                  ui::AcceleratorTarget* target);

  // Unregisters all keyboard accelerators for the specified target.
  void UnregisterAll(ui::AcceleratorTarget* target);

  // Returns true if there is an action for |accelerator| and it is enabled.
  bool IsActionForAcceleratorEnabled(const ui::Accelerator& accelerator) const;

  // Activates the target associated with the specified accelerator.
  // First, AcceleratorPressed handler of the most recently registered target
  // is called, and if that handler processes the event (i.e. returns true),
  // this method immediately returns. If not, we do the same thing on the next
  // target, and so on.
  // Returns true if an accelerator was activated.
  bool Process(const ui::Accelerator& accelerator);

  // Returns true if the |accelerator| is registered.
  bool IsRegistered(const ui::Accelerator& accelerator) const;

  // Returns true if the |accelerator| is preferred. A preferred accelerator
  // is handled before being passed to an window/web contents, unless
  // the window is in fullscreen state.
  bool IsPreferred(const ui::Accelerator& accelerator) const;

  // Returns true if the |accelerator| is reserved. A reserved accelerator
  // is always handled and will never be passed to an window/web contents.
  bool IsReserved(const ui::Accelerator& accelerator) const;

  // Returns true if the |accelerator| is deprecated. Deprecated accelerators
  // can be consumed by web contents if needed.
  bool IsDeprecated(const ui::Accelerator& accelerator) const;

  // Performs the specified action if it is enabled. Returns whether the action
  // was performed successfully.
  bool PerformActionIfEnabled(AcceleratorAction action);

  // Returns the restriction for the current context.
  AcceleratorProcessingRestriction GetCurrentAcceleratorRestriction();

  // Provides access to the ExitWarningHandler for testing.
  ExitWarningHandler* GetExitWarningHandlerForTest() {
    return &exit_warning_handler_;
  }

  // Returns true if the menu should close in order to perform the accelerator.
  bool ShouldCloseMenuAndRepostAccelerator(
      const ui::Accelerator& accelerator) const;

  ui::AcceleratorHistory* accelerator_history() {
    return accelerator_history_.get();
  }

  // Overridden from ui::AcceleratorTarget:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool CanHandleAccelerators() const override;

  // Binds the mojom::AcceleratorController interface to this object.
  void BindRequest(mojom::AcceleratorControllerRequest request);

  // mojom::AcceleratorController:
  void SetVolumeController(mojom::VolumeControllerPtr controller) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(AcceleratorControllerTest, GlobalAccelerators);
  FRIEND_TEST_ALL_PREFIXES(AcceleratorControllerTest,
                           DontRepeatToggleFullscreen);
  FRIEND_TEST_ALL_PREFIXES(DeprecatedAcceleratorTester,
                           TestDeprecatedAcceleratorsBehavior);

  // Initializes the accelerators this class handles as a target.
  void Init();

  // Registers the specified accelerators.
  void RegisterAccelerators(const AcceleratorData accelerators[],
                            size_t accelerators_length);

  // Registers the deprecated accelerators and their replacing new ones.
  void RegisterDeprecatedAccelerators();

  // Returns whether |action| can be performed. The |accelerator| may provide
  // additional data the action needs.
  bool CanPerformAction(AcceleratorAction action,
                        const ui::Accelerator& accelerator) const;

  // Performs the specified action. The |accelerator| may provide additional
  // data the action needs.
  void PerformAction(AcceleratorAction action,
                     const ui::Accelerator& accelerator);

  // Returns whether performing |action| should consume the key event.
  bool ShouldActionConsumeKeyEvent(AcceleratorAction action);

  // Get the accelerator restriction for the given action. Supply an |action|
  // of -1 to get restrictions that apply for the current context.
  AcceleratorProcessingRestriction GetAcceleratorProcessingRestriction(
      int action) const;

  // If |accelerator| is a deprecated accelerator, it performs the appropriate
  // deprecated accelerator pre-handling.
  // Returns PROCEED if the accelerator's action should be performed (i.e. if
  // |accelerator| is not a deprecated accelerator, or it's an enabled
  // deprecated accelerator), and STOP otherwise (if the accelerator is a
  // disabled deprecated accelerator).
  enum class AcceleratorProcessingStatus { PROCEED, STOP };
  AcceleratorProcessingStatus MaybeDeprecatedAcceleratorPressed(
      AcceleratorAction action,
      const ui::Accelerator& accelerator) const;

  std::unique_ptr<ui::AcceleratorManager> accelerator_manager_;

  // A tracker for the current and previous accelerators.
  std::unique_ptr<ui::AcceleratorHistory> accelerator_history_;

  // Handles the exit accelerator which requires a double press to exit and
  // shows a popup with an explanation.
  ExitWarningHandler exit_warning_handler_;

  // A map from accelerators to the AcceleratorAction values, which are used in
  // the implementation.
  std::map<ui::Accelerator, AcceleratorAction> accelerators_;

  std::map<AcceleratorAction, const DeprecatedAcceleratorData*>
      actions_with_deprecations_;
  std::set<ui::Accelerator> deprecated_accelerators_;

  // Bindings for the mojom::AcceleratorController interface.
  mojo::BindingSet<mojom::AcceleratorController> bindings_;

  // Volume controller interface in chrome browser. May be null in tests. Exists
  // because chrome owns the CrasAudioHandler dbus communication.
  mojom::VolumeControllerPtr volume_controller_;

  // Actions allowed when the user is not signed in.
  std::set<int> actions_allowed_at_login_screen_;
  // Actions allowed when the screen is locked.
  std::set<int> actions_allowed_at_lock_screen_;
  // Actions allowed when the power menu is opened.
  std::set<int> actions_allowed_at_power_menu_;
  // Actions allowed when a modal window is up.
  std::set<int> actions_allowed_at_modal_window_;
  // Preferred actions. See accelerator_table.h for details.
  std::set<int> preferred_actions_;
  // Reserved actions. See accelerator_table.h for details.
  std::set<int> reserved_actions_;
  // Actions which will be repeated while holding the accelerator key.
  std::set<int> repeatable_actions_;
  // Actions allowed in app mode.
  std::set<int> actions_allowed_in_app_mode_;
  // Actions allowed in pinned mode.
  std::set<int> actions_allowed_in_pinned_mode_;
  // Actions disallowed if there are no windows.
  std::set<int> actions_needing_window_;
  // Actions that can be performed without closing the menu (if one is present).
  std::set<int> actions_keeping_menu_open_;

  DISALLOW_COPY_AND_ASSIGN(AcceleratorController);
};

}  // namespace ash

#endif  // ASH_ACCELERATORS_ACCELERATOR_CONTROLLER_H_
