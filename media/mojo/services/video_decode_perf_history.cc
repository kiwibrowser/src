// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/video_decode_perf_history.h"

#include "base/callback.h"
#include "base/format_macros.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "media/base/video_codecs.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace media {

VideoDecodePerfHistory::VideoDecodePerfHistory(
    std::unique_ptr<VideoDecodeStatsDBFactory> db_factory)
    : db_factory_(std::move(db_factory)),
      db_init_status_(UNINITIALIZED),
      weak_ptr_factory_(this) {
  DVLOG(2) << __func__;
}

VideoDecodePerfHistory::~VideoDecodePerfHistory() {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void VideoDecodePerfHistory::BindRequest(
    mojom::VideoDecodePerfHistoryRequest request) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bindings_.AddBinding(this, std::move(request));
}

void VideoDecodePerfHistory::InitDatabase() {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (db_init_status_ == PENDING)
    return;

  db_ = db_factory_->CreateDB();
  db_->Initialize(base::BindOnce(&VideoDecodePerfHistory::OnDatabaseInit,
                                 weak_ptr_factory_.GetWeakPtr()));
  db_init_status_ = PENDING;
}

void VideoDecodePerfHistory::OnDatabaseInit(bool success) {
  DVLOG(2) << __func__ << " " << success;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(db_init_status_, PENDING);

  db_init_status_ = success ? COMPLETE : FAILED;

  // Post all the deferred API calls as if they're just now coming in. Posting
  // avoids subtle issues with deferred calls that may otherwise re-enter and
  // potentially reinitialize the DB (e.g. ClearHistory).
  for (auto& deferred_call : init_deferred_api_calls_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  std::move(deferred_call));
  }
  init_deferred_api_calls_.clear();
}

void VideoDecodePerfHistory::GetPerfInfo(mojom::PredictionFeaturesPtr features,
                                         GetPerfInfoCallback got_info_cb) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK_NE(features->profile, VIDEO_CODEC_PROFILE_UNKNOWN);
  DCHECK_GT(features->frames_per_sec, 0);
  DCHECK(features->video_size.width() > 0 && features->video_size.height() > 0);

  if (db_init_status_ == FAILED) {
    // Optimistically claim perf is both smooth and power efficient.
    std::move(got_info_cb).Run(true, true);
    return;
  }

  // Defer this request until the DB is initialized.
  if (db_init_status_ != COMPLETE) {
    init_deferred_api_calls_.push_back(base::BindOnce(
        &VideoDecodePerfHistory::GetPerfInfo, weak_ptr_factory_.GetWeakPtr(),
        std::move(features), std::move(got_info_cb)));
    InitDatabase();
    return;
  }

  VideoDecodeStatsDB::VideoDescKey video_key =
      VideoDecodeStatsDB::VideoDescKey::MakeBucketedKey(
          features->profile, features->video_size, features->frames_per_sec);

  db_->GetDecodeStats(
      video_key, base::BindOnce(&VideoDecodePerfHistory::OnGotStatsForRequest,
                                weak_ptr_factory_.GetWeakPtr(), video_key,
                                std::move(got_info_cb)));
}

void VideoDecodePerfHistory::AssessStats(
    const VideoDecodeStatsDB::DecodeStatsEntry* stats,
    bool* is_smooth,
    bool* is_power_efficient) {
  // TODO(chcunningham/mlamouri): Refactor database API to give us nearby
  // stats whenever we don't have a perfect match. If higher
  // resolutions/frame rates are known to be smooth, we can report this as
  /// smooth. If lower resolutions/frames are known to be janky, we can assume
  // this will be janky.

  // No stats? Lets be optimistic.
  if (!stats) {
    *is_power_efficient = true;
    *is_smooth = true;
    return;
  }

  double percent_dropped =
      static_cast<double>(stats->frames_dropped) / stats->frames_decoded;
  double percent_power_efficient =
      static_cast<double>(stats->frames_decoded_power_efficient) /
      stats->frames_decoded;

  *is_power_efficient =
      percent_power_efficient >= kMinPowerEfficientDecodedFramePercent;
  *is_smooth = percent_dropped <= kMaxSmoothDroppedFramesPercent;
}

