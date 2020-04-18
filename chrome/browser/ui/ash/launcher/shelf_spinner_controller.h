// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_LAUNCHER_SHELF_SPINNER_CONTROLLER_H_
#define CHROME_BROWSER_UI_ASH_LAUNCHER_SHELF_SPINNER_CONTROLLER_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

class ShelfSpinnerItemController;
class ChromeLauncherController;
class Profile;

namespace gfx {
class ImageSkia;
}  // namespace gfx

// ShelfSpinnerController displays visual feedback that the application the user
// has just activated will not be immediately available, as it is for example
// waiting for ARC or Crostini to be ready.
class ShelfSpinnerController {
 public:
  explicit ShelfSpinnerController(ChromeLauncherController* owner);
  ~ShelfSpinnerController();

  bool HasApp(const std::string& app_id) const;

  base::TimeDelta GetActiveTime(const std::string& app_id) const;

  // Adds a spinner to the shelf unless the app is already running.
  void AddSpinnerToShelf(
      const std::string& app_id,
      std::unique_ptr<ShelfSpinnerItemController> controller);

  // Applies spinning effect if requested app is handled by spinner controller.
  void MaybeApplySpinningEffect(const std::string& app_id,
                                gfx::ImageSkia* image);

  // Removes entry from the list of tracking items.
  void Remove(const std::string& app_id);

  // Closes Shelf item if it has ShelfSpinnerItemController controller
  // and removes entry from the list of tracking items.
  void Close(const std::string& app_id);

  Profile* OwnerProfile();

 private:
  // Defines mapping of a shelf app id to a corresponded controller. Shelf app
  // id is optional mapping (for example, Play Store to ARC Host Support).
  using AppControllerMap = std::map<std::string, ShelfSpinnerItemController*>;

  void UpdateApps();
  void UpdateShelfItemIcon(const std::string& app_id);
  void RegisterNextUpdate();

  // Unowned pointers.
  ChromeLauncherController* owner_;

  AppControllerMap app_controller_map_;

  // Always keep this the last member of this class.
  base::WeakPtrFactory<ShelfSpinnerController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShelfSpinnerController);
};

#endif  // CHROME_BROWSER_UI_ASH_LAUNCHER_SHELF_SPINNER_CONTROLLER_H_
