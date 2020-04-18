// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_NULL_RESOURCE_CONTROLLER_H_
#define CONTENT_BROWSER_LOADER_NULL_RESOURCE_CONTROLLER_H_

#include "base/macros.h"
#include "content/browser/loader/resource_controller.h"

namespace content {

// A pseudo-ResourceController that just tracks whether it was resumed or not.
// Only intended when calling into ResourceHandlers that either much resume the
// request, or when the consumer doesn't care if the request was resumed or not.
// TODO(mmenke): Modify consumers so they don't need to use this class.
class NullResourceController : public ResourceController {
 public:
  explicit NullResourceController(bool* was_resumed);
  ~NullResourceController() override;

  // ResourceController implementation:
  void Cancel() override;
  void CancelWithError(int error_code) override;
  void Resume() override;

 private:
  bool* was_resumed_;

  DISALLOW_COPY_AND_ASSIGN(NullResourceController);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_NULL_RESOURCE_CONTROLLER_H_