void VideoDecodePerfHistory::OnGotStatsForRequest(
    const VideoDecodeStatsDB::VideoDescKey& video_key,
    GetPerfInfoCallback got_info_cb,
    bool database_success,
    std::unique_ptr<VideoDecodeStatsDB::DecodeStatsEntry> stats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!got_info_cb.is_null());
  DCHECK_EQ(db_init_status_, COMPLETE);

  bool is_power_efficient = false;
  bool is_smooth = false;
  double percent_dropped = 0;
  double percent_power_efficient = 0;

  AssessStats(stats.get(), &is_smooth, &is_power_efficient);

  if (stats) {
    DCHECK(database_success);
    percent_dropped =
        static_cast<double>(stats->frames_dropped) / stats->frames_decoded;
    percent_power_efficient =
        static_cast<double>(stats->frames_decoded_power_efficient) /
        stats->frames_decoded;
  }

  DVLOG(3) << __func__
           << base::StringPrintf(
                  " profile:%s size:%s fps:%d --> ",
                  GetProfileName(video_key.codec_profile).c_str(),
                  video_key.size.ToString().c_str(), video_key.frame_rate)
           << (stats.get()
                   ? base::StringPrintf(
                         "smooth:%d frames_decoded:%" PRIu64 " pcnt_dropped:%f"
                         " pcnt_power_efficent:%f",
                         is_smooth, stats->frames_decoded, percent_dropped,
                         percent_power_efficient)
                   : (database_success ? "no info" : "query FAILED"));

  std::move(got_info_cb).Run(is_smooth, is_power_efficient);
}

void VideoDecodePerfHistory::SavePerfRecord(
    const url::Origin& untrusted_top_frame_origin,
    bool is_top_frame,
    mojom::PredictionFeatures features,
    mojom::PredictionTargets targets,
    uint64_t player_id,
    base::OnceClosure save_done_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(3) << __func__
           << base::StringPrintf(
                  " profile:%s size:%s fps:%d decoded:%d dropped:%d",
                  GetProfileName(features.profile).c_str(),
                  features.video_size.ToString().c_str(),
                  features.frames_per_sec, targets.frames_decoded,
                  targets.frames_dropped);

  if (db_init_status_ == FAILED) {
    DVLOG(3) << __func__ << " Can't save stats. No DB!";
    return;
  }

  // Defer this request until the DB is initialized.
  if (db_init_status_ != COMPLETE) {
    init_deferred_api_calls_.push_back(base::BindOnce(
        &VideoDecodePerfHistory::SavePerfRecord, weak_ptr_factory_.GetWeakPtr(),
        untrusted_top_frame_origin, is_top_frame, std::move(features),
        std::move(targets), player_id, std::move(save_done_cb)));
    InitDatabase();
    return;
  }

  VideoDecodeStatsDB::VideoDescKey video_key =
      VideoDecodeStatsDB::VideoDescKey::MakeBucketedKey(
          features.profile, features.video_size, features.frames_per_sec);
  VideoDecodeStatsDB::DecodeStatsEntry new_stats(
      targets.frames_decoded, targets.frames_dropped,
      targets.frames_decoded_power_efficient);

  // Get past perf info and report UKM metrics before saving this record.
  db_->GetDecodeStats(
      video_key,
      base::BindOnce(&VideoDecodePerfHistory::OnGotStatsForSave,
                     weak_ptr_factory_.GetWeakPtr(), untrusted_top_frame_origin,
                     is_top_frame, player_id, video_key, new_stats,
                     std::move(save_done_cb)));
}

void VideoDecodePerfHistory::OnGotStatsForSave(
    const url::Origin& untrusted_top_frame_origin,
    bool is_top_frame,
    uint64_t player_id,
    const VideoDecodeStatsDB::VideoDescKey& video_key,
    const VideoDecodeStatsDB::DecodeStatsEntry& new_stats,
    base::OnceClosure save_done_cb,
    bool success,
    std::unique_ptr<VideoDecodeStatsDB::DecodeStatsEntry> past_stats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(db_init_status_, COMPLETE);

  if (!success) {
    DVLOG(3) << __func__ << " FAILED! Aborting save.";
    std::move(save_done_cb).Run();
    return;
  }

  ReportUkmMetrics(untrusted_top_frame_origin, is_top_frame, player_id,
                   video_key, new_stats, past_stats.get());

  // TODO(dalecurtis): Abort stats recording if db_ is in read-only mode.

  db_->AppendDecodeStats(
      video_key, new_stats,
      base::BindOnce(&VideoDecodePerfHistory::OnSaveDone,
                     weak_ptr_factory_.GetWeakPtr(), std::move(save_done_cb)));
}

void VideoDecodePerfHistory::OnSaveDone(base::OnceClosure save_done_cb,
                                        bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // TODO(chcunningham): Monitor UMA. Experiment with re-initializing DB to
  // remedy IO failures.
  DVLOG(3) << __func__ << (success ? " succeeded" : " FAILED!");

  // Don't bother to bubble success. Its not actionable for upper layers. Also,
  // save_done_cb only used for test sequencing, where DB should always behave
  // (or fail the test).
  if (save_done_cb)
    std::move(save_done_cb).Run();
}

