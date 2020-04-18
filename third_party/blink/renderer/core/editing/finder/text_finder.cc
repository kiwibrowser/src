/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/editing/finder/text_finder.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_float_rect.h"
#include "third_party/blink/public/platform/web_scroll_into_view_params.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/public/web/web_find_options.h"
#include "third_party/blink/public/web/web_frame_client.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/dom/ax_object_cache_base.h"
#include "third_party/blink/renderer/core/dom/range.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/finder/find_in_page_coordinates.h"
#include "third_party/blink/renderer/core/editing/finder/find_options.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/iterators/search_buffer.h"
#include "third_party/blink/renderer/core/editing/markers/document_marker.h"
#include "third_party/blink/renderer/core/editing/markers/document_marker_controller.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/text_autosizer.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

TextFinder::FindMatch::FindMatch(Range* range, int ordinal)
    : range_(range), ordinal_(ordinal) {}

void TextFinder::FindMatch::Trace(blink::Visitor* visitor) {
  visitor->Trace(range_);
}

class TextFinder::DeferredScopeStringMatches
    : public GarbageCollectedFinalized<TextFinder::DeferredScopeStringMatches> {
 public:
  static DeferredScopeStringMatches* Create(TextFinder* text_finder,
                                            int identifier,
                                            const WebString& search_text,
                                            const WebFindOptions& options) {
    return new DeferredScopeStringMatches(text_finder, identifier, search_text,
                                          options);
  }

  void Trace(blink::Visitor* visitor) { visitor->Trace(text_finder_); }

  void Dispose() { timer_.Stop(); }

 private:
  DeferredScopeStringMatches(TextFinder* text_finder,
                             int identifier,
                             const WebString& search_text,
                             const WebFindOptions& options)
      : timer_(text_finder->OwnerFrame().GetFrame()->GetTaskRunner(
                   TaskType::kInternalDefault),
               this,
               &DeferredScopeStringMatches::DoTimeout),
        text_finder_(text_finder),
        identifier_(identifier),
        search_text_(search_text),
        options_(options) {
    timer_.StartOneShot(TimeDelta(), FROM_HERE);
  }

  void DoTimeout(TimerBase*) {
    text_finder_->ResumeScopingStringMatches(identifier_, search_text_,
                                             options_);
  }

  TaskRunnerTimer<DeferredScopeStringMatches> timer_;
  Member<TextFinder> text_finder_;
  const int identifier_;
  const WebString search_text_;
  const WebFindOptions options_;
};

static void ScrollToVisible(Range* match) {
  const Node& first_node = *match->FirstNode();
  Settings* settings = first_node.GetDocument().GetSettings();
  bool smooth_find_enabled =
      settings ? settings->GetSmoothScrollForFindEnabled() : false;
  ScrollBehavior scroll_behavior =
      smooth_find_enabled ? kScrollBehaviorSmooth : kScrollBehaviorAuto;
  first_node.GetLayoutObject()->ScrollRectToVisible(
      LayoutRect(match->BoundingBox()),
      WebScrollIntoViewParams(ScrollAlignment::kAlignCenterIfNeeded,
                              ScrollAlignment::kAlignCenterIfNeeded,
                              kUserScroll, false, scroll_behavior, true));
  first_node.GetDocument().SetSequentialFocusNavigationStartingPoint(
      const_cast<Node*>(&first_node));
}

