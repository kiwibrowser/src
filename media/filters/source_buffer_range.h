// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_SOURCE_BUFFER_RANGE_H_
#define MEDIA_FILTERS_SOURCE_BUFFER_RANGE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/media_export.h"
#include "media/base/stream_parser_buffer.h"

namespace media {

// Base class for representing a continuous range of buffered data in the
// presentation timeline. All buffers in a SourceBufferRange are ordered
// sequentially by GOP presentation interval, and within each GOP by decode
// order. Unless constructed with |ALLOW_GAPS|, the range contains no internal
// presentation gaps.
class MEDIA_EXPORT SourceBufferRange {
 public:
  // Returns the maximum distance in time between any buffer seen in the stream
  // of which this range is a part. Used to estimate the duration of a buffer if
  // its duration is not known, and in GetFudgeRoom() for determining whether a
  // time or coded frame is close enough to be considered part of this range.
  using InterbufferDistanceCB = base::Callback<base::TimeDelta()>;

  using BufferQueue = StreamParser::BufferQueue;

  // Policy for handling large gaps between buffers. Continuous media like
  // audio & video should use NO_GAPS_ALLOWED. Discontinuous media like
  // timed text should use ALLOW_GAPS because large differences in timestamps
  // are common and acceptable.
  enum GapPolicy {
    NO_GAPS_ALLOWED,
    ALLOW_GAPS
  };

  // Sequential buffers with the same decode timestamp make sense under certain
  // conditions, typically when the first buffer is a keyframe. Due to some
  // atypical media append behaviors where a new keyframe might have the same
  // decode timestamp as a previous non-keyframe, the playback of the sequence
  // might involve some throwaway decode work. This method supports detecting
  // this situation so that callers can log warnings (it returns true in this
  // case only).
  // For all other cases, including more typical same-DTS sequences, this method
  // returns false. Examples of typical situations where DTS of two consecutive
  // frames can be equal:
  // - Video: VP8 Alt-Ref frames.
  // - Video: IPBPBP...: DTS for I frame and for P frame can be equal.
  // - Text track cues that start at same time.
  // Returns true if |prev_is_keyframe| and |current_is_keyframe| indicate a
  // same timestamp situation that is atypical. False is returned otherwise.
  static bool IsUncommonSameTimestampSequence(bool prev_is_keyframe,
                                              bool current_is_keyframe);

  SourceBufferRange(GapPolicy gap_policy,
                    const InterbufferDistanceCB& interbuffer_distance_cb);

  virtual ~SourceBufferRange();

  // Deletes all buffers in range.
  virtual void DeleteAll(BufferQueue* deleted_buffers) = 0;

  // Seeks to the beginning of the range.
  void SeekToStart();

  // Updates |out_buffer| with the next buffer in presentation order. Seek()
  // must be called before calls to GetNextBuffer(), and buffers are returned
  // in order from the last call to Seek(). Returns true if |out_buffer| is
  // filled with a valid buffer, false if there is not enough data to fulfill
  // the request.
  bool GetNextBuffer(scoped_refptr<StreamParserBuffer>* out_buffer);
  bool HasNextBuffer() const;

  // Returns the config ID for the buffer that will be returned by
  // GetNextBuffer().
  int GetNextConfigId() const;

  // Returns true if the range knows the position of the next buffer it should
  // return, i.e. it has been Seek()ed. This does not necessarily mean that it
  // has the next buffer yet.
  bool HasNextBufferPosition() const;

  // Resets this range to an "unseeked" state.
  void ResetNextBufferPosition();

  // TODO(wolenetz): Remove in favor of
  // GetEndTimestamp()/GetBufferedEndTimestamp() once they report in PTS, not
  // DTS. See https://crbug.com/718641.
  void GetRangeEndTimesForTesting(base::TimeDelta* highest_pts,
                                  base::TimeDelta* end_time) const;

  size_t size_in_bytes() const { return size_in_bytes_; }

 protected:
  // Friend of protected is only for IsNextInPresentationSequence testing.
  friend class SourceBufferStreamTest;

