// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_ENUMS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_ENUMS_H_

#include "third_party/blink/renderer/core/core_export.h"

namespace blink {

enum AccessibilityRole {
  kUnknownRole = 0,  // Not mapped in platform APIs, generally indicates a bug
  kAbbrRole,         // No mapping to ARIA role.
  kAlertDialogRole,
  kAlertRole,
  kAnchorRole,      // No mapping to ARIA role.
  kAnnotationRole,  // No mapping to ARIA role.
  kApplicationRole,
  kArticleRole,
  kAudioRole,  // No mapping to ARIA role.
  kBannerRole,
  kBlockquoteRole,  // No mapping to ARIA role.
  kButtonRole,
  kCanvasRole,   // No mapping to ARIA role.
  kCaptionRole,  // No mapping to ARIA role.
  kCellRole,
  kCheckBoxRole,
  kColorWellRole,  // No mapping to ARIA role.
  kColumnHeaderRole,
  kColumnRole,  // No mapping to ARIA role.
  kComboBoxGroupingRole,
  kComboBoxMenuButtonRole,
  kComplementaryRole,
  kContentInfoRole,
  kDateRole,      // No mapping to ARIA role.
  kDateTimeRole,  // No mapping to ARIA role.
  kDefinitionRole,
  kDescriptionListDetailRole,  // No mapping to ARIA role.
  kDescriptionListRole,        // No mapping to ARIA role.
  kDescriptionListTermRole,    // No mapping to ARIA role.
  kDetailsRole,                // No mapping to ARIA role.
  kDialogRole,
  kDirectoryRole,
  kDisclosureTriangleRole,  // No mapping to ARIA role.
  kDocAbstractRole,
  kDocAcknowledgmentsRole,
  kDocAfterwordRole,
  kDocAppendixRole,
  // --------------------------------------------------------------
  // DPub Roles:
  // https://www.w3.org/TR/dpub-aam-1.0/#mapping_role_table
  kDocBackLinkRole,
  kDocBiblioEntryRole,
  kDocBibliographyRole,
  kDocBiblioRefRole,
  kDocChapterRole,
  kDocColophonRole,
  kDocConclusionRole,
  kDocCoverRole,
  kDocCreditRole,
  kDocCreditsRole,
  kDocDedicationRole,
  kDocEndnoteRole,
  kDocEndnotesRole,
  kDocEpigraphRole,
  kDocEpilogueRole,
  kDocErrataRole,
  kDocExampleRole,
  kDocFootnoteRole,
  kDocForewordRole,
  kDocGlossaryRole,
  kDocGlossRefRole,
  kDocIndexRole,
  kDocIntroductionRole,
  kDocNoteRefRole,
  kDocNoticeRole,
  kDocPageBreakRole,
  kDocPageListRole,
  kDocPartRole,
  kDocPrefaceRole,
  kDocPrologueRole,
  kDocPullquoteRole,
  kDocQnaRole,
  kDocSubtitleRole,
  kDocTipRole,
  kDocTocRole,
  // End DPub roles.
  // --------------------------------------------------------------
  kDocumentRole,
  kEmbeddedObjectRole,  // No mapping to ARIA role.
  kFeedRole,
  kFigcaptionRole,  // No mapping to ARIA role.
  kFigureRole,
  kFooterRole,
  kFormRole,
  kGenericContainerRole,  // No role was defined for this container
  // --------------------------------------------------------------
  // ARIA Graphics module roles:
  // https://rawgit.com/w3c/graphics-aam/master/#mapping_role_table
  kGraphicsDocumentRole,
  kGraphicsObjectRole,
  kGraphicsSymbolRole,
  // End ARIA Graphics module roles.
  // --------------------------------------------------------------
  kGridRole,
  kGroupRole,
  kHeadingRole,
  kIframePresentationalRole,  // No mapping to ARIA role.
  kIframeRole,                // No mapping to ARIA role.
  kIgnoredRole,               // No mapping to ARIA role.
  kImageMapRole,              // No mapping to ARIA role.
  kImageRole,
  kInlineTextBoxRole,  // No mapping to ARIA role.
  kInputTimeRole,      // No mapping to ARIA role.
  kLabelRole,
  kLayoutTableRole,
  kLayoutTableCellRole,
  kLayoutTableColumnRole,
  kLayoutTableRowRole,
  kLegendRole,     // No mapping to ARIA role.
  kLineBreakRole,  // No mapping to ARIA role.
  kLinkRole,
  kListBoxOptionRole,
  kListBoxRole,
  kListItemRole,
  kListMarkerRole,  // No mapping to ARIA role.
  kListRole,
  kLogRole,
  kMainRole,
  kMarkRole,  // No mapping to ARIA role.
  kMarqueeRole,
  kMathRole,
  kMenuBarRole,
  kMenuButtonRole,
  kMenuItemRole,
  kMenuItemCheckBoxRole,
  kMenuItemRadioRole,
  kMenuListOptionRole,
  kMenuListPopupRole,
  kMenuRole,
  kMeterRole,
  kNavigationRole,
  kNoneRole,  // ARIA role of "none"
  kNoteRole,
  kParagraphRole,  // No mapping to ARIA role.
  kPopUpButtonRole,
  kPreRole,  // No mapping to ARIA role.
  kPresentationalRole,
  kProgressIndicatorRole,
  kRadioButtonRole,
  kRadioGroupRole,
  kRegionRole,
  kRowHeaderRole,
  kRowRole,
  kRubyRole,     // No mapping to ARIA role.
  kSVGRootRole,  // No mapping to ARIA role.
  kScrollBarRole,
  kSearchRole,
  kSearchBoxRole,
  kSliderRole,
  kSliderThumbRole,     // No mapping to ARIA role.
  kSpinButtonRole,
  kSplitterRole,
  kStaticTextRole,  // No mapping to ARIA role.
  kStatusRole,
  kSwitchRole,
  kTabListRole,
  kTabPanelRole,
  kTabRole,
  kTableHeaderContainerRole,  // No mapping to ARIA role.
  kTableRole,
  kTermRole,
  kTextFieldRole,
  kTextFieldWithComboBoxRole,
  kTimeRole,  // No mapping to ARIA role.
  kTimerRole,
  kToggleButtonRole,
  kToolbarRole,
  kTreeGridRole,
  kTreeItemRole,
  kTreeRole,
  kUserInterfaceTooltipRole,
  kVideoRole,    // No mapping to ARIA role.
  kWebAreaRole,  // No mapping to ARIA role.
  kNumRoles
};

enum AccessibilityOrientation {
  kAccessibilityOrientationUndefined = 0,
  kAccessibilityOrientationVertical,
  kAccessibilityOrientationHorizontal,
};

enum class AXDefaultActionVerb {
  kNone = 0,
  kActivate,
  kCheck,
  kClick,

