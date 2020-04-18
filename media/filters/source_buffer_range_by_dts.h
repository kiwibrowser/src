// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_SOURCE_BUFFER_RANGE_BY_DTS_H_
#define MEDIA_FILTERS_SOURCE_BUFFER_RANGE_BY_DTS_H_

#include <stddef.h>
#include <map>
#include <memory>

#include "media/filters/source_buffer_range.h"

namespace media {

// Helper class representing a continuous range of buffered data in the
// presentation timeline. All buffers in a SourceBufferRangeByDts are ordered
// sequentially in decode timestamp order with no gaps. This class conflates
// decode intervals (and decode gaps) with presentation intervals and gaps; it
// provides the "LegacyByDts" buffering implementation used prior to fixing
// https://crbug.com/718641.
class MEDIA_EXPORT SourceBufferRangeByDts : public SourceBufferRange {
 public:
  // Creates a range with |new_buffers|. |new_buffers| cannot be empty and the
  // front of |new_buffers| must be a keyframe.
  // |range_start_decode_time| refers to the starting timestamp for the coded
  // frame group to which these buffers belong.
  SourceBufferRangeByDts(GapPolicy gap_policy,
                         const BufferQueue& new_buffers,
                         DecodeTimestamp range_start_decode_time,
                         const InterbufferDistanceCB& interbuffer_distance_cb);

  ~SourceBufferRangeByDts() override;

  void DeleteAll(BufferQueue* deleted_buffers) override;

  // Appends the buffers from |range| into this range.
  // The first buffer in |range| must come directly after the last buffer
  // in this range.
  // If |transfer_current_position| is true, |range|'s |next_buffer_index_|
  // is transfered to this SourceBufferRange.
  // Note: Use these only to merge existing ranges. |range|'s first buffer
  // timestamp must be adjacent to this range. No group start timestamp
  // adjacency is involved in these methods.
  // During append, |highest_frame_| is updated, if necessary.
  void AppendRangeToEnd(const SourceBufferRangeByDts& range,
                        bool transfer_current_position);
  bool CanAppendRangeToEnd(const SourceBufferRangeByDts& range) const;

  // Appends |buffers| to the end of the range and updates |keyframe_map_| as
  // it encounters new keyframes.
  // If |new_buffers_group_start_timestamp| is kNoDecodeTimestamp(), then the
  // first buffer in |buffers| must come directly after the last buffer in this
  // range (within the fudge room), or be a keyframe that starts precisely where
  // the last buffer in this range ends and that last buffer has estimated
  // duration.
  // If |new_buffers_group_start_timestamp| is set otherwise, then that time
  // must come directly after the last buffer in this range (within the fudge
  // room) or precisely where the last buffer in this range ends and that last
  // buffer has estimated duration.
  // The latter scenario is required when a muxed coded frame group has such a
  // large jagged start across tracks that its first buffer is not within the
  // fudge room, yet its group start was.
  // The conditions around estimated duration are handled by
  // AllowableAppendAfterEstimatedDuration, and are intended to solve the edge
  // case in the SourceBufferStreamTest
  // MergeAllowedIfRangeEndTimeWithEstimatedDurationMatchesNextRangeStart.
  // During append, |highest_frame_| is updated, if necessary.
  void AppendBuffersToEnd(const BufferQueue& buffers,
                          DecodeTimestamp new_buffers_group_start_timestamp);
  bool AllowableAppendAfterEstimatedDuration(
      const BufferQueue& buffers,
      DecodeTimestamp new_buffers_group_start_timestamp) const;
  bool CanAppendBuffersToEnd(
      const BufferQueue& buffers,
      DecodeTimestamp new_buffers_group_start_timestamp) const;

  // Updates |next_buffer_index_| to point to the Buffer containing |timestamp|.
  // Assumes |timestamp| is valid and in this range.
  void Seek(DecodeTimestamp timestamp);

  // Return the config ID for the buffer at |timestamp|. Precondition: callers
  // must first verify CanSeekTo(timestamp) == true.
  int GetConfigIdAtTime(DecodeTimestamp timestamp) const;

  // Return true if all buffers in range of [start, end] have the same config
  // ID. Precondition: callers must first verify that
  // CanSeekTo(start) ==  CanSeekTo(end) == true.
  bool SameConfigThruRange(DecodeTimestamp start, DecodeTimestamp end) const;

