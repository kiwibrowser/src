// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/blink_ax_enum_conversion.h"

#include "base/logging.h"

namespace content {

void AXStateFromBlink(const blink::WebAXObject& o, ui::AXNodeData* dst) {
  blink::WebAXExpanded expanded = o.IsExpanded();
  if (expanded) {
    if (expanded == blink::kWebAXExpandedCollapsed)
      dst->AddState(ax::mojom::State::kCollapsed);
    else if (expanded == blink::kWebAXExpandedExpanded)
      dst->AddState(ax::mojom::State::kExpanded);
  }

  if (o.CanSetFocusAttribute())
    dst->AddState(ax::mojom::State::kFocusable);

  if (o.HasPopup())
    dst->SetHasPopup(AXHasPopupFromBlink(o.HasPopup()));
  else if (o.Role() == blink::kWebAXRolePopUpButton)
    dst->SetHasPopup(ax::mojom::HasPopup::kMenu);

  if (o.IsHovered())
    dst->AddState(ax::mojom::State::kHovered);

  if (!o.IsVisible())
    dst->AddState(ax::mojom::State::kInvisible);

  if (o.IsLinked())
    dst->AddState(ax::mojom::State::kLinked);

  if (o.IsMultiline())
    dst->AddState(ax::mojom::State::kMultiline);

  if (o.IsMultiSelectable())
    dst->AddState(ax::mojom::State::kMultiselectable);

  if (o.IsPasswordField())
    dst->AddState(ax::mojom::State::kProtected);

  if (o.IsRequired())
    dst->AddState(ax::mojom::State::kRequired);

  if (o.IsEditable())
    dst->AddState(ax::mojom::State::kEditable);

  if (o.IsSelected() != blink::kWebAXSelectedStateUndefined) {
    dst->AddBoolAttribute(ax::mojom::BoolAttribute::kSelected,
                          o.IsSelected() == blink::kWebAXSelectedStateTrue);
  }

  if (o.IsRichlyEditable())
    dst->AddState(ax::mojom::State::kRichlyEditable);

  if (o.IsVisited())
    dst->AddState(ax::mojom::State::kVisited);

  if (o.Orientation() == blink::kWebAXOrientationVertical)
    dst->AddState(ax::mojom::State::kVertical);
  else if (o.Orientation() == blink::kWebAXOrientationHorizontal)
    dst->AddState(ax::mojom::State::kHorizontal);

  if (o.IsVisited())
    dst->AddState(ax::mojom::State::kVisited);
}

ax::mojom::Role AXRoleFromBlink(blink::WebAXRole role) {
  switch (role) {
    case blink::kWebAXRoleAbbr:
      return ax::mojom::Role::kAbbr;
    case blink::kWebAXRoleAlert:
      return ax::mojom::Role::kAlert;
    case blink::kWebAXRoleAlertDialog:
      return ax::mojom::Role::kAlertDialog;
    case blink::kWebAXRoleAnchor:
      return ax::mojom::Role::kAnchor;
    case blink::kWebAXRoleAnnotation:
      return ax::mojom::Role::kAnnotation;
    case blink::kWebAXRoleApplication:
      return ax::mojom::Role::kApplication;
    case blink::kWebAXRoleArticle:
      return ax::mojom::Role::kArticle;
    case blink::kWebAXRoleAudio:
      return ax::mojom::Role::kAudio;
    case blink::kWebAXRoleBanner:
      return ax::mojom::Role::kBanner;
    case blink::kWebAXRoleBlockquote:
      return ax::mojom::Role::kBlockquote;
    case blink::kWebAXRoleButton:
      return ax::mojom::Role::kButton;
    case blink::kWebAXRoleCanvas:
      return ax::mojom::Role::kCanvas;
    case blink::kWebAXRoleCaption:
      return ax::mojom::Role::kCaption;
    case blink::kWebAXRoleCell:
      return ax::mojom::Role::kCell;
    case blink::kWebAXRoleCheckBox:
      return ax::mojom::Role::kCheckBox;
    case blink::kWebAXRoleColorWell:
      return ax::mojom::Role::kColorWell;
    case blink::kWebAXRoleColumn:
      return ax::mojom::Role::kColumn;
    case blink::kWebAXRoleColumnHeader:
      return ax::mojom::Role::kColumnHeader;
    case blink::kWebAXRoleComboBoxGrouping:
      return ax::mojom::Role::kComboBoxGrouping;
    case blink::kWebAXRoleComboBoxMenuButton:
      return ax::mojom::Role::kComboBoxMenuButton;
    case blink::kWebAXRoleComplementary:
      return ax::mojom::Role::kComplementary;
    case blink::kWebAXRoleContentInfo:
      return ax::mojom::Role::kContentInfo;
    case blink::kWebAXRoleDate:
      return ax::mojom::Role::kDate;
    case blink::kWebAXRoleDateTime:
      return ax::mojom::Role::kDateTime;
    case blink::kWebAXRoleDefinition:
      return ax::mojom::Role::kDefinition;
    case blink::kWebAXRoleDescriptionListDetail:
      return ax::mojom::Role::kDescriptionListDetail;
    case blink::kWebAXRoleDescriptionList:
      return ax::mojom::Role::kDescriptionList;
    case blink::kWebAXRoleDescriptionListTerm:
      return ax::mojom::Role::kDescriptionListTerm;
    case blink::kWebAXRoleDetails:
      return ax::mojom::Role::kDetails;
    case blink::kWebAXRoleDialog:
      return ax::mojom::Role::kDialog;
    case blink::kWebAXRoleDirectory:
      return ax::mojom::Role::kDirectory;
    case blink::kWebAXRoleDisclosureTriangle:
      return ax::mojom::Role::kDisclosureTriangle;
    case blink::kWebAXRoleDocAbstract:
      return ax::mojom::Role::kDocAbstract;
    case blink::kWebAXRoleDocAcknowledgments:
      return ax::mojom::Role::kDocAcknowledgments;
    case blink::kWebAXRoleDocAfterword:
      return ax::mojom::Role::kDocAfterword;
    case blink::kWebAXRoleDocAppendix:
      return ax::mojom::Role::kDocAppendix;
    case blink::kWebAXRoleDocBackLink:
      return ax::mojom::Role::kDocBackLink;
    case blink::kWebAXRoleDocBiblioEntry:
      return ax::mojom::Role::kDocBiblioEntry;
    case blink::kWebAXRoleDocBibliography:
      return ax::mojom::Role::kDocBibliography;
    case blink::kWebAXRoleDocBiblioRef:
      return ax::mojom::Role::kDocBiblioRef;
    case blink::kWebAXRoleDocChapter:
      return ax::mojom::Role::kDocChapter;
    case blink::kWebAXRoleDocColophon:
      return ax::mojom::Role::kDocColophon;
    case blink::kWebAXRoleDocConclusion:
      return ax::mojom::Role::kDocConclusion;
    case blink::kWebAXRoleDocCover:
      return ax::mojom::Role::kDocCover;
    case blink::kWebAXRoleDocCredit:
      return ax::mojom::Role::kDocCredit;
    case blink::kWebAXRoleDocCredits:
      return ax::mojom::Role::kDocCredits;
    case blink::kWebAXRoleDocDedication:
      return ax::mojom::Role::kDocDedication;
    case blink::kWebAXRoleDocEndnote:
      return ax::mojom::Role::kDocEndnote;
    case blink::kWebAXRoleDocEndnotes:
      return ax::mojom::Role::kDocEndnotes;
    case blink::kWebAXRoleDocEpigraph:
      return ax::mojom::Role::kDocEpigraph;
    case blink::kWebAXRoleDocEpilogue:
      return ax::mojom::Role::kDocEpilogue;
    case blink::kWebAXRoleDocErrata:
      return ax::mojom::Role::kDocErrata;
    case blink::kWebAXRoleDocExample:
      return ax::mojom::Role::kDocExample;
    case blink::kWebAXRoleDocFootnote:
      return ax::mojom::Role::kDocFootnote;
    case blink::kWebAXRoleDocForeword:
      return ax::mojom::Role::kDocForeword;
    case blink::kWebAXRoleDocGlossary:
      return ax::mojom::Role::kDocGlossary;
    case blink::kWebAXRoleDocGlossRef:
      return ax::mojom::Role::kDocGlossRef;
    case blink::kWebAXRoleDocIndex:
      return ax::mojom::Role::kDocIndex;
    case blink::kWebAXRoleDocIntroduction:
      return ax::mojom::Role::kDocIntroduction;
    case blink::kWebAXRoleDocNoteRef:
      return ax::mojom::Role::kDocNoteRef;
    case blink::kWebAXRoleDocNotice:
      return ax::mojom::Role::kDocNotice;
    case blink::kWebAXRoleDocPageBreak:
      return ax::mojom::Role::kDocPageBreak;
    case blink::kWebAXRoleDocPageList:
      return ax::mojom::Role::kDocPageList;
    case blink::kWebAXRoleDocPart:
      return ax::mojom::Role::kDocPart;
    case blink::kWebAXRoleDocPreface:
      return ax::mojom::Role::kDocPreface;
    case blink::kWebAXRoleDocPrologue:
      return ax::mojom::Role::kDocPrologue;
    case blink::kWebAXRoleDocPullquote:
      return ax::mojom::Role::kDocPullquote;
    case blink::kWebAXRoleDocQna:
      return ax::mojom::Role::kDocQna;
    case blink::kWebAXRoleDocSubtitle:
      return ax::mojom::Role::kDocSubtitle;
    case blink::kWebAXRoleDocTip:
      return ax::mojom::Role::kDocTip;
    case blink::kWebAXRoleDocToc:
      return ax::mojom::Role::kDocToc;
    case blink::kWebAXRoleDocument:
      return ax::mojom::Role::kDocument;
    case blink::kWebAXRoleEmbeddedObject:
      return ax::mojom::Role::kEmbeddedObject;
    case blink::kWebAXRoleFeed:
      return ax::mojom::Role::kFeed;
    case blink::kWebAXRoleFigcaption:
      return ax::mojom::Role::kFigcaption;
    case blink::kWebAXRoleFigure:
      return ax::mojom::Role::kFigure;
    case blink::kWebAXRoleFooter:
      return ax::mojom::Role::kFooter;
    case blink::kWebAXRoleForm:
      return ax::mojom::Role::kForm;
    case blink::kWebAXRoleGenericContainer:
      return ax::mojom::Role::kGenericContainer;
    case blink::kWebAXRoleGraphicsDocument:
      return ax::mojom::Role::kGraphicsDocument;
    case blink::kWebAXRoleGraphicsObject:
      return ax::mojom::Role::kGraphicsObject;
    case blink::kWebAXRoleGraphicsSymbol:
      return ax::mojom::Role::kGraphicsSymbol;
    case blink::kWebAXRoleGrid:
      return ax::mojom::Role::kGrid;
    case blink::kWebAXRoleGroup:
      return ax::mojom::Role::kGroup;
    case blink::kWebAXRoleHeading:
      return ax::mojom::Role::kHeading;
    case blink::kWebAXRoleIframe:
      return ax::mojom::Role::kIframe;
    case blink::kWebAXRoleIframePresentational:
      return ax::mojom::Role::kIframePresentational;
    case blink::kWebAXRoleIgnored:
      return ax::mojom::Role::kIgnored;
    case blink::kWebAXRoleImage:
      return ax::mojom::Role::kImage;
    case blink::kWebAXRoleImageMap:
      return ax::mojom::Role::kImageMap;
    case blink::kWebAXRoleInlineTextBox:
      return ax::mojom::Role::kInlineTextBox;
    case blink::kWebAXRoleInputTime:
      return ax::mojom::Role::kInputTime;
    case blink::kWebAXRoleLabel:
      return ax::mojom::Role::kLabelText;
    case blink::kWebAXRoleLayoutTable:
      return ax::mojom::Role::kLayoutTable;
    case blink::kWebAXRoleLayoutTableCell:
      return ax::mojom::Role::kLayoutTableCell;
    case blink::kWebAXRoleLayoutTableColumn:
      return ax::mojom::Role::kLayoutTableColumn;
    case blink::kWebAXRoleLayoutTableRow:
      return ax::mojom::Role::kLayoutTableRow;
    case blink::kWebAXRoleLegend:
      return ax::mojom::Role::kLegend;
    case blink::kWebAXRoleLink:
      return ax::mojom::Role::kLink;
    case blink::kWebAXRoleList:
      return ax::mojom::Role::kList;
    case blink::kWebAXRoleListBox:
      return ax::mojom::Role::kListBox;
    case blink::kWebAXRoleListBoxOption:
      return ax::mojom::Role::kListBoxOption;
    case blink::kWebAXRoleListItem:
      return ax::mojom::Role::kListItem;
    case blink::kWebAXRoleListMarker:
      return ax::mojom::Role::kListMarker;
    case blink::kWebAXRoleLog:
      return ax::mojom::Role::kLog;
    case blink::kWebAXRoleMain:
      return ax::mojom::Role::kMain;
    case blink::kWebAXRoleMarquee:
      return ax::mojom::Role::kMarquee;
    case blink::kWebAXRoleMark:
      return ax::mojom::Role::kMark;
    case blink::kWebAXRoleMath:
      return ax::mojom::Role::kMath;
    case blink::kWebAXRoleMenu:
      return ax::mojom::Role::kMenu;
    case blink::kWebAXRoleMenuBar:
      return ax::mojom::Role::kMenuBar;
    case blink::kWebAXRoleMenuButton:
      return ax::mojom::Role::kMenuButton;
    case blink::kWebAXRoleMenuItem:
      return ax::mojom::Role::kMenuItem;
    case blink::kWebAXRoleMenuItemCheckBox:
      return ax::mojom::Role::kMenuItemCheckBox;
    case blink::kWebAXRoleMenuItemRadio:
      return ax::mojom::Role::kMenuItemRadio;
    case blink::kWebAXRoleMenuListOption:
      return ax::mojom::Role::kMenuListOption;
    case blink::kWebAXRoleMenuListPopup:
      return ax::mojom::Role::kMenuListPopup;
    case blink::kWebAXRoleMeter:
      return ax::mojom::Role::kMeter;
    case blink::kWebAXRoleNavigation:
      return ax::mojom::Role::kNavigation;
    case blink::kWebAXRoleNone:
      return ax::mojom::Role::kNone;
    case blink::kWebAXRoleNote:
      return ax::mojom::Role::kNote;
    case blink::kWebAXRoleParagraph:
      return ax::mojom::Role::kParagraph;
    case blink::kWebAXRolePopUpButton:
      return ax::mojom::Role::kPopUpButton;
    case blink::kWebAXRolePre:
      return ax::mojom::Role::kPre;
    case blink::kWebAXRolePresentational:
      return ax::mojom::Role::kPresentational;
    case blink::kWebAXRoleProgressIndicator:
      return ax::mojom::Role::kProgressIndicator;
    case blink::kWebAXRoleRadioButton:
      return ax::mojom::Role::kRadioButton;
    case blink::kWebAXRoleRadioGroup:
      return ax::mojom::Role::kRadioGroup;
    case blink::kWebAXRoleRegion:
      return ax::mojom::Role::kRegion;
    case blink::kWebAXRoleRow:
      return ax::mojom::Role::kRow;
    case blink::kWebAXRoleRuby:
      return ax::mojom::Role::kRuby;
    case blink::kWebAXRoleRowHeader:
      return ax::mojom::Role::kRowHeader;
    case blink::kWebAXRoleSVGRoot:
      return ax::mojom::Role::kSvgRoot;
    case blink::kWebAXRoleScrollBar:
      return ax::mojom::Role::kScrollBar;
    case blink::kWebAXRoleSearch:
      return ax::mojom::Role::kSearch;
    case blink::kWebAXRoleSearchBox:
      return ax::mojom::Role::kSearchBox;
    case blink::kWebAXRoleSlider:
      return ax::mojom::Role::kSlider;
    case blink::kWebAXRoleSliderThumb:
      return ax::mojom::Role::kSliderThumb;
    case blink::kWebAXRoleSpinButton:
      return ax::mojom::Role::kSpinButton;
    case blink::kWebAXRoleSplitter:
      return ax::mojom::Role::kSplitter;
    case blink::kWebAXRoleStaticText:
      return ax::mojom::Role::kStaticText;
    case blink::kWebAXRoleStatus:
      return ax::mojom::Role::kStatus;
    case blink::kWebAXRoleSwitch:
      return ax::mojom::Role::kSwitch;
    case blink::kWebAXRoleTab:
      return ax::mojom::Role::kTab;
    case blink::kWebAXRoleTabList:
      return ax::mojom::Role::kTabList;
    case blink::kWebAXRoleTabPanel:
      return ax::mojom::Role::kTabPanel;
    case blink::kWebAXRoleTable:
      return ax::mojom::Role::kTable;
    case blink::kWebAXRoleTableHeaderContainer:
      return ax::mojom::Role::kTableHeaderContainer;
    case blink::kWebAXRoleTerm:
      return ax::mojom::Role::kTerm;
    case blink::kWebAXRoleTextField:
      return ax::mojom::Role::kTextField;
    case blink::kWebAXRoleTextFieldWithComboBox:
      return ax::mojom::Role::kTextFieldWithComboBox;
    case blink::kWebAXRoleTime:
      return ax::mojom::Role::kTime;
    case blink::kWebAXRoleTimer:
      return ax::mojom::Role::kTimer;
    case blink::kWebAXRoleToggleButton:
      return ax::mojom::Role::kToggleButton;
    case blink::kWebAXRoleToolbar:
      return ax::mojom::Role::kToolbar;
    case blink::kWebAXRoleTree:
      return ax::mojom::Role::kTree;
    case blink::kWebAXRoleTreeGrid:
      return ax::mojom::Role::kTreeGrid;
    case blink::kWebAXRoleTreeItem:
      return ax::mojom::Role::kTreeItem;
    case blink::kWebAXRoleUnknown:
      return ax::mojom::Role::kUnknown;
    case blink::kWebAXRoleUserInterfaceTooltip:
      return ax::mojom::Role::kTooltip;
    case blink::kWebAXRoleVideo:
      return ax::mojom::Role::kVideo;
    case blink::kWebAXRoleWebArea:
      return ax::mojom::Role::kRootWebArea;
    case blink::kWebAXRoleLineBreak:
      return ax::mojom::Role::kLineBreak;
    default:
      return ax::mojom::Role::kUnknown;
  }
}

ax::mojom::Event AXEventFromBlink(blink::WebAXEvent event) {
  switch (event) {
    case blink::kWebAXEventActiveDescendantChanged:
      return ax::mojom::Event::kActiveDescendantChanged;
    case blink::kWebAXEventAriaAttributeChanged:
      return ax::mojom::Event::kAriaAttributeChanged;
    case blink::kWebAXEventAutocorrectionOccured:
      return ax::mojom::Event::kAutocorrectionOccured;
    case blink::kWebAXEventBlur:
      return ax::mojom::Event::kBlur;
    case blink::kWebAXEventCheckedStateChanged:
      return ax::mojom::Event::kCheckedStateChanged;
    case blink::kWebAXEventChildrenChanged:
      return ax::mojom::Event::kChildrenChanged;
    case blink::kWebAXEventClicked:
      return ax::mojom::Event::kClicked;
    case blink::kWebAXEventDocumentSelectionChanged:
      return ax::mojom::Event::kDocumentSelectionChanged;
    case blink::kWebAXEventExpandedChanged:
      return ax::mojom::Event::kExpandedChanged;
    case blink::kWebAXEventFocus:
      return ax::mojom::Event::kFocus;
    case blink::kWebAXEventHover:
      return ax::mojom::Event::kHover;
    case blink::kWebAXEventInvalidStatusChanged:
      return ax::mojom::Event::kInvalidStatusChanged;
    case blink::kWebAXEventLayoutComplete:
      return ax::mojom::Event::kLayoutComplete;
    case blink::kWebAXEventLiveRegionChanged:
      return ax::mojom::Event::kLiveRegionChanged;
    case blink::kWebAXEventLoadComplete:
      return ax::mojom::Event::kLoadComplete;
    case blink::kWebAXEventLocationChanged:
      return ax::mojom::Event::kLocationChanged;
    case blink::kWebAXEventMenuListItemSelected:
      return ax::mojom::Event::kMenuListItemSelected;
    case blink::kWebAXEventMenuListItemUnselected:
      return ax::mojom::Event::kMenuListItemSelected;
    case blink::kWebAXEventMenuListValueChanged:
      return ax::mojom::Event::kMenuListValueChanged;
    case blink::kWebAXEventRowCollapsed:
      return ax::mojom::Event::kRowCollapsed;
    case blink::kWebAXEventRowCountChanged:
      return ax::mojom::Event::kRowCountChanged;
    case blink::kWebAXEventRowExpanded:
      return ax::mojom::Event::kRowExpanded;
    case blink::kWebAXEventScrollPositionChanged:
      return ax::mojom::Event::kScrollPositionChanged;
    case blink::kWebAXEventScrolledToAnchor:
      return ax::mojom::Event::kScrolledToAnchor;
    case blink::kWebAXEventSelectedChildrenChanged:
      return ax::mojom::Event::kSelectedChildrenChanged;
    case blink::kWebAXEventSelectedTextChanged:
      return ax::mojom::Event::kTextSelectionChanged;
    case blink::kWebAXEventTextChanged:
      return ax::mojom::Event::kTextChanged;
    case blink::kWebAXEventValueChanged:
      return ax::mojom::Event::kValueChanged;
    default:
      // We can't add an assertion here, that prevents us
      // from adding new event enums in Blink.
      return ax::mojom::Event::kNone;
  }
}

ax::mojom::DefaultActionVerb AXDefaultActionVerbFromBlink(
    blink::WebAXDefaultActionVerb action_verb) {
  switch (action_verb) {
    case blink::WebAXDefaultActionVerb::kNone:
      return ax::mojom::DefaultActionVerb::kNone;
    case blink::WebAXDefaultActionVerb::kActivate:
      return ax::mojom::DefaultActionVerb::kActivate;
    case blink::WebAXDefaultActionVerb::kCheck:
      return ax::mojom::DefaultActionVerb::kCheck;
    case blink::WebAXDefaultActionVerb::kClick:
      return ax::mojom::DefaultActionVerb::kClick;
    case blink::WebAXDefaultActionVerb::kClickAncestor:
      return ax::mojom::DefaultActionVerb::kClickAncestor;
    case blink::WebAXDefaultActionVerb::kJump:
      return ax::mojom::DefaultActionVerb::kJump;
    case blink::WebAXDefaultActionVerb::kOpen:
      return ax::mojom::DefaultActionVerb::kOpen;
    case blink::WebAXDefaultActionVerb::kPress:
      return ax::mojom::DefaultActionVerb::kPress;
    case blink::WebAXDefaultActionVerb::kSelect:
      return ax::mojom::DefaultActionVerb::kSelect;
    case blink::WebAXDefaultActionVerb::kUncheck:
      return ax::mojom::DefaultActionVerb::kUncheck;
  }
  NOTREACHED();
  return ax::mojom::DefaultActionVerb::kNone;
}

ax::mojom::MarkerType AXMarkerTypeFromBlink(
    blink::WebAXMarkerType marker_type) {
  switch (marker_type) {
    case blink::kWebAXMarkerTypeSpelling:
      return ax::mojom::MarkerType::kSpelling;
    case blink::kWebAXMarkerTypeGrammar:
      return ax::mojom::MarkerType::kGrammar;
    case blink::kWebAXMarkerTypeTextMatch:
      return ax::mojom::MarkerType::kTextMatch;
    case blink::kWebAXMarkerTypeActiveSuggestion:
      return ax::mojom::MarkerType::kActiveSuggestion;
    case blink::kWebAXMarkerTypeSuggestion:
      return ax::mojom::MarkerType::kSuggestion;
  }
  NOTREACHED();
  return ax::mojom::MarkerType::kNone;
}

ax::mojom::TextDirection AXTextDirectionFromBlink(
    blink::WebAXTextDirection text_direction) {
  switch (text_direction) {
    case blink::kWebAXTextDirectionLR:
      return ax::mojom::TextDirection::kLtr;
    case blink::kWebAXTextDirectionRL:
      return ax::mojom::TextDirection::kRtl;
    case blink::kWebAXTextDirectionTB:
      return ax::mojom::TextDirection::kTtb;
    case blink::kWebAXTextDirectionBT:
      return ax::mojom::TextDirection::kBtt;
  }
  NOTREACHED();
  return ax::mojom::TextDirection::kNone;
}

ax::mojom::TextPosition AXTextPositionFromBlink(
    blink::WebAXTextPosition text_position) {
  switch (text_position) {
    case blink::kWebAXTextPositionNone:
      return ax::mojom::TextPosition::kNone;
    case blink::kWebAXTextPositionSubscript:
      return ax::mojom::TextPosition::kSubscript;
    case blink::kWebAXTextPositionSuperscript:
      return ax::mojom::TextPosition::kSuperscript;
  }
  NOTREACHED();
  return ax::mojom::TextPosition::kNone;
}

ax::mojom::TextStyle AXTextStyleFromBlink(blink::WebAXTextStyle text_style) {
  uint32_t browser_text_style =
      static_cast<uint32_t>(ax::mojom::TextStyle::kNone);
  if (text_style & blink::kWebAXTextStyleBold)
    browser_text_style |=
        static_cast<int32_t>(ax::mojom::TextStyle::kTextStyleBold);
  if (text_style & blink::kWebAXTextStyleItalic)
    browser_text_style |=
        static_cast<int32_t>(ax::mojom::TextStyle::kTextStyleItalic);
  if (text_style & blink::kWebAXTextStyleUnderline)
    browser_text_style |=
        static_cast<int32_t>(ax::mojom::TextStyle::kTextStyleUnderline);
  if (text_style & blink::kWebAXTextStyleLineThrough)
    browser_text_style |=
        static_cast<int32_t>(ax::mojom::TextStyle::kTextStyleLineThrough);
  return static_cast<ax::mojom::TextStyle>(browser_text_style);
}

ax::mojom::AriaCurrentState AXAriaCurrentStateFromBlink(
    blink::WebAXAriaCurrentState aria_current_state) {
  switch (aria_current_state) {
    case blink::kWebAXAriaCurrentStateUndefined:
      return ax::mojom::AriaCurrentState::kNone;
    case blink::kWebAXAriaCurrentStateFalse:
      return ax::mojom::AriaCurrentState::kFalse;
    case blink::kWebAXAriaCurrentStateTrue:
      return ax::mojom::AriaCurrentState::kTrue;
    case blink::kWebAXAriaCurrentStatePage:
      return ax::mojom::AriaCurrentState::kPage;
    case blink::kWebAXAriaCurrentStateStep:
      return ax::mojom::AriaCurrentState::kStep;
    case blink::kWebAXAriaCurrentStateLocation:
      return ax::mojom::AriaCurrentState::kLocation;
    case blink::kWebAXAriaCurrentStateDate:
      return ax::mojom::AriaCurrentState::kDate;
    case blink::kWebAXAriaCurrentStateTime:
      return ax::mojom::AriaCurrentState::kTime;
  }

  NOTREACHED();
  return ax::mojom::AriaCurrentState::kNone;
}

ax::mojom::HasPopup AXHasPopupFromBlink(blink::WebAXHasPopup has_popup) {
  switch (has_popup) {
    case blink::kWebAXHasPopupFalse:
      return ax::mojom::HasPopup::kFalse;
    case blink::kWebAXHasPopupTrue:
      return ax::mojom::HasPopup::kTrue;
    case blink::kWebAXHasPopupMenu:
      return ax::mojom::HasPopup::kMenu;
    case blink::kWebAXHasPopupListbox:
      return ax::mojom::HasPopup::kListbox;
    case blink::kWebAXHasPopupTree:
      return ax::mojom::HasPopup::kTree;
    case blink::kWebAXHasPopupGrid:
      return ax::mojom::HasPopup::kGrid;
    case blink::kWebAXHasPopupDialog:
      return ax::mojom::HasPopup::kDialog;
  }

  NOTREACHED();
  return ax::mojom::HasPopup::kFalse;
}

ax::mojom::InvalidState AXInvalidStateFromBlink(
    blink::WebAXInvalidState invalid_state) {
  switch (invalid_state) {
    case blink::kWebAXInvalidStateUndefined:
      return ax::mojom::InvalidState::kNone;
    case blink::kWebAXInvalidStateFalse:
      return ax::mojom::InvalidState::kFalse;
    case blink::kWebAXInvalidStateTrue:
      return ax::mojom::InvalidState::kTrue;
    case blink::kWebAXInvalidStateSpelling:
      return ax::mojom::InvalidState::kSpelling;
    case blink::kWebAXInvalidStateGrammar:
      return ax::mojom::InvalidState::kGrammar;
    case blink::kWebAXInvalidStateOther:
      return ax::mojom::InvalidState::kOther;
  }
  NOTREACHED();
  return ax::mojom::InvalidState::kNone;
}

ax::mojom::CheckedState AXCheckedStateFromBlink(
    blink::WebAXCheckedState checked_state) {
  switch (checked_state) {
    case blink::kWebAXCheckedUndefined:
      return ax::mojom::CheckedState::kNone;
    case blink::kWebAXCheckedTrue:
      return ax::mojom::CheckedState::kTrue;
    case blink::kWebAXCheckedMixed:
      return ax::mojom::CheckedState::kMixed;
    case blink::kWebAXCheckedFalse:
      return ax::mojom::CheckedState::kFalse;
  }
  NOTREACHED();
  return ax::mojom::CheckedState::kNone;
}

ax::mojom::SortDirection AXSortDirectionFromBlink(
    blink::WebAXSortDirection sort_direction) {
  switch (sort_direction) {
    case blink::kWebAXSortDirectionUndefined:
      return ax::mojom::SortDirection::kNone;
    case blink::kWebAXSortDirectionNone:
      return ax::mojom::SortDirection::kUnsorted;
    case blink::kWebAXSortDirectionAscending:
      return ax::mojom::SortDirection::kAscending;
    case blink::kWebAXSortDirectionDescending:
      return ax::mojom::SortDirection::kDescending;
    case blink::kWebAXSortDirectionOther:
      return ax::mojom::SortDirection::kOther;
  }
  NOTREACHED();
  return ax::mojom::SortDirection::kNone;
}

ax::mojom::NameFrom AXNameFromFromBlink(blink::WebAXNameFrom name_from) {
  switch (name_from) {
    case blink::kWebAXNameFromUninitialized:
      return ax::mojom::NameFrom::kUninitialized;
    case blink::kWebAXNameFromAttribute:
      return ax::mojom::NameFrom::kAttribute;
    case blink::kWebAXNameFromAttributeExplicitlyEmpty:
      return ax::mojom::NameFrom::kAttributeExplicitlyEmpty;
    case blink::kWebAXNameFromCaption:
      return ax::mojom::NameFrom::kRelatedElement;
    case blink::kWebAXNameFromContents:
      return ax::mojom::NameFrom::kContents;
    case blink::kWebAXNameFromPlaceholder:
      return ax::mojom::NameFrom::kPlaceholder;
    case blink::kWebAXNameFromRelatedElement:
      return ax::mojom::NameFrom::kRelatedElement;
    case blink::kWebAXNameFromValue:
      return ax::mojom::NameFrom::kValue;
    case blink::kWebAXNameFromTitle:
      return ax::mojom::NameFrom::kAttribute;
  }
  NOTREACHED();
  return ax::mojom::NameFrom::kNone;
}

ax::mojom::DescriptionFrom AXDescriptionFromFromBlink(
    blink::WebAXDescriptionFrom description_from) {
  switch (description_from) {
    case blink::kWebAXDescriptionFromUninitialized:
      return ax::mojom::DescriptionFrom::kUninitialized;
    case blink::kWebAXDescriptionFromAttribute:
      return ax::mojom::DescriptionFrom::kAttribute;
    case blink::kWebAXDescriptionFromContents:
      return ax::mojom::DescriptionFrom::kContents;
    case blink::kWebAXDescriptionFromRelatedElement:
      return ax::mojom::DescriptionFrom::kRelatedElement;
  }
  NOTREACHED();
  return ax::mojom::DescriptionFrom::kNone;
}

ax::mojom::TextAffinity AXTextAffinityFromBlink(
    blink::WebAXTextAffinity affinity) {
  switch (affinity) {
    case blink::kWebAXTextAffinityUpstream:
      return ax::mojom::TextAffinity::kUpstream;
    case blink::kWebAXTextAffinityDownstream:
      return ax::mojom::TextAffinity::kDownstream;
  }
  NOTREACHED();
  return ax::mojom::TextAffinity::kDownstream;
}

}  // namespace content.