bool TextFinder::Find(int identifier,
                      const WebString& search_text,
                      const WebFindOptions& options,
                      bool wrap_within_frame,
                      bool* active_now) {
  if (!options.find_next)
    UnmarkAllTextMatches();
  else
    SetMarkerActive(active_match_.Get(), false);

  if (active_match_ &&
      &active_match_->OwnerDocument() != OwnerFrame().GetFrame()->GetDocument())
    active_match_ = nullptr;

  // If the user has selected something since the last Find operation we want
  // to start from there. Otherwise, we start searching from where the last Find
  // operation left off (either a Find or a FindNext operation).
  // TODO(editing-dev): The use of VisibleSelection should be audited. See
  // crbug.com/657237 for details.
  VisibleSelection selection(
      OwnerFrame().GetFrame()->Selection().ComputeVisibleSelectionInDOMTree());
  bool active_selection = !selection.IsNone();
  if (active_selection) {
    active_match_ = CreateRange(FirstEphemeralRangeOf(selection));
    OwnerFrame().GetFrame()->Selection().Clear();
  }

  DCHECK(OwnerFrame().GetFrame());
  DCHECK(OwnerFrame().GetFrame()->View());
  const FindOptions find_options =
      (options.forward ? 0 : kBackwards) |
      (options.match_case ? 0 : kCaseInsensitive) |
      (wrap_within_frame ? kWrapAround : 0) |
      (options.word_start ? kAtWordStarts : 0) |
      (options.medial_capital_as_word_start ? kTreatMedialCapitalAsWordStart
                                            : 0) |
      (options.find_next ? 0 : kStartInSelection);
  active_match_ = Editor::FindRangeOfString(
      *OwnerFrame().GetFrame()->GetDocument(), search_text,
      EphemeralRangeInFlatTree(active_match_.Get()), find_options);

  if (!active_match_) {
    // If we're finding next the next active match might not be in the current
    // frame.  In this case we don't want to clear the matches cache.
    if (!options.find_next)
      ClearFindMatchesCache();

    OwnerFrame().GetFrameView()->InvalidatePaintForTickmarks();
    return false;
  }
  ScrollToVisible(active_match_);

  // If the user is browsing a page with autosizing, adjust the zoom to the
  // column where the next hit has been found. Doing this when autosizing is
  // not set will result in a zoom reset on small devices.
  if (OwnerFrame()
          .GetFrame()
          ->GetDocument()
          ->GetTextAutosizer()
          ->PageNeedsAutosizing()) {
    OwnerFrame().ViewImpl()->ZoomToFindInPageRect(
        OwnerFrame().GetFrameView()->AbsoluteToRootFrame(
            EnclosingIntRect(LayoutObject::AbsoluteBoundingBoxRectForRange(
                EphemeralRange(active_match_.Get())))));
  }

  bool was_active_frame = current_active_match_frame_;
  current_active_match_frame_ = true;

  bool is_active = SetMarkerActive(active_match_.Get(), true);
  if (active_now)
    *active_now = is_active;

  // Make sure no node is focused. See http://crbug.com/38700.
  OwnerFrame().GetFrame()->GetDocument()->ClearFocusedElement();

  // Set this frame as focused.
  OwnerFrame().ViewImpl()->SetFocusedFrame(&OwnerFrame());

  if (!options.find_next || active_selection || !is_active) {
    // This is either an initial Find operation, a Find-next from a new
    // start point due to a selection, or new matches were found during
    // Find-next due to DOM alteration (that couldn't be set as active), so
    // we set the flag to ask the scoping effort to find the active rect for
    // us and report it back to the UI.
    locating_active_rect_ = true;
  } else {
    if (!was_active_frame) {
      if (options.forward)
        active_match_index_ = 0;
      else
        active_match_index_ = last_match_count_ - 1;
    } else {
      if (options.forward)
        ++active_match_index_;
      else
        --active_match_index_;

      if (active_match_index_ + 1 > last_match_count_)
        active_match_index_ = 0;
      else if (active_match_index_ < 0)
        active_match_index_ = last_match_count_ - 1;
    }
    WebRect selection_rect = OwnerFrame().GetFrameView()->AbsoluteToRootFrame(
        active_match_->BoundingBox());
    ReportFindInPageSelection(selection_rect, active_match_index_ + 1,
                              identifier);
  }

  // We found something, so the result of the previous scoping may be outdated.
  last_find_request_completed_with_no_matches_ = false;

  return true;
}

void TextFinder::ClearActiveFindMatch() {
  current_active_match_frame_ = false;
  SetMarkerActive(active_match_.Get(), false);
  ResetActiveMatch();
}

