// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TRANSLATE_TRANSLATE_BUBBLE_VIEW_STATE_TRANSITION_H_
#define CHROME_BROWSER_UI_TRANSLATE_TRANSLATE_BUBBLE_VIEW_STATE_TRANSITION_H_

#include "base/macros.h"
#include "chrome/browser/ui/translate/translate_bubble_model.h"

namespace translate {

enum TranslateBubbleUiEvent {
  // Start with 1 to match existing UMA values: see http://crbug.com/612558
  // The user clicked the advanced option.
  SET_STATE_OPTIONS = 1,

  // The user clicked "Done" and went back from the advanced option.
  LEAVE_STATE_OPTIONS,

  // The user clicked the advanced link.
  ADVANCED_LINK_CLICKED,

  // The user checked the "always translate" checkbox.
  ALWAYS_TRANSLATE_CHECKED,

  // The user unchecked the "always translate" checkbox.
  ALWAYS_TRANSLATE_UNCHECKED,

  // The user selected "Nope" in the "Options" menu.
  NOPE_MENU_CLICKED,

  // The user selected "Never translate language" in the "Options" menu.
  NEVER_TRANSLATE_LANGUAGE_MENU_CLICKED,

  // The user selected "Never translate this site" in the "Options" menu.
  NEVER_TRANSLATE_SITE_MENU_CLICKED,

  // The user clicked the "Translate" button.
  TRANSLATE_BUTTON_CLICKED,

  // The user clicked the "Done" button.
  DONE_BUTTON_CLICKED,

  // The user clicked the "Cancel" button.
  CANCEL_BUTTON_CLICKED,

  // The user clicked the "Closed" [X] button.
  CLOSE_BUTTON_CLICKED,

  // The user clicked the "Try Again" button.
  TRY_AGAIN_BUTTON_CLICKED,

  // The user clicked the "Show Original" button.
  SHOW_ORIGINAL_BUTTON_CLICKED,

  // The user clicked the "Settings" link.
  SETTINGS_LINK_CLICKED,

  // The user changed the "Source language".
  SOURCE_LANGUAGE_MENU_CLICKED,

  // The user changed the "Target language".
  TARGET_LANGUAGE_MENU_CLICKED,

  // The user activated the translate page action icon.
  PAGE_ACTION_ICON_ACTIVATED,

  // The user deactivated the translate page action icon.
  PAGE_ACTION_ICON_DEACTIVATED,

  // The translate bubble was shown to the user.
  BUBBLE_SHOWN,

  // The translate bugbble could not be shown to the user, for various reasons.
  BUBBLE_NOT_SHOWN_WINDOW_NOT_VALID,
  BUBBLE_NOT_SHOWN_WINDOW_MINIMIZED,
  BUBBLE_NOT_SHOWN_WINDOW_NOT_ACTIVE,
  BUBBLE_NOT_SHOWN_WEB_CONTENTS_NOT_ACTIVE,
  BUBBLE_NOT_SHOWN_EDITABLE_FIELD_IS_ACTIVE,

  // The user clicked the advanced menu item.
  ADVANCED_MENU_CLICKED,

  // The user clicked the advanced button.
  ADVANCED_BUTTON_CLICKED,

  TRANSLATE_BUBBLE_UI_EVENT_MAX
};

// Logs metrics for the user's TranslateBubbleUiEvent |action|.
void ReportUiAction(translate::TranslateBubbleUiEvent action);

}  // namespace translate

// The class which manages the transition of the view state of the Translate
// bubble.
class TranslateBubbleViewStateTransition {
 public:
  explicit TranslateBubbleViewStateTransition(
      TranslateBubbleModel::ViewState view_state);

  TranslateBubbleModel::ViewState view_state() const { return view_state_; }

  // Transitions the view state.
  void SetViewState(TranslateBubbleModel::ViewState view_state);

  // Goes back from the 'Advanced' view state.
  void GoBackFromAdvanced();

 private:
  // The current view type.
  TranslateBubbleModel::ViewState view_state_;

  // The view type. When the current view type is not 'Advanced' view, this is
  // equivalent to |view_state_|. Otherwise, this is the previous view type
  // before the user opens the 'Advanced' view. This is used to navigate when
  // pressing 'Cancel' button on the 'Advanced' view.
  TranslateBubbleModel::ViewState view_state_before_advanced_view_;

  DISALLOW_COPY_AND_ASSIGN(TranslateBubbleViewStateTransition);
};

#endif  // CHROME_BROWSER_UI_TRANSLATE_TRANSLATE_BUBBLE_VIEW_STATE_TRANSITION_H_
