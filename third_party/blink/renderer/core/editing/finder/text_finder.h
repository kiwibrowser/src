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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_FINDER_TEXT_FINDER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_FINDER_TEXT_FINDER_H_

#include "base/macros.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class LocalFrame;
class Range;
class WebLocalFrameImpl;
class WebString;
struct WebFindOptions;
struct WebFloatPoint;
struct WebFloatRect;
struct WebRect;

class CORE_EXPORT TextFinder final
    : public GarbageCollectedFinalized<TextFinder> {
 public:
  static TextFinder* Create(WebLocalFrameImpl& owner_frame);

  bool Find(int identifier,
            const WebString& search_text,
            const WebFindOptions&,
            bool wrap_within_frame,
            bool* active_now = nullptr);
  void ClearActiveFindMatch();
  void SetFindEndstateFocusAndSelection();
  void StopFindingAndClearSelection();
  void IncreaseMatchCount(int identifier, int count);
  int FindMatchMarkersVersion() const { return find_match_markers_version_; }
  WebFloatRect ActiveFindMatchRect();
  Vector<WebFloatRect> FindMatchRects();
  int SelectNearestFindMatch(const WebFloatPoint&, WebRect* selection_rect);

  // Starts brand new scoping request: resets the scoping state and
  // asyncronously calls scopeStringMatches().
  void StartScopingStringMatches(int identifier,
                                 const WebString& search_text,
                                 const WebFindOptions&);

  // Cancels any outstanding requests for scoping string matches on the frame.
  void CancelPendingScopingEffort();

  // This function is called to reset the total number of matches found during
  // the scoping effort.
  void ResetMatchCount();

  // Return the index in the find-in-page cache of the match closest to the
  // provided point in find-in-page coordinates, or -1 in case of error.
  // The squared distance to the closest match is returned in the
  // |distanceSquared| parameter.
  int NearestFindMatch(const FloatPoint&, float* distance_squared);

  // Returns whether this frame has the active match.
  bool ActiveMatchFrame() const { return current_active_match_frame_; }

  // Returns the active match in the current frame. Could be a null range if
  // the local frame has no active match.
  Range* ActiveMatch() const { return active_match_.Get(); }

  void FlushCurrentScoping();

  void ResetActiveMatch() { active_match_ = nullptr; }

  bool FrameScoping() const { return frame_scoping_; }
  int TotalMatchCount() const { return total_match_count_; }
  bool ScopingInProgress() const { return scoping_in_progress_; }
  void IncreaseMarkerVersion() { ++find_match_markers_version_; }

  ~TextFinder();

  class FindMatch {
    DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

   public:
    FindMatch(Range*, int ordinal);

    void Trace(blink::Visitor*);

    Member<Range> range_;

    // 1-based index within this frame.
    int ordinal_;

    // In find-in-page coordinates.
    // Lazily calculated by updateFindMatchRects.
    FloatRect rect_;
  };

  void Trace(blink::Visitor*);

 private:
  class DeferredScopeStringMatches;
  friend class DeferredScopeStringMatches;

  explicit TextFinder(WebLocalFrameImpl& owner_frame);

  // Notifies the delegate about a new selection rect.
  void ReportFindInPageSelection(const WebRect& selection_rect,
                                 int active_match_ordinal,
                                 int identifier);

  void ReportFindInPageResultToAccessibility(int identifier);

  // Clear the find-in-page matches cache forcing rects to be fully
  // calculated again next time updateFindMatchRects is called.
  void ClearFindMatchesCache();

  // Select a find-in-page match marker in the current frame using a cache
  // match index returned by nearestFindMatch. Returns the ordinal of the new
  // selected match or -1 in case of error. Also provides the bounding box of
  // the marker in window coordinates if selectionRect is not null.
  int SelectFindMatch(unsigned index, WebRect* selection_rect);

  // Compute and cache the rects for FindMatches if required.
  // Rects are automatically invalidated in case of content size changes.
  void UpdateFindMatchRects();

  // Sets the markers within a range as active or inactive. Returns true if at
  // least one such marker found.
  bool SetMarkerActive(Range*, bool active);

  // Removes all markers.
  void UnmarkAllTextMatches();

  // Determines whether the scoping effort is required for a particular frame.
  // It is not necessary if the frame is invisible, for example, or if this
  // is a repeat search that already returned nothing last time the same prefix
  // was searched.
  bool ShouldScopeMatches(const WTF::String& search_text,
                          const WebFindOptions&);

  // Removes the current frame from the global scoping effort and triggers any
  // updates if appropriate. This method does not mark the scoping operation
  // as finished.
  void FlushCurrentScopingEffort(int identifier);

  // Finishes the current scoping effort and triggers any updates if
  // appropriate.
  void FinishCurrentScopingEffort(int identifier);

  // Counts how many times a particular string occurs within the frame.  It
  // also retrieves the location of the string and updates a vector in the
  // frame so that tick-marks and highlighting can be drawn.  This function
  // does its work asynchronously, by running for a certain time-slice and
  // then scheduling itself (co-operative multitasking) to be invoked later
  // (repeating the process until all matches have been found).  This allows
  // multiple frames to be searched at the same time and provides a way to
  // cancel at any time (see cancelPendingScopingEffort).  The parameter
  // searchText specifies what to look for.
  void ScopeStringMatches(int identifier,
                          const WebString& search_text,
                          const WebFindOptions&);

  // Queue up a deferred call to scopeStringMatches.
  void ScopeStringMatchesSoon(int identifier,
                              const WebString& search_text,
                              const WebFindOptions&);

  // Called by a DeferredScopeStringMatches instance.
  void ResumeScopingStringMatches(int identifier,
                                  const WebString& search_text,
                                  const WebFindOptions&);

  // Determines whether to invalidate the content area and scrollbar.
  void InvalidateIfNecessary();

  LocalFrame* GetFrame() const;

  WebLocalFrameImpl& OwnerFrame() const {
    DCHECK(owner_frame_);
    return *owner_frame_;
  }

  Member<WebLocalFrameImpl> owner_frame_;

  // Indicates whether this frame currently has the active match.
  bool current_active_match_frame_;

  // The range of the active match for the current frame.
  Member<Range> active_match_;

  // The index of the active match for the current frame.
  int active_match_index_;

  // The scoping effort can time out and we need to keep track of where we
  // ended our last search so we can continue from where we left of.
  //
  // This range is collapsed to the end position of the last successful
  // search; the new search should start from this position.
  Member<Range> resume_scoping_from_range_;

  // Keeps track of the last string this frame searched for. This is used for
  // short-circuiting searches in the following scenarios: When a frame has
  // been searched and returned 0 results, we don't need to search that frame
  // again if the user is just adding to the search (making it more specific).
  WTF::String last_search_string_;

  // Keeps track of how many matches this frame has found so far, so that we
  // don't lose count between scoping efforts, and is also used (in conjunction
  // with m_lastSearchString) to figure out if we need to search the frame
  // again.
  int last_match_count_;

  // This variable keeps a cumulative total of matches found so far in this
  // frame, and is only incremented by calling IncreaseMatchCount.
  int total_match_count_;

  // Keeps track of whether the frame is currently scoping (being searched for
  // matches).
  bool frame_scoping_;

  // Identifier of the latest find-in-page request. Required to be stored in
  // the frame in order to reply if required in case the frame is detached.
  int find_request_identifier_;

  // Keeps track of when the scoping effort should next invalidate the scrollbar
  // and the frame area.
  int next_invalidate_after_;

  // Pending call to scopeStringMatches.
  Member<DeferredScopeStringMatches> deferred_scoping_work_;

  // Version number incremented whenever this frame's find-in-page match
  // markers change.
  int find_match_markers_version_;

  // Local cache of the find match markers currently displayed for this frame.
  HeapVector<FindMatch> find_matches_cache_;

  // Contents size when find-in-page match rects were last computed for this
  // frame's cache.
  IntSize document_size_for_current_find_match_rects_;

  // This flag is used by the scoping effort to determine if we need to figure
  // out which rectangle is the active match. Once we find the active
  // rectangle we clear this flag.
  bool locating_active_rect_;

  // Keeps track of whether there is an scoping effort ongoing in the frame.
  bool scoping_in_progress_;

  // Keeps track of whether the last find request completed its scoping effort
  // without finding any matches in this frame.
  bool last_find_request_completed_with_no_matches_;

  // Determines if the rects in the find-in-page matches cache of this frame
  // are invalid and should be recomputed.
  bool find_match_rects_are_valid_;

  DISALLOW_COPY_AND_ASSIGN(TextFinder);
};

}  // namespace blink

WTF_ALLOW_INIT_WITH_MEM_FUNCTIONS(blink::TextFinder::FindMatch);

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EDITING_FINDER_TEXT_FINDER_H_