LocalFrame* TextFinder::GetFrame() const {
  return OwnerFrame().GetFrame();
}

void TextFinder::SetFindEndstateFocusAndSelection() {
  if (!ActiveMatchFrame())
    return;

  Range* active_match = ActiveMatch();
  if (!active_match)
    return;

  // If the user has set the selection since the match was found, we
  // don't focus anything.
  if (!GetFrame()->Selection().GetSelectionInDOMTree().IsNone())
    return;

  // Need to clean out style and layout state before querying
  // Element::isFocusable().
  GetFrame()->GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();

  // Try to find the first focusable node up the chain, which will, for
  // example, focus links if we have found text within the link.
  Node* node = active_match->FirstNode();
  if (node && node->IsInShadowTree()) {
    if (Node* host = node->OwnerShadowHost()) {
      if (IsHTMLInputElement(*host) || IsHTMLTextAreaElement(*host))
        node = host;
    }
  }
  const EphemeralRange active_match_range(active_match);
  if (node) {
    for (Node& runner : NodeTraversal::InclusiveAncestorsOf(*node)) {
      if (!runner.IsElementNode())
        continue;
      Element& element = ToElement(runner);
      if (element.IsFocusable()) {
        // Found a focusable parent node. Set the active match as the
        // selection and focus to the focusable node.
        GetFrame()->Selection().SetSelectionAndEndTyping(
            SelectionInDOMTree::Builder()
                .SetBaseAndExtent(active_match_range)
                .Build());
        GetFrame()->GetDocument()->SetFocusedElement(
            &element, FocusParams(SelectionBehaviorOnFocus::kNone,
                                  kWebFocusTypeNone, nullptr));
        return;
      }
    }
  }

  // Iterate over all the nodes in the range until we find a focusable node.
  // This, for example, sets focus to the first link if you search for
  // text and text that is within one or more links.
  for (Node& runner : active_match_range.Nodes()) {
    if (!runner.IsElementNode())
      continue;
    Element& element = ToElement(runner);
    if (element.IsFocusable()) {
      GetFrame()->GetDocument()->SetFocusedElement(
          &element, FocusParams(SelectionBehaviorOnFocus::kNone,
                                kWebFocusTypeNone, nullptr));
      return;
    }
  }

  // No node related to the active match was focusable, so set the
  // active match as the selection (so that when you end the Find session,
  // you'll have the last thing you found highlighted) and make sure that
  // we have nothing focused (otherwise you might have text selected but
  // a link focused, which is weird).
  GetFrame()->Selection().SetSelectionAndEndTyping(
      SelectionInDOMTree::Builder()
          .SetBaseAndExtent(active_match_range)
          .Build());
  GetFrame()->GetDocument()->ClearFocusedElement();

  // Finally clear the active match, for two reasons:
  // We just finished the find 'session' and we don't want future (potentially
  // unrelated) find 'sessions' operations to start at the same place.
  // The WebLocalFrameImpl could get reused and the activeMatch could end up
  // pointing to a document that is no longer valid. Keeping an invalid
  // reference around is just asking for trouble.
  ResetActiveMatch();
}

void TextFinder::StopFindingAndClearSelection() {
  CancelPendingScopingEffort();

  // Remove all markers for matches found and turn off the highlighting.
  OwnerFrame().GetFrame()->GetDocument()->Markers().RemoveMarkersOfTypes(
      DocumentMarker::kTextMatch);
  OwnerFrame().GetFrame()->GetEditor().SetMarkedTextMatchesAreHighlighted(
      false);
  ClearFindMatchesCache();
  ResetActiveMatch();

  // Let the frame know that we don't want tickmarks anymore.
  OwnerFrame().GetFrameView()->InvalidatePaintForTickmarks();
}

