// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef MEDIA_BASE_MOCK_DEMUXER_HOST_H_
#define MEDIA_BASE_MOCK_DEMUXER_HOST_H_

#include "base/macros.h"
#include "media/base/demuxer.h"
#include "media/base/text_track_config.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {

class MockDemuxerHost : public DemuxerHost {
 public:
  MockDemuxerHost();
  ~MockDemuxerHost() override;

  MOCK_METHOD1(OnBufferedTimeRangesChanged,
               void(const Ranges<base::TimeDelta>&));
  MOCK_METHOD1(SetDuration, void(base::TimeDelta duration));
  MOCK_METHOD1(OnDemuxerError, void(PipelineStatus error));
  MOCK_METHOD2(AddTextStream, void(DemuxerStream*,
                                   const TextTrackConfig&));
  MOCK_METHOD1(RemoveTextStream, void(DemuxerStream*));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockDemuxerHost);
};

}  // namespace media

#endif  // MEDIA_BASE_MOCK_DEMUXER_HOST_H_
