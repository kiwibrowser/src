// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/examples/example_base.h"

#include <stdarg.h>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "ui/views/view.h"

namespace views {
namespace examples {

// Logs the specified string to the status area of the examples window.
// This function can only be called if there is a visible examples window.
void LogStatus(const std::string& status);

namespace {

// TODO(oshima): Check if this special container is still necessary.
class ContainerView : public View {
 public:
  explicit ContainerView(ExampleBase* base)
      : example_view_created_(false),
        example_base_(base) {
  }

 private:
  // View:
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override {
    View::ViewHierarchyChanged(details);
    // We're not using child == this because a Widget may not be
    // available when this is added to the hierarchy.
    if (details.is_add && GetWidget() && !example_view_created_) {
      example_view_created_ = true;
      example_base_->CreateExampleView(this);
    }
  }

  // True if the example view has already been created, or false otherwise.
  bool example_view_created_;

  ExampleBase* example_base_;

  DISALLOW_COPY_AND_ASSIGN(ContainerView);
};

}  // namespace

ExampleBase::~ExampleBase() {}

ExampleBase::ExampleBase(const char* title) : example_title_(title) {
  container_ = new ContainerView(this);
}

// Prints a message in the status area, at the bottom of the window.
void ExampleBase::PrintStatus(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  std::string msg;
  base::StringAppendV(&msg, format, ap);
  LogStatus(msg);
}

}  // namespace examples
}  // namespace views