void TextFinder::ReportFindInPageResultToAccessibility(int identifier) {
  if (!active_match_)
    return;

  AXObjectCacheBase* ax_object_cache = ToAXObjectCacheBase(
      OwnerFrame().GetFrame()->GetDocument()->ExistingAXObjectCache());
  if (!ax_object_cache)
    return;

  Node* start_node = active_match_->startContainer();
  Node* end_node = active_match_->endContainer();
  ax_object_cache->HandleTextMarkerDataAdded(start_node, end_node);

  if (OwnerFrame().Client()) {
    OwnerFrame().Client()->HandleAccessibilityFindInPageResult(
        identifier, active_match_index_ + 1, blink::WebNode(start_node),
        active_match_->startOffset(), blink::WebNode(end_node),
        active_match_->endOffset());
  }
}

void TextFinder::StartScopingStringMatches(int identifier,
                                           const WebString& search_text,
                                           const WebFindOptions& options) {
  CancelPendingScopingEffort();

  // This is a brand new search, so we need to reset everything.
  // Scoping is just about to begin.
  scoping_in_progress_ = true;

  // Need to keep the current identifier locally in order to finish the
  // request in case the frame is detached during the process.
  find_request_identifier_ = identifier;

  // Clear highlighting for this frame.
  UnmarkAllTextMatches();

  // Clear the tickmarks and results cache.
  ClearFindMatchesCache();

  // Clear the total match count and increment markers version.
  ResetMatchCount();

  // Clear the counters from last operation.
  last_match_count_ = 0;
  next_invalidate_after_ = 0;

  // The view might be null on detached frames.
  LocalFrame* frame = OwnerFrame().GetFrame();
  if (frame && frame->GetPage())
    frame_scoping_ = true;

  // Now, defer scoping until later to allow find operation to finish quickly.
  ScopeStringMatchesSoon(identifier, search_text, options);
}