  // Called during AppendBuffersToEnd to adjust estimated duration at the
  // end of the last append to match the delta in timestamps between
  // the last append and the upcoming append. This is a workaround for
  // WebM media where a duration is not always specified. Caller should take
  // care of updating |highest_frame_|.
  void AdjustEstimatedDurationForNewAppend(const BufferQueue& new_buffers);

  // Frees the buffers in |buffers_| from [|start_point|,|ending_point|) and
  // updates the |size_in_bytes_| accordingly. Note, this does not update
  // |keyframe_map_|.
  // TODO(wolenetz): elevate keyframe_map_ to base class so this comment has
  // better context. See https://crbug.com/718641.
  void FreeBufferRange(const BufferQueue::const_iterator& starting_point,
                       const BufferQueue::const_iterator& ending_point);

  // Returns the distance in time estimating how far from the beginning or end
  // of this range a buffer can be to considered in the range.
  base::TimeDelta GetFudgeRoom() const;

  // Returns the approximate duration of a buffer in this range.
  base::TimeDelta GetApproximateDuration() const;

  // Updates |highest_frame_| if |new_buffer| has a higher PTS than
  // |highest_frame_| or if the range was previously empty.
  void UpdateEndTime(scoped_refptr<StreamParserBuffer> new_buffer);

  // Returns true if |timestamp| is allowed in this range as the timestamp of
  // the next buffer in presentation sequence at or after |highest_frame_|.
  // |buffers_| must not be empty, and |highest_frame_| must not be nullptr.
  // Uses |gap_policy_| to potentially allow gaps.
  // TODO(wolenetz): Switch to using this helper in CanAppendBuffersToEnd(),
  // etc, when switching to managing ranges by their presentation interval, and
  // not necessarily just their decode times. See https://crbug.com/718641. Once
  // being used and not just tested, the following also applies:
  // Due to potential for out-of-order decode vs presentation time, this method
  // should only be used to determine adjacency of keyframes with the end of
  // |buffers_|.
  bool IsNextInPresentationSequence(base::TimeDelta timestamp) const;

  // Returns true if |decode_timestamp| is allowed in this range as the decode
  // timestamp of the next buffer in decode sequence at or after the last buffer
  // in |buffers_|'s decode timestamp.  |buffers_| must not be empty. Uses
  // |gap_policy_| to potentially allow gaps.
  // TODO(wolenetz): Switch to using this helper in CanAppendBuffersToEnd(),
  // etc, appropriately when switching to managing ranges by their presentation
  // interval between GOPs, and by their decode sequence within GOPs. See
  // https://crbug.com/718641. Once that's done, the following also would apply:
  // Due to potential for out-of-order decode vs presentation time, this method
  // should only be used to determine adjacency of non-keyframes with the end of
  // |buffers_|, when determining if a non-keyframe with |decode_timestamp|
  // continues the decode sequence of the coded frame group at the end of
  // |buffers_|.
  bool IsNextInDecodeSequence(DecodeTimestamp decode_timestamp) const;

  // Keeps track of whether gaps are allowed.
  const GapPolicy gap_policy_;

  // An ordered list of buffers in this range.
  BufferQueue buffers_;

  // Index into |buffers_| for the next buffer to be returned by
  // GetNextBuffer(), set to -1 before Seek().
  int next_buffer_index_;

  // Caches the buffer, if any, with the highest PTS currently in |buffers_|.
  // This is nullptr if this range is empty.  This is useful in determining
  // range membership and adjacency in SourceBufferRangeByPts.
  scoped_refptr<StreamParserBuffer> highest_frame_;

  // Called to get the largest interbuffer distance seen so far in the stream.
  InterbufferDistanceCB interbuffer_distance_cb_;

  // Stores the amount of memory taken up by the data in |buffers_|.
  size_t size_in_bytes_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SourceBufferRange);
};

}  // namespace media

#endif  // MEDIA_FILTERS_SOURCE_BUFFER_RANGE_H_
