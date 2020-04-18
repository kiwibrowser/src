// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_TEST_API_H_
#define ASH_SHELF_SHELF_TEST_API_H_

#include "ash/public/interfaces/shelf_test_api.mojom.h"
#include "base/macros.h"

namespace ash {

class Shelf;

// Allows tests to access private state of the shelf.
class ShelfTestApi : public mojom::ShelfTestApi {
 public:
  explicit ShelfTestApi(Shelf* shelf);
  ~ShelfTestApi() override;

  // Creates and binds an instance from a remote request (e.g. from chrome).
  static void BindRequest(mojom::ShelfTestApiRequest request);

  // mojom::ShelfTestApi:
  void IsVisible(IsVisibleCallback cb) override;
  void UpdateVisibility(UpdateVisibilityCallback cb) override;
  void HasOverlappingWindow(HasOverlappingWindowCallback cb) override;
  void IsAlignmentBottomLocked(IsAlignmentBottomLockedCallback cb) override;

 private:
  Shelf* shelf_;

  DISALLOW_COPY_AND_ASSIGN(ShelfTestApi);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_TEST_API_H_