void TextFinder::ScopeStringMatches(int identifier,
                                    const WebString& search_text,
                                    const WebFindOptions& options) {
  if (!ShouldScopeMatches(search_text, options)) {
    FinishCurrentScopingEffort(identifier);
    return;
  }

  PositionInFlatTree search_start = PositionInFlatTree::FirstPositionInNode(
      *OwnerFrame().GetFrame()->GetDocument());
  PositionInFlatTree search_end = PositionInFlatTree::LastPositionInNode(
      *OwnerFrame().GetFrame()->GetDocument());
  DCHECK_EQ(search_start.GetDocument(), search_end.GetDocument());

  if (resume_scoping_from_range_) {
    // This is a continuation of a scoping operation that timed out and didn't
    // complete last time around, so we should start from where we left off.
    DCHECK(resume_scoping_from_range_->collapsed());
    search_start = FromPositionInDOMTree<EditingInFlatTreeStrategy>(
        resume_scoping_from_range_->EndPosition());
    if (search_start.GetDocument() != search_end.GetDocument())
      return;
  }

  // TODO(editing-dev): Use of updateStyleAndLayoutIgnorePendingStylesheets
  // needs to be audited.  see http://crbug.com/590369 for more details.
  search_start.GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();

  // This timeout controls how long we scope before releasing control. This
  // value does not prevent us from running for longer than this, but it is
  // periodically checked to see if we have exceeded our allocated time.
  const double kMaxScopingDuration = 0.1;  // seconds

  int match_count = 0;
  bool timed_out = false;
  double start_time = CurrentTime();
  PositionInFlatTree next_scoping_start;
  do {
    // Find next occurrence of the search string.
    // FIXME: (http://crbug.com/6818) This WebKit operation may run for longer
    // than the timeout value, and is not interruptible as it is currently
    // written. We may need to rewrite it with interruptibility in mind, or
    // find an alternative.
    const EphemeralRangeInFlatTree result =
        FindPlainText(EphemeralRangeInFlatTree(search_start, search_end),
                      search_text, options.match_case ? 0 : kCaseInsensitive);
    if (result.IsCollapsed()) {
      // Not found.
      break;
    }
    Range* result_range = Range::Create(
        result.GetDocument(), ToPositionInDOMTree(result.StartPosition()),
        ToPositionInDOMTree(result.EndPosition()));
    if (result_range->collapsed()) {
      // resultRange will be collapsed if the matched text spans over multiple
      // TreeScopes.  FIXME: Show such matches to users.
      search_start = result.EndPosition();
      continue;
    }

    ++match_count;

    // Catch a special case where Find found something but doesn't know what
    // the bounding box for it is. In this case we set the first match we find
    // as the active rect.
    IntRect result_bounds = result_range->BoundingBox();
    IntRect active_selection_rect;
    if (locating_active_rect_) {
      active_selection_rect =
          active_match_.Get() ? active_match_->BoundingBox() : result_bounds;
    }

    // If the Find function found a match it will have stored where the
    // match was found in m_activeSelectionRect on the current frame. If we
    // find this rect during scoping it means we have found the active
    // tickmark.
    bool found_active_match = false;
    if (locating_active_rect_ && (active_selection_rect == result_bounds)) {
      // We have found the active tickmark frame.
      current_active_match_frame_ = true;
      found_active_match = true;
      // We also know which tickmark is active now.
      active_match_index_ = total_match_count_ + match_count - 1;
      // To stop looking for the active tickmark, we set this flag.
      locating_active_rect_ = false;

      // Notify browser of new location for the selected rectangle.
      ReportFindInPageSelection(
          OwnerFrame().GetFrameView()->AbsoluteToRootFrame(result_bounds),
          active_match_index_ + 1, identifier);
    }

    OwnerFrame().GetFrame()->GetDocument()->Markers().AddTextMatchMarker(
        EphemeralRange(result_range),
        found_active_match ? TextMatchMarker::MatchStatus::kActive
                           : TextMatchMarker::MatchStatus::kInactive);

    find_matches_cache_.push_back(
        FindMatch(result_range, last_match_count_ + match_count));

    // Set the new start for the search range to be the end of the previous
    // result range. There is no need to use a VisiblePosition here,
    // since findPlainText will use a TextIterator to go over the visible
    // text nodes.
    search_start = result.EndPosition();

    next_scoping_start = search_start;
    timed_out = (CurrentTime() - start_time) >= kMaxScopingDuration;
  } while (!timed_out);

  if (next_scoping_start.IsNotNull()) {
    resume_scoping_from_range_ =
        Range::Create(*next_scoping_start.GetDocument(),
                      ToPositionInDOMTree(next_scoping_start),
                      ToPositionInDOMTree(next_scoping_start));
  }

  // Remember what we search for last time, so we can skip searching if more
  // letters are added to the search string (and last outcome was 0).
  last_search_string_ = search_text;

  if (match_count > 0) {
    OwnerFrame().GetFrame()->GetEditor().SetMarkedTextMatchesAreHighlighted(
        true);

    last_match_count_ += match_count;

    // Let the frame know how many matches we found during this pass.
    IncreaseMatchCount(identifier, match_count);
  }

  if (timed_out) {
    // If we found anything during this pass, we should redraw. However, we
    // don't want to spam too much if the page is extremely long, so if we
    // reach a certain point we start throttling the redraw requests.
    if (match_count > 0)
      InvalidateIfNecessary();

    // Scoping effort ran out of time, lets ask for another time-slice.
    ScopeStringMatchesSoon(identifier, search_text, options);
    return;  // Done for now, resume work later.
  }

  FinishCurrentScopingEffort(identifier);
}

void TextFinder::FlushCurrentScopingEffort(int identifier) {
  if (!OwnerFrame().GetFrame() || !OwnerFrame().GetFrame()->GetPage())
    return;

  frame_scoping_ = false;
  IncreaseMatchCount(identifier, 0);
}

void TextFinder::FinishCurrentScopingEffort(int identifier) {
  if (!total_match_count_)
    OwnerFrame().GetFrame()->Selection().Clear();

  FlushCurrentScopingEffort(identifier);

  scoping_in_progress_ = false;
  last_find_request_completed_with_no_matches_ = !last_match_count_;

  // This frame is done, so show any scrollbar tickmarks we haven't drawn yet.
  OwnerFrame().GetFrameView()->InvalidatePaintForTickmarks();
}