  // A click will be performed on one of the object's ancestors.
  // This happens when the object itself is not clickable, but one of its
  // ancestors has click handlers attached which are able to capture the click
  // as it bubbles up.
  kClickAncestor,

  kJump,
  kOpen,
  kPress,
  kSelect,
  kUncheck
};

// The input restriction on an object.
enum AXRestriction {
  kNone = 0,  // An object that is not disabled.
  kReadOnly,
  kDisabled,
};

enum class AXSupportedAction {
  kNone = 0,
  kActivate,
  kCheck,
  kClick,
  kJump,
  kOpen,
  kPress,
  kSelect,
  kUncheck
};

enum AccessibilityTextDirection {
  kAccessibilityTextDirectionLTR,
  kAccessibilityTextDirectionRTL,
  kAccessibilityTextDirectionTTB,
  kAccessibilityTextDirectionBTT
};

enum AXTextPosition {
  kAXTextPositionNone = 0,
  kAXTextPositionSubscript,
  kAXTextPositionSuperscript
};

enum SortDirection {
  kSortDirectionUndefined = 0,
  kSortDirectionNone,
  kSortDirectionAscending,
  kSortDirectionDescending,
  kSortDirectionOther
};

enum AccessibilityExpanded {
  kExpandedUndefined = 0,
  kExpandedCollapsed,
  kExpandedExpanded,
};

enum AccessibilitySelectedState {
  kSelectedStateUndefined = 0,
  kSelectedStateFalse,
  kSelectedStateTrue,
};

enum AriaCurrentState {
  kAriaCurrentStateUndefined = 0,
  kAriaCurrentStateFalse,
  kAriaCurrentStateTrue,
  kAriaCurrentStatePage,
  kAriaCurrentStateStep,
  kAriaCurrentStateLocation,
  kAriaCurrentStateDate,
  kAriaCurrentStateTime
};

enum AXHasPopup {
  kAXHasPopupFalse = 0,
  kAXHasPopupTrue,
  kAXHasPopupMenu,
  kAXHasPopupListbox,
  kAXHasPopupTree,
  kAXHasPopupGrid,
  kAXHasPopupDialog
};

enum InvalidState {
  kInvalidStateUndefined = 0,
  kInvalidStateFalse,
  kInvalidStateTrue,
  kInvalidStateSpelling,
  kInvalidStateGrammar,
  kInvalidStateOther
};

enum TextStyle {
  kTextStyleNone = 0,
  kTextStyleBold = 1 << 0,
  kTextStyleItalic = 1 << 1,
  kTextStyleUnderline = 1 << 2,
  kTextStyleLineThrough = 1 << 3
};

enum class AXBoolAttribute {
  kAriaBusy,
};

enum class AXStringAttribute {
  kAriaKeyShortcuts,
  kAriaRoleDescription,
};

enum class AXObjectAttribute {
  kAriaActiveDescendant,
  kAriaDetails,
  kAriaErrorMessage,
};

enum class AXObjectVectorAttribute {
  kAriaControls,
  kAriaFlowTo,
};

// The source of the accessible name of an element. This is needed
// because on some platforms this determines how the accessible name
// is exposed.
enum AXNameFrom {
  kAXNameFromUninitialized = -1,
  kAXNameFromAttribute = 0,
  kAXNameFromAttributeExplicitlyEmpty,
  kAXNameFromCaption,
  kAXNameFromContents,
  kAXNameFromPlaceholder,
  kAXNameFromRelatedElement,
  kAXNameFromValue,
  kAXNameFromTitle,
};

// The source of the accessible description of an element. This is needed
// because on some platforms this determines how the accessible description
// is exposed.
enum AXDescriptionFrom {
  kAXDescriptionFromUninitialized = -1,
  kAXDescriptionFromAttribute = 0,
  kAXDescriptionFromContents,
  kAXDescriptionFromRelatedElement,
};

enum AXObjectInclusion {
  kIncludeObject,
  kIgnoreObject,
  kDefaultBehavior,
};

enum AccessibilityCheckedState {
  kCheckedStateUndefined = 0,
  kCheckedStateFalse,
  kCheckedStateTrue,
  kCheckedStateMixed
};

enum AccessibilityOptionalBool {
  kOptionalBoolUndefined = 0,
  kOptionalBoolTrue,
  kOptionalBoolFalse
};

// The potential native HTML-based text (name, description or placeholder)
// sources for an element.  See
// http://rawgit.com/w3c/aria/master/html-aam/html-aam.html#accessible-name-and-description-calculation
enum AXTextFromNativeHTML {
  kAXTextFromNativeHTMLUninitialized = -1,
  kAXTextFromNativeHTMLFigcaption,
  kAXTextFromNativeHTMLLabel,
  kAXTextFromNativeHTMLLabelFor,
  kAXTextFromNativeHTMLLabelWrapped,
  kAXTextFromNativeHTMLLegend,
  kAXTextFromNativeHTMLTableCaption,
  kAXTextFromNativeHTMLTitleElement,
};

enum AXIgnoredReason {
  kAXActiveModalDialog,
  kAXAncestorIsLeafNode,
  kAXAriaHiddenElement,
  kAXAriaHiddenSubtree,
  kAXEmptyAlt,
  kAXEmptyText,
  kAXInertElement,
  kAXInertSubtree,
  kAXInheritsPresentation,
  kAXLabelContainer,
  kAXLabelFor,
  kAXNotRendered,
  kAXNotVisible,
  kAXPresentationalRole,
  kAXProbablyPresentational,
  kAXStaticTextUsedAsNameFor,
  kAXUninteresting
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ACCESSIBILITY_AX_ENUMS_H_