void VideoDecodePerfHistory::ReportUkmMetrics(
    const url::Origin& untrusted_top_frame_origin,
    bool is_top_frame,
    uint64_t player_id,
    const VideoDecodeStatsDB::VideoDescKey& video_key,
    const VideoDecodeStatsDB::DecodeStatsEntry& new_stats,
    VideoDecodeStatsDB::DecodeStatsEntry* past_stats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // UKM may be unavailable in content_shell or other non-chrome/ builds; it
  // may also be unavailable if browser shutdown has started; so this may be a
  // nullptr. If it's unavailable, UKM reporting will be skipped.
  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (!ukm_recorder)
    return;

  const int32_t source_id = ukm_recorder->GetNewSourceID();
  ukm::builders::Media_VideoDecodePerfRecord builder(source_id);

  // TODO(crbug.com/787209): Stop getting origin from the renderer.
  ukm_recorder->UpdateSourceURL(source_id, untrusted_top_frame_origin.GetURL());
  builder.SetVideo_InTopFrame(is_top_frame);
  builder.SetVideo_PlayerID(player_id);

  builder.SetVideo_CodecProfile(video_key.codec_profile);
  builder.SetVideo_FramesPerSecond(video_key.frame_rate);
  builder.SetVideo_NaturalHeight(video_key.size.height());
  builder.SetVideo_NaturalWidth(video_key.size.width());

  bool past_is_smooth = false;
  bool past_is_efficient = false;
  AssessStats(past_stats, &past_is_smooth, &past_is_efficient);
  builder.SetPerf_ApiWouldClaimIsSmooth(past_is_smooth);
  builder.SetPerf_ApiWouldClaimIsPowerEfficient(past_is_efficient);
  if (past_stats) {
    builder.SetPerf_PastVideoFramesDecoded(past_stats->frames_decoded);
    builder.SetPerf_PastVideoFramesDropped(past_stats->frames_dropped);
    builder.SetPerf_PastVideoFramesPowerEfficient(
        past_stats->frames_decoded_power_efficient);
  } else {
    builder.SetPerf_PastVideoFramesDecoded(0);
    builder.SetPerf_PastVideoFramesDropped(0);
    builder.SetPerf_PastVideoFramesPowerEfficient(0);
  }

  bool new_is_smooth = false;
  bool new_is_efficient = false;
  AssessStats(&new_stats, &new_is_smooth, &new_is_efficient);
  builder.SetPerf_RecordIsSmooth(new_is_smooth);
  builder.SetPerf_RecordIsPowerEfficient(new_is_efficient);
  builder.SetPerf_VideoFramesDecoded(new_stats.frames_decoded);
  builder.SetPerf_VideoFramesDropped(new_stats.frames_dropped);
  builder.SetPerf_VideoFramesPowerEfficient(
      new_stats.frames_decoded_power_efficient);

  builder.Record(ukm_recorder);
}

void VideoDecodePerfHistory::ClearHistory(base::OnceClosure clear_done_cb) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (db_init_status_ == FAILED) {
    DVLOG(3) << __func__ << " Can't clear history - No DB!";
    std::move(clear_done_cb).Run();
    return;
  }

  // Defer this request until the DB is initialized.
  if (db_init_status_ != COMPLETE) {
    init_deferred_api_calls_.push_back(base::BindOnce(
        &VideoDecodePerfHistory::ClearHistory, weak_ptr_factory_.GetWeakPtr(),
        std::move(clear_done_cb)));
    InitDatabase();
    return;
  }

  // Set status to pending to prevent using the DB while destruction is ongoing.
  // Once finished, we will re-initialize the DB and run any deferred API calls.
  db_init_status_ = PENDING;
  db_->DestroyStats(base::BindOnce(&VideoDecodePerfHistory::OnClearedHistory,
                                   weak_ptr_factory_.GetWeakPtr(),
                                   std::move(clear_done_cb)));
}

void VideoDecodePerfHistory::OnClearedHistory(base::OnceClosure clear_done_cb) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // DB is effectively uninitialized while destructively clearing the history.
  // During this period |db_init_status_| should be PENDING to prevent other
  // APIs from racing to reinitialize.
  DCHECK_EQ(db_init_status_, PENDING);
  // With destructive clearing complete, reset to UNITINIALIZED so
  // InitDatabase() will run initialization and any deferred API calls once
  // complete.
  db_init_status_ = UNINITIALIZED;
  InitDatabase();

  std::move(clear_done_cb).Run();
}

}  // namespace media