void TextFinder::CancelPendingScopingEffort() {
  if (deferred_scoping_work_) {
    deferred_scoping_work_->Dispose();
    deferred_scoping_work_.Clear();
  }

  active_match_index_ = -1;

  // Last request didn't complete.
  if (scoping_in_progress_)
    last_find_request_completed_with_no_matches_ = false;

  scoping_in_progress_ = false;

  resume_scoping_from_range_ = nullptr;
}

void TextFinder::IncreaseMatchCount(int identifier, int count) {
  if (count)
    ++find_match_markers_version_;

  total_match_count_ += count;

  // Update the UI with the latest findings.
  if (OwnerFrame().Client()) {
    OwnerFrame().Client()->ReportFindInPageMatchCount(
        identifier, total_match_count_, !frame_scoping_ || !total_match_count_);
  }
}

void TextFinder::ReportFindInPageSelection(const WebRect& selection_rect,
                                           int active_match_ordinal,
                                           int identifier) {
  // Update the UI with the latest selection rect.
  if (OwnerFrame().Client()) {
    OwnerFrame().Client()->ReportFindInPageSelection(
        identifier, active_match_ordinal, selection_rect);
  }
  // Update accessibility too, so if the user commits to this query
  // we can move accessibility focus to this result.
  ReportFindInPageResultToAccessibility(identifier);
}

void TextFinder::ResetMatchCount() {
  if (total_match_count_ > 0)
    ++find_match_markers_version_;

  total_match_count_ = 0;
  frame_scoping_ = false;
}

void TextFinder::ClearFindMatchesCache() {
  if (!find_matches_cache_.IsEmpty())
    ++find_match_markers_version_;

  find_matches_cache_.clear();
  find_match_rects_are_valid_ = false;
}

void TextFinder::UpdateFindMatchRects() {
  IntSize current_document_size = OwnerFrame().DocumentSize();
  if (document_size_for_current_find_match_rects_ != current_document_size) {
    document_size_for_current_find_match_rects_ = current_document_size;
    find_match_rects_are_valid_ = false;
  }

  size_t dead_matches = 0;
  for (FindMatch& match : find_matches_cache_) {
    if (!match.range_->BoundaryPointsValid() ||
        !match.range_->startContainer()->isConnected())
      match.rect_ = FloatRect();
    else if (!find_match_rects_are_valid_)
      match.rect_ = FindInPageRectFromRange(EphemeralRange(match.range_.Get()));

    if (match.rect_.IsEmpty())
      ++dead_matches;
  }

  // Remove any invalid matches from the cache.
  if (dead_matches) {
    HeapVector<FindMatch> filtered_matches;
    filtered_matches.ReserveCapacity(find_matches_cache_.size() - dead_matches);

    for (const FindMatch& match : find_matches_cache_) {
      if (!match.rect_.IsEmpty())
        filtered_matches.push_back(match);
    }

    find_matches_cache_.swap(filtered_matches);
  }

  find_match_rects_are_valid_ = true;
}

WebFloatRect TextFinder::ActiveFindMatchRect() {
  if (!current_active_match_frame_ || !active_match_)
    return WebFloatRect();

  return WebFloatRect(FindInPageRectFromRange(EphemeralRange(ActiveMatch())));
}

Vector<WebFloatRect> TextFinder::FindMatchRects() {
  UpdateFindMatchRects();

  Vector<WebFloatRect> match_rects;
  match_rects.ReserveCapacity(match_rects.size() + find_matches_cache_.size());
  for (const FindMatch& match : find_matches_cache_) {
    DCHECK(!match.rect_.IsEmpty());
    match_rects.push_back(match.rect_);
  }

  return match_rects;
}

int TextFinder::SelectNearestFindMatch(const WebFloatPoint& point,
                                       WebRect* selection_rect) {
  int index = NearestFindMatch(point, nullptr);
  if (index != -1)
    return SelectFindMatch(static_cast<unsigned>(index), selection_rect);

  return -1;
}

