// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "ui/app_list/test/app_list_test_model.h"
#include "ui/app_list/test/app_list_test_view_delegate.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views_content_client/views_content_client.h"

namespace {

// Number of dummy apps to populate in the app list.
const int kInitialItems = 20;

// Extends the test AppListViewDelegate to quit the run loop when the launcher
// window is closed, and to close the window if it is simply dismissed.
class DemoAppListViewDelegate : public app_list::test::AppListTestViewDelegate {
 public:
  DemoAppListViewDelegate() : view_(NULL) {}
  ~DemoAppListViewDelegate() override {}

  app_list::AppListView* InitView(gfx::NativeWindow window_context);

  // Overridden from AppListViewDelegate:
  void DismissAppList() override;
  void ViewClosing() override;

 private:
  app_list::AppListView* view_;  // Weak. Owns this.

  DISALLOW_COPY_AND_ASSIGN(DemoAppListViewDelegate);
};

app_list::AppListView* DemoAppListViewDelegate::InitView(
    gfx::NativeWindow window_context) {
  // On Ash, the app list is placed into an aura::Window container. For the demo
  // use the root window context as the parent. This only works on Aura since an
  // aura::Window is also a NativeView.
  gfx::NativeView container = window_context;

  view_ = new app_list::AppListView(this);
  app_list::AppListView::InitParams params;
  params.parent = container;
  view_->Initialize(params);

  // Populate some apps.
  GetTestModel()->PopulateApps(kInitialItems);
  app_list::AppListItemList* item_list = GetTestModel()->top_level_item_list();
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::Image test_image = rb.GetImageNamed(IDR_DEFAULT_FAVICON_32);
  for (size_t i = 0; i < item_list->item_count(); ++i) {
    app_list::AppListItem* item = item_list->item_at(i);
    // Alternate images with shadows and images without.
    item->SetIcon(*test_image.ToImageSkia());
  }
  return view_;
}

void DemoAppListViewDelegate::DismissAppList() {
  view_->GetWidget()->Close();
}

void DemoAppListViewDelegate::ViewClosing() {
  base::MessageLoopCurrent message_loop = base::MessageLoopCurrentForUI::Get();
  message_loop->task_runner()->DeleteSoon(FROM_HERE, this);
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

void ShowAppList(content::BrowserContext* browser_context,
                 gfx::NativeWindow window_context) {
  DemoAppListViewDelegate* delegate = new DemoAppListViewDelegate;
  app_list::AppListView* view = delegate->InitView(window_context);
  view->GetWidget()->Show();
  view->GetWidget()->Activate();
}

}  // namespace

int main(int argc, const char** argv) {
  ui::ViewsContentClient views_content_client(argc, argv);
  views_content_client.set_task(base::Bind(&ShowAppList));
  return views_content_client.RunMain();
}