  // Finds the next keyframe from |buffers_| starting at or after |timestamp|
  // and creates and returns a new SourceBufferRangeByDts with the buffers from
  // that keyframe onward. The buffers in the new SourceBufferRangeByDts are
  // moved out of this range. The start time of the new SourceBufferRangeByDts
  // is set to the later of |timestamp| and this range's GetStartTimestamp().
  // If there is no keyframe at or after |timestamp|, SplitRange() returns null
  // and this range is unmodified. This range can become empty if |timestamp| <=
  // the DTS of the first buffer in this range.  |highest_frame_| is updated, if
  // necessary.
  std::unique_ptr<SourceBufferRangeByDts> SplitRange(DecodeTimestamp timestamp);

  // Deletes the buffers from this range starting at |timestamp|, exclusive if
  // |is_exclusive| is true, inclusive otherwise.
  // Resets |next_buffer_index_| if the buffer at |next_buffer_index_| was
  // deleted, and deletes the |keyframe_map_| entries for the buffers that
  // were removed.
  // |highest_frame_| is updated, if necessary.
  // |deleted_buffers| contains the buffers that were deleted from this range,
  // starting at the buffer that had been at |next_buffer_index_|.
  // Returns true if everything in the range was deleted. Otherwise
  // returns false.
  bool TruncateAt(DecodeTimestamp timestamp,
                  BufferQueue* deleted_buffers,
                  bool is_exclusive);

  // Deletes a GOP from the front or back of the range and moves these
  // buffers into |deleted_buffers|. Returns the number of bytes deleted from
  // the range (i.e. the size in bytes of |deleted_buffers|).
  // |highest_frame_| is updated, if necessary.
  // This range must NOT be empty when these methods are called.
  // The GOP being deleted must NOT contain the next buffer position.
  size_t DeleteGOPFromFront(BufferQueue* deleted_buffers);
  size_t DeleteGOPFromBack(BufferQueue* deleted_buffers);

  // Gets the range of GOP to secure at least |bytes_to_free| from
  // [|start_timestamp|, |end_timestamp|).
  // Returns the size of the buffers to secure if the buffers of
  // [|start_timestamp|, |end_removal_timestamp|) is removed.
  // Will not update |end_removal_timestamp| if the returned size is 0.
  size_t GetRemovalGOP(DecodeTimestamp start_timestamp,
                       DecodeTimestamp end_timestamp,
                       size_t bytes_to_free,
                       DecodeTimestamp* end_removal_timestamp) const;

  // Returns true iff the buffered end time of the first GOP in this range is
  // at or before |media_time|.
  bool FirstGOPEarlierThanMediaTime(DecodeTimestamp media_time) const;

  // Indicates whether the GOP at the beginning or end of the range contains the
  // next buffer position.
  bool FirstGOPContainsNextBufferPosition() const;
  bool LastGOPContainsNextBufferPosition() const;

  // Returns the timestamp of the next buffer that will be returned from
  // GetNextBuffer(), or kNoTimestamp if the timestamp is unknown.
  DecodeTimestamp GetNextTimestamp() const;

  // Returns the start timestamp of the range.
  DecodeTimestamp GetStartTimestamp() const;

  // Returns the timestamp of the last buffer in the range.
  DecodeTimestamp GetEndTimestamp() const;

  // Returns the timestamp for the end of the buffered region in this range.
  // This is an approximation if the duration for the last buffer in the range
  // is unset.
  DecodeTimestamp GetBufferedEndTimestamp() const;

  // Returns whether a buffer with a starting timestamp of |timestamp| would
  // belong in this range. This includes a buffer that would be appended to
  // the end of the range.
  bool BelongsToRange(DecodeTimestamp timestamp) const;

  // Returns the highest time from among GetStartTimestamp() and frame decode
  // timestamp (in order in |buffers_| beginning at the first keyframe at or
  // before |timestamp|) for buffers in this range up to and including
  // |timestamp|.
  // Note that |timestamp| must belong to this range.
  DecodeTimestamp FindHighestBufferedTimestampAtOrBefore(
      DecodeTimestamp timestamp) const;

  // Gets the timestamp for the keyframe that is after |timestamp|. If
  // there isn't a keyframe in the range after |timestamp| then kNoTimestamp
  // is returned. If |timestamp| is in the "gap" between the value  returned by
  // GetStartTimestamp() and the timestamp on the first buffer in |buffers_|,
  // then |timestamp| is returned.
  DecodeTimestamp NextKeyframeTimestamp(DecodeTimestamp timestamp) const;