int TextFinder::NearestFindMatch(const FloatPoint& point,
                                 float* distance_squared) {
  UpdateFindMatchRects();

  int nearest = -1;
  float nearest_distance_squared = FLT_MAX;
  for (size_t i = 0; i < find_matches_cache_.size(); ++i) {
    DCHECK(!find_matches_cache_[i].rect_.IsEmpty());
    FloatSize offset = point - find_matches_cache_[i].rect_.Center();
    float width = offset.Width();
    float height = offset.Height();
    float current_distance_squared = width * width + height * height;
    if (current_distance_squared < nearest_distance_squared) {
      nearest = i;
      nearest_distance_squared = current_distance_squared;
    }
  }

  if (distance_squared)
    *distance_squared = nearest_distance_squared;

  return nearest;
}

int TextFinder::SelectFindMatch(unsigned index, WebRect* selection_rect) {
  SECURITY_DCHECK(index < find_matches_cache_.size());

  Range* range = find_matches_cache_[index].range_;
  if (!range->BoundaryPointsValid() || !range->startContainer()->isConnected())
    return -1;

  // Check if the match is already selected.
  if (!current_active_match_frame_ || !active_match_ ||
      !AreRangesEqual(active_match_.Get(), range)) {
    active_match_index_ = find_matches_cache_[index].ordinal_ - 1;

    // Set this frame as the active frame (the one with the active highlight).
    current_active_match_frame_ = true;
    OwnerFrame().ViewImpl()->SetFocusedFrame(&OwnerFrame());

    if (active_match_)
      SetMarkerActive(active_match_.Get(), false);
    active_match_ = range;
    SetMarkerActive(active_match_.Get(), true);

    // Clear any user selection, to make sure Find Next continues on from the
    // match we just activated.
    OwnerFrame().GetFrame()->Selection().Clear();

    // Make sure no node is focused. See http://crbug.com/38700.
    OwnerFrame().GetFrame()->GetDocument()->ClearFocusedElement();
  }

  IntRect active_match_rect;
  IntRect active_match_bounding_box =
      EnclosingIntRect(LayoutObject::AbsoluteBoundingBoxRectForRange(
          EphemeralRange(active_match_.Get())));

  if (!active_match_bounding_box.IsEmpty()) {
    if (active_match_->FirstNode() &&
        active_match_->FirstNode()->GetLayoutObject()) {
      active_match_->FirstNode()->GetLayoutObject()->ScrollRectToVisible(
          LayoutRect(active_match_bounding_box),
          WebScrollIntoViewParams(ScrollAlignment::kAlignCenterIfNeeded,
                                  ScrollAlignment::kAlignCenterIfNeeded,
                                  kUserScroll));

      // Absolute coordinates are scroll-variant so the bounding box will change
      // if the page is scrolled by ScrollRectToVisible above. Recompute the
      // bounding box so we have the updated location for the zoom below.
      // TODO(bokan): This should really use the return value from
      // ScrollRectToVisible which returns the updated position of the
      // scrolled rect. However, this was recently added and this is a fix
      // that needs to be merged to a release branch.
      // https://crbug.com/823365.
      active_match_bounding_box =
          EnclosingIntRect(LayoutObject::AbsoluteBoundingBoxRectForRange(
              EphemeralRange(active_match_.Get())));
    }

    // Zoom to the active match.
    active_match_rect = OwnerFrame().GetFrameView()->AbsoluteToRootFrame(
        active_match_bounding_box);
    OwnerFrame().ViewImpl()->ZoomToFindInPageRect(active_match_rect);
  }

  if (selection_rect)
    *selection_rect = active_match_rect;

  return active_match_index_ + 1;
}

TextFinder* TextFinder::Create(WebLocalFrameImpl& owner_frame) {
  return new TextFinder(owner_frame);
}

