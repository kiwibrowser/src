// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_text_track_list_element.h"

#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/forms/html_label_element.h"
#include "third_party/blink/renderer/core/html/html_span_element.h"
#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/core/html/track/text_track.h"
#include "third_party/blink/renderer/core/html/track/text_track_list.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/modules/media_controls/elements/media_control_toggle_closed_captions_button_element.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"
#include "third_party/blink/renderer/platform/event_dispatch_forbidden_scope.h"
#include "third_party/blink/renderer/platform/text/platform_locale.h"

namespace blink {

namespace {

// When specified as trackIndex, disable text tracks.
constexpr int kTrackIndexOffValue = -1;

const QualifiedName& TrackIndexAttrName() {
  // Save the track index in an attribute to avoid holding a pointer to the text
  // track.
  DEFINE_STATIC_LOCAL(QualifiedName, track_index_attr,
                      (g_null_atom, "data-track-index", g_null_atom));
  return track_index_attr;
}

bool HasDuplicateLabel(TextTrack* current_track) {
  DCHECK(current_track);
  TextTrackList* track_list = current_track->TrackList();
  // The runtime of this method is quadratic but since there are usually very
  // few text tracks it won't affect the performance much.
  String current_track_label = current_track->label();
  for (unsigned i = 0; i < track_list->length(); i++) {
    TextTrack* track = track_list->AnonymousIndexedGetter(i);
    if (current_track != track && current_track_label == track->label())
      return true;
  }
  return false;
}

}  // anonymous namespace

MediaControlTextTrackListElement::MediaControlTextTrackListElement(
    MediaControlsImpl& media_controls)
    : MediaControlPopupMenuElement(media_controls, kMediaTextTrackList) {
  SetShadowPseudoId(AtomicString("-internal-media-controls-text-track-list"));
}

bool MediaControlTextTrackListElement::WillRespondToMouseClickEvents() {
  return true;
}

void MediaControlTextTrackListElement::SetIsWanted(bool wanted) {
  if (wanted)
    RefreshTextTrackListMenu();

  MediaControlPopupMenuElement::SetIsWanted(wanted);
}

Element* MediaControlTextTrackListElement::PopupAnchor() const {
  return &GetMediaControls().ToggleClosedCaptions();
}

void MediaControlTextTrackListElement::DefaultEventHandler(Event* event) {
  if (event->type() == EventTypeNames::click) {
    // This handles the back button click. Clicking on a menu item triggers the
    // change event instead.
    GetMediaControls().ToggleOverflowMenu();
    event->SetDefaultHandled();
  } else if (event->type() == EventTypeNames::change) {
    // Identify which input element was selected and set track to showing
    Node* target = event->target()->ToNode();
    if (!target || !target->IsElementNode())
      return;

    GetMediaControls().DisableShowingTextTracks();
    int track_index =
        ToElement(target)->GetIntegralAttribute(TrackIndexAttrName());
    if (track_index != kTrackIndexOffValue) {
      DCHECK_GE(track_index, 0);
      GetMediaControls().ShowTextTrackAtIndex(track_index);
      MediaElement().DisableAutomaticTextTrackSelection();
    }

    event->SetDefaultHandled();
  }
  MediaControlPopupMenuElement::DefaultEventHandler(event);
}

// TextTrack parameter when passed in as a nullptr, creates the "Off" list item
// in the track list.
Element* MediaControlTextTrackListElement::CreateTextTrackListItem(
    TextTrack* track) {
  int track_index = track ? track->TrackIndex() : kTrackIndexOffValue;
  HTMLLabelElement* track_item = HTMLLabelElement::Create(GetDocument());
  track_item->SetShadowPseudoId(
      AtomicString("-internal-media-controls-text-track-list-item"));
  auto* track_item_input =
      HTMLInputElement::Create(GetDocument(), CreateElementFlags());
  track_item_input->SetShadowPseudoId(
      AtomicString("-internal-media-controls-text-track-list-item-input"));
  track_item_input->setType(InputTypeNames::checkbox);
  track_item_input->SetIntegralAttribute(TrackIndexAttrName(), track_index);
  if (!MediaElement().TextTracksVisible()) {
    if (!track)
      track_item_input->setChecked(true);
  } else {
    // If there are multiple text tracks set to showing, they must all have
    // checkmarks displayed.
    if (track && track->mode() == TextTrack::ShowingKeyword())
      track_item_input->setChecked(true);
  }

  // Allows to focus the list entry instead of the button.
  track_item->setTabIndex(0);
  track_item_input->setTabIndex(-1);

  // Modern media controls should have the checkbox after the text instead of
  // the other way around.
  if (!MediaControlsImpl::IsModern())
    track_item->ParserAppendChild(track_item_input);
  String track_label = GetMediaControls().GetTextTrackLabel(track);
  track_item->ParserAppendChild(Text::Create(GetDocument(), track_label));
  if (MediaControlsImpl::IsModern())
    track_item->ParserAppendChild(track_item_input);

  // Add a track kind marker icon if there are multiple tracks with the same
  // label or if the track has no label.
  if (track && (track->label().IsEmpty() || HasDuplicateLabel(track))) {
    HTMLSpanElement* track_kind_marker = HTMLSpanElement::Create(GetDocument());
    if (track->kind() == track->CaptionsKeyword()) {
      track_kind_marker->SetShadowPseudoId(AtomicString(
          "-internal-media-controls-text-track-list-kind-captions"));
    } else {
      DCHECK_EQ(track->kind(), track->SubtitlesKeyword());
      track_kind_marker->SetShadowPseudoId(AtomicString(
          "-internal-media-controls-text-track-list-kind-subtitles"));
    }
    track_item->ParserAppendChild(track_kind_marker);
  }
  return track_item;
}

Element* MediaControlTextTrackListElement::CreateTextTrackHeaderItem() {
  HTMLLabelElement* header_item = HTMLLabelElement::Create(GetDocument());
  header_item->SetShadowPseudoId(
      "-internal-media-controls-text-track-list-header");
  header_item->ParserAppendChild(
      Text::Create(GetDocument(),
                   GetLocale().QueryString(
                       WebLocalizedString::kOverflowMenuCaptionsSubmenuTitle)));
  header_item->setTabIndex(0);
  return header_item;
}

void MediaControlTextTrackListElement::RefreshTextTrackListMenu() {
  if (!MediaElement().HasClosedCaptions() ||
      !MediaElement().TextTracksAreReady()) {
    return;
  }

  EventDispatchForbiddenScope::AllowUserAgentEvents allow_events;
  RemoveChildren(kOmitSubtreeModifiedEvent);

  if (MediaControlsImpl::IsModern())
    ParserAppendChild(CreateTextTrackHeaderItem());

  // Construct a menu for subtitles and captions.  Pass in a nullptr to
  // createTextTrackListItem to create the "Off" track item.
  ParserAppendChild(CreateTextTrackListItem(nullptr));

  TextTrackList* track_list = MediaElement().textTracks();
  for (unsigned i = 0; i < track_list->length(); i++) {
    TextTrack* track = track_list->AnonymousIndexedGetter(i);
    if (!track->CanBeRendered())
      continue;
    ParserAppendChild(CreateTextTrackListItem(track));
  }
}

}  // namespace blink
