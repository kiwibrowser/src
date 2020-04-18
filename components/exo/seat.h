// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_SEAT_H_
#define COMPONENTS_EXO_SEAT_H_

#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "components/exo/data_source_observer.h"
#include "ui/aura/client/drag_drop_delegate.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/base/clipboard/clipboard_observer.h"
#include "ui/events/event_handler.h"

namespace ui {
enum class DomCode;
class KeyEvent;
}  // namespace ui

namespace exo {
class ScopedDataSource;
class SeatObserver;
class Surface;

// Seat object represent a group of input devices such as keyboard, pointer and
// touch devices and keeps track of input focus.
class Seat : public aura::client::FocusChangeObserver,
             public ui::EventHandler,
             public ui::ClipboardObserver,
             public DataSourceObserver {
 public:
  Seat();
  ~Seat() override;

  void AddObserver(SeatObserver* observer);
  void RemoveObserver(SeatObserver* observer);

  // Returns currently focused surface. This is vertual so that we can override
  // the behavior for testing.
  virtual Surface* GetFocusedSurface();

  // Returns currently pressed keys.
  const base::flat_set<ui::DomCode>& pressed_keys() const {
    return pressed_keys_;
  }

  // Returns current set of modifier flags.
  int modifier_flags() const { return modifier_flags_; }

  // Sets clipboard data from |source|.
  void SetSelection(DataSource* source);

  // Overridden from aura::client::FocusChangeObserver:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;

  // Overridden from ui::ClipboardObserver:
  void OnClipboardDataChanged() override;

  // Overridden from DataSourceObserver:
  void OnDataSourceDestroying(DataSource* source) override;

 private:
  // Called when data is read from FD passed from a client.
  // |data| is read data. |source| is source of the data, or nullptr if
  // DataSource has already been destroyed.
  void OnDataRead(const std::vector<uint8_t>& data);

  base::ObserverList<SeatObserver> observers_;
  base::flat_set<ui::DomCode> pressed_keys_;
  int modifier_flags_ = 0;

  // Data source being used as a clipboard content.
  std::unique_ptr<ScopedDataSource> selection_source_;

  // True while Seat is updating clipboard data to selection source.
  bool changing_clipboard_data_to_selection_source_;

  DISALLOW_COPY_AND_ASSIGN(Seat);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_SEAT_H_