TextFinder::TextFinder(WebLocalFrameImpl& owner_frame)
    : owner_frame_(&owner_frame),
      current_active_match_frame_(false),
      active_match_index_(-1),
      resume_scoping_from_range_(nullptr),
      last_match_count_(-1),
      total_match_count_(-1),
      frame_scoping_(false),
      find_request_identifier_(-1),
      next_invalidate_after_(0),
      find_match_markers_version_(0),
      locating_active_rect_(false),
      scoping_in_progress_(false),
      last_find_request_completed_with_no_matches_(false),
      find_match_rects_are_valid_(false) {}

TextFinder::~TextFinder() = default;

bool TextFinder::SetMarkerActive(Range* range, bool active) {
  if (!range || range->collapsed())
    return false;
  return OwnerFrame()
      .GetFrame()
      ->GetDocument()
      ->Markers()
      .SetTextMatchMarkersActive(EphemeralRange(range), active);
}

void TextFinder::UnmarkAllTextMatches() {
  LocalFrame* frame = OwnerFrame().GetFrame();
  if (frame && frame->GetPage() &&
      frame->GetEditor().MarkedTextMatchesAreHighlighted()) {
    frame->GetDocument()->Markers().RemoveMarkersOfTypes(
        DocumentMarker::kTextMatch);
  }
}

bool TextFinder::ShouldScopeMatches(const String& search_text,
                                    const WebFindOptions& options) {
  // Don't scope if we can't find a frame or a view.
  // The user may have closed the tab/application, so abort.
  LocalFrame* frame = OwnerFrame().GetFrame();
  if (!frame || !frame->View() || !frame->GetPage())
    return false;

  DCHECK(frame->GetDocument());
  DCHECK(frame->View());

  if (options.force)
    return true;

  if (!OwnerFrame().HasVisibleContent())
    return false;

  // If the frame completed the scoping operation and found 0 matches the last
  // time it was searched, then we don't have to search it again if the user is
  // just adding to the search string or sending the same search string again.
  if (last_find_request_completed_with_no_matches_ &&
      !last_search_string_.IsEmpty()) {
    // Check to see if the search string prefixes match.
    String previous_search_prefix =
        search_text.Substring(0, last_search_string_.length());

    if (previous_search_prefix == last_search_string_)
      return false;  // Don't search this frame, it will be fruitless.
  }

  return true;
}

void TextFinder::ScopeStringMatchesSoon(int identifier,
                                        const WebString& search_text,
                                        const WebFindOptions& options) {
  DCHECK_EQ(deferred_scoping_work_, nullptr);
  deferred_scoping_work_ = DeferredScopeStringMatches::Create(
      this, identifier, search_text, options);
}

void TextFinder::ResumeScopingStringMatches(int identifier,
                                            const WebString& search_text,
                                            const WebFindOptions& options) {
  deferred_scoping_work_.Clear();

  ScopeStringMatches(identifier, search_text, options);
}

void TextFinder::InvalidateIfNecessary() {
  if (last_match_count_ <= next_invalidate_after_)
    return;

  // FIXME: (http://crbug.com/6819) Optimize the drawing of the tickmarks and
  // remove this. This calculation sets a milestone for when next to
  // invalidate the scrollbar and the content area. We do this so that we
  // don't spend too much time drawing the scrollbar over and over again.
  // Basically, up until the first 500 matches there is no throttle.
  // After the first 500 matches, we set set the milestone further and
  // further out (750, 1125, 1688, 2K, 3K).
  static const int kStartSlowingDownAfter = 500;
  static const int kSlowdown = 750;

  int i = last_match_count_ / kStartSlowingDownAfter;
  next_invalidate_after_ += i * kSlowdown;
  OwnerFrame().GetFrameView()->InvalidatePaintForTickmarks();
}

void TextFinder::FlushCurrentScoping() {
  FlushCurrentScopingEffort(find_request_identifier_);
}

void TextFinder::Trace(blink::Visitor* visitor) {
  visitor->Trace(owner_frame_);
  visitor->Trace(active_match_);
  visitor->Trace(resume_scoping_from_range_);
  visitor->Trace(deferred_scoping_work_);
  visitor->Trace(find_matches_cache_);
}

}  // namespace blink