  // Gets the timestamp for the closest keyframe that is <= |timestamp|. If
  // there isn't a keyframe before |timestamp| or |timestamp| is outside
  // this range, then kNoTimestamp is returned.
  DecodeTimestamp KeyframeBeforeTimestamp(DecodeTimestamp timestamp) const;

  // Returns true if the range has enough data to seek to the specified
  // |timestamp|, false otherwise.
  bool CanSeekTo(DecodeTimestamp timestamp) const;

  // Adds all buffers which overlap [start, end) to the end of |buffers|.  If
  // no buffers exist in the range returns false, true otherwise.
  bool GetBuffersInRange(DecodeTimestamp start,
                         DecodeTimestamp end,
                         BufferQueue* buffers) const;

 private:
  typedef std::map<DecodeTimestamp, int> KeyframeMap;

  // Helper method for Appending |range| to the end of this range.  If |range|'s
  // first buffer time is before the time of the last buffer in this range,
  // returns kNoDecodeTimestamp().  Otherwise, returns the closest time within
  // [|range|'s start time, |range|'s first buffer time] that is at or after the
  // time of the last buffer in this range. This allows |range| to potentially
  // be determined to be adjacent within fudge room for appending to the end of
  // this range, especially if |range| has a start time that is before its first
  // buffer's time.
  DecodeTimestamp NextRangeStartTimeForAppendRangeToEnd(
      const SourceBufferRangeByDts& range) const;

  // Helper method to delete buffers in |buffers_| starting at
  // |starting_point|, an iterator in |buffers_|.
  // Returns true if everything in the range was removed. Returns
  // false if the range still contains buffers.
  bool TruncateAt(const BufferQueue::const_iterator& starting_point,
                  BufferQueue* deleted_buffers);

  // Returns an iterator in |buffers_| pointing to the buffer at |timestamp|.
  // If |skip_given_timestamp| is true, this returns the first buffer with
  // timestamp greater than |timestamp|.
  BufferQueue::const_iterator GetBufferItrAt(DecodeTimestamp timestamp,
                                             bool skip_given_timestamp) const;

  // Returns an iterator in |keyframe_map_| pointing to the next keyframe after
  // |timestamp|. If |skip_given_timestamp| is true, this returns the first
  // keyframe with a timestamp strictly greater than |timestamp|.
  KeyframeMap::const_iterator GetFirstKeyframeAt(
      DecodeTimestamp timestamp,
      bool skip_given_timestamp) const;

  // Returns an iterator in |keyframe_map_| pointing to the first keyframe
  // before or at |timestamp|.
  KeyframeMap::const_iterator GetFirstKeyframeAtOrBefore(
      DecodeTimestamp timestamp) const;

  // Updates |highest_frame_| to be the frame with highest PTS in the last GOP
  // in this range.  If there are no buffers in this range, resets
  // |highest_frame_|.
  // Normally, incremental additions to this range should just use
  // UpdateEndTime(). When removing buffers from this range (which could be out
  // of order presentation vs decode order), inspecting the last buffer in
  // decode order of this range can be insufficient to determine the correct
  // presentation end time of this range. Hence this helper method.
  void UpdateEndTimeUsingLastGOP();

  // If the first buffer in this range is the beginning of a coded frame group,
  // |range_start_decode_time_| is the time when the coded frame group begins.
  // This is especially important in muxed media where the first coded frames
  // for each track do not necessarily begin at the same time.
  // |range_start_decode_time_| may be <= the timestamp of the first buffer in
  // |buffers_|. |range_start_decode_time_| is kNoDecodeTimestamp() if this
  // range does not start at the beginning of a coded frame group, which can
  // happen by range removal or split when we don't have a way of knowing,
  // across potentially multiple muxed streams, the coded frame group start
  // timestamp for the new range.
  DecodeTimestamp range_start_decode_time_;

  // Index base of all positions in |keyframe_map_|. In other words, the
  // real position of entry |k| of |keyframe_map_| in the range is:
  //   keyframe_map_[k] - keyframe_map_index_base_
  int keyframe_map_index_base_;

  // Maps keyframe decode timestamps to its index position in |buffers_|.
  KeyframeMap keyframe_map_;

  DISALLOW_COPY_AND_ASSIGN(SourceBufferRangeByDts);
};

}  // namespace media

#endif  // MEDIA_FILTERS_SOURCE_BUFFER_RANGE_BY_DTS_H_
