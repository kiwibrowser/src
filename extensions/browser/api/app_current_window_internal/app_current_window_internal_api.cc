// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/app_current_window_internal/app_current_window_internal_api.h"

#include <stdint.h>

#include <utility>

#include "base/command_line.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_client.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "extensions/browser/app_window/size_constraints.h"
#include "extensions/common/api/app_current_window_internal.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/switches.h"
#include "third_party/skia/include/core/SkRegion.h"

namespace app_current_window_internal =
    extensions::api::app_current_window_internal;

namespace Show = app_current_window_internal::Show;
namespace SetBounds = app_current_window_internal::SetBounds;
namespace SetSizeConstraints = app_current_window_internal::SetSizeConstraints;
namespace SetIcon = app_current_window_internal::SetIcon;
namespace SetShape = app_current_window_internal::SetShape;
namespace SetAlwaysOnTop = app_current_window_internal::SetAlwaysOnTop;
namespace SetVisibleOnAllWorkspaces =
    app_current_window_internal::SetVisibleOnAllWorkspaces;
namespace SetActivateOnPointer =
    app_current_window_internal::SetActivateOnPointer;

using app_current_window_internal::Bounds;
using app_current_window_internal::Region;
using app_current_window_internal::RegionRect;
using app_current_window_internal::SizeConstraints;

namespace extensions {

namespace {

const char kNoAssociatedAppWindow[] =
    "The context from which the function was called did not have an "
    "associated app window.";

}  // namespace

namespace bounds {

enum BoundsType {
  INNER_BOUNDS,
  OUTER_BOUNDS,
  DEPRECATED_BOUNDS,
  INVALID_TYPE
};

const char kInnerBoundsType[] = "innerBounds";
const char kOuterBoundsType[] = "outerBounds";
const char kDeprecatedBoundsType[] = "bounds";

BoundsType GetBoundsType(const std::string& type_as_string) {
  if (type_as_string == kInnerBoundsType)
    return INNER_BOUNDS;
  else if (type_as_string == kOuterBoundsType)
    return OUTER_BOUNDS;
  else if (type_as_string == kDeprecatedBoundsType)
    return DEPRECATED_BOUNDS;
  else
    return INVALID_TYPE;
}

}  // namespace bounds

bool AppCurrentWindowInternalExtensionFunction::PreRunValidation(
    std::string* error) {
  if (!UIThreadExtensionFunction::PreRunValidation(error))
    return false;

  AppWindowRegistry* registry = AppWindowRegistry::Get(browser_context());
  DCHECK(registry);
  content::WebContents* web_contents = GetSenderWebContents();
  if (!web_contents) {
    *error = "No valid web contents";
    return false;
  }
  window_ = registry->GetAppWindowForWebContents(web_contents);
  if (!window_) {
    *error = kNoAssociatedAppWindow;
    return false;
  }
  return true;
}

ExtensionFunction::ResponseAction AppCurrentWindowInternalFocusFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalFullscreenFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalMaximizeFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalMinimizeFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalRestoreFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalDrawAttentionFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalClearAttentionFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AppCurrentWindowInternalShowFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction AppCurrentWindowInternalHideFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalSetBoundsFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalSetSizeConstraintsFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalSetIconFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalSetShapeFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalSetAlwaysOnTopFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalSetVisibleOnAllWorkspacesFunction::Run() {
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction
AppCurrentWindowInternalSetActivateOnPointerFunction::Run() {
  return RespondNow(NoArguments());
}

}  // namespace extensions
