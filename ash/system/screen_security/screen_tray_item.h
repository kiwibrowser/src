// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SCREEN_SECURITY_SCREEN_TRAY_ITEM_H_
#define ASH_SYSTEM_SCREEN_SECURITY_SCREEN_TRAY_ITEM_H_

#include <string>

#include "ash/system/tray/system_tray_item.h"
#include "ash/system/tray/tray_item_view.h"
#include "base/macros.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view.h"

namespace views {
class ImageView;
}

namespace ash {
class ScreenTrayItem;
class SystemTray;

namespace tray {

class ScreenTrayView : public TrayItemView {
 public:
  explicit ScreenTrayView(ScreenTrayItem* screen_tray_item);
  ~ScreenTrayView() override;

  void Update();

 private:
  ScreenTrayItem* screen_tray_item_;

  DISALLOW_COPY_AND_ASSIGN(ScreenTrayView);
};

class ScreenStatusView : public views::View, public views::ButtonListener {
 public:
  ScreenStatusView(ScreenTrayItem* screen_tray_item,
                   const base::string16& label_text,
                   const base::string16& stop_button_text);
  ~ScreenStatusView() override;

  // Overridden from views::ButtonListener.
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  void CreateItems();
  void UpdateFromScreenTrayItem();

 protected:
  views::ImageView* icon() { return icon_; }
  views::Label* label() { return label_; }
  views::Button* stop_button() { return stop_button_; }

 private:
  // The controller for this view. May be null.
  ScreenTrayItem* screen_tray_item_;
  views::ImageView* icon_;
  views::Label* label_;
  views::Button* stop_button_;
  base::string16 label_text_;
  base::string16 stop_button_text_;

  DISALLOW_COPY_AND_ASSIGN(ScreenStatusView);
};

}  // namespace tray

// The base tray item for screen capture and screen sharing. The
// Start method brings up a notification and a tray item, and the user
// can stop the screen capture/sharing by pressing the stop button.
class ASH_EXPORT ScreenTrayItem : public SystemTrayItem {
 public:
  ScreenTrayItem(SystemTray* system_tray, UmaType uma_type);
  ~ScreenTrayItem() override;

  tray::ScreenTrayView* tray_view() { return tray_view_; }

  tray::ScreenStatusView* default_view() { return default_view_; }
  void set_default_view(tray::ScreenStatusView* default_view) {
    default_view_ = default_view;
  }

  bool is_started() const { return is_started_; }

  void SetIsStarted(bool is_started);

  void Update();
  void Start(base::OnceClosure stop_callback);
  void Stop();

  // Called after Stop() is invoked from the default view.
  virtual void RecordStoppedFromDefaultViewMetric() = 0;

  // Overridden from SystemTrayItem.
  views::View* CreateTrayView(LoginStatus status) override;
  views::View* CreateDefaultView(LoginStatus status) override = 0;
  void OnTrayViewDestroyed() override;
  void OnDefaultViewDestroyed() override;

 private:
  base::OnceClosure stop_callback_;
  tray::ScreenTrayView* tray_view_;
  tray::ScreenStatusView* default_view_;
  bool is_started_;

  DISALLOW_COPY_AND_ASSIGN(ScreenTrayItem);
};

}  // namespace ash

#endif  // ASH_SYSTEM_SCREEN_SECURITY_SCREEN_TRAY_ITEM_H_
