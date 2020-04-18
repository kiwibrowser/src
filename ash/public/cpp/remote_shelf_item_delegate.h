// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_REMOTE_SHELF_ITEM_DELEGATE_H_
#define ASH_PUBLIC_CPP_REMOTE_SHELF_ITEM_DELEGATE_H_

#include "ash/public/cpp/shelf_item_delegate.h"

namespace ash {

// A ShelfItemDelegate that forwards calls to a remote ShelfItemDelegatePtr.
// Used by Ash and Chrome ShelfModels for delegation across service boundaries.
class ASH_PUBLIC_EXPORT RemoteShelfItemDelegate : public ShelfItemDelegate {
 public:
  RemoteShelfItemDelegate(const ShelfID& shelf_id,
                          mojom::ShelfItemDelegatePtr delegate);
  ~RemoteShelfItemDelegate() override;

  // mojom::ShelfItemDelegate
  void ItemSelected(std::unique_ptr<ui::Event> event,
                    int64_t display_id,
                    ShelfLaunchSource source,
                    ItemSelectedCallback callback) override;
  void GetContextMenuItems(int64_t display_id,
                           GetContextMenuItemsCallback callback) override;
  void ExecuteCommand(bool from_context_menu,
                      int64_t command_id,
                      int32_t event_flags,
                      int64_t display_id) override;
  void Close() override;

 private:
  // The pointer to the remote delegate.
  mojom::ShelfItemDelegatePtr delegate_;

  DISALLOW_COPY_AND_ASSIGN(RemoteShelfItemDelegate);
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_REMOTE_SHELF_ITEM_DELEGATE_H_
