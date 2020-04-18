// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_THEME_H_
#define CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_THEME_H_

#include "third_party/skia/include/core/SkColor.h"

// A part of the omnibox (location bar, location bar decoration, or dropdown).
enum class OmniboxPart {
  LOCATION_BAR_BACKGROUND,
  LOCATION_BAR_CLEAR_ALL,
  LOCATION_BAR_IME_AUTOCOMPLETE_BACKGROUND,
  LOCATION_BAR_IME_AUTOCOMPLETE_TEXT,
  LOCATION_BAR_SECURITY_CHIP,
  LOCATION_BAR_SELECTED_KEYWORD,
  LOCATION_BAR_TEXT_DEFAULT,
  LOCATION_BAR_TEXT_DIMMED,
  LOCATION_BAR_BUBBLE_OUTLINE,
  LOCATION_BAR_FOCUS_RING,

  RESULTS_BACKGROUND,  // Background of the results dropdown.
  RESULTS_ICON,
  RESULTS_TEXT_DEFAULT,
  RESULTS_TEXT_DIMMED,
  RESULTS_TEXT_INVISIBLE,
  RESULTS_TEXT_NEGATIVE,
  RESULTS_TEXT_POSITIVE,
  RESULTS_TEXT_URL,
};

// The tint of the omnibox theme. E.g. Incognito may use a DARK tint. NATIVE is
// only used on Desktop Linux.
enum class OmniboxTint { DARK, LIGHT, NATIVE };

// An optional state for a given |OmniboxPart|.
enum class OmniboxPartState {
  NORMAL,
  HOVERED,
  SELECTED,
  HOVERED_AND_SELECTED,

  // Applicable to LOCATION_BAR_SECURITY_CHIP only.
  CHIP_DEFAULT,
  CHIP_DANGEROUS,
  CHIP_SECURE
};

// Returns the color for the given |part| and |tint|. An optional |state| can be
// provided for OmniboxParts that support stateful colors.
SkColor GetOmniboxColor(OmniboxPart part,
                        OmniboxTint tint,
                        OmniboxPartState state = OmniboxPartState::NORMAL);

float GetOmniboxStateAlpha(OmniboxPartState state);

#endif  // CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_THEME_H_
