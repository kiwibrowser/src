// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capabilities/video_decode_stats_db_impl.h"

#include <memory>
#include <tuple>

#include "base/files/file_path.h"
#include "base/format_macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequence_checker.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "components/leveldb_proto/proto_database_impl.h"
#include "media/capabilities/video_decode_stats.pb.h"

namespace media {

namespace {

// Avoid changing client name. Used in UMA.
// See comments in components/leveldb_proto/leveldb_database.h
const char kDatabaseClientName[] = "VideoDecodeStatsDB";

// Serialize the |entry| to a string to use as a key in the database.
std::string SerializeKey(const VideoDecodeStatsDB::VideoDescKey& key) {
  return base::StringPrintf("%d|%s|%d", static_cast<int>(key.codec_profile),
                            key.size.ToString().c_str(), key.frame_rate);
}

// For debug logging.
std::string KeyToString(const VideoDecodeStatsDB::VideoDescKey& key) {
  return "Key {" + SerializeKey(key) + "}";
}

// For debug logging.
std::string EntryToString(const VideoDecodeStatsDB::DecodeStatsEntry& entry) {
  return base::StringPrintf("DecodeStatsEntry {frames decoded:%" PRIu64
                            ", dropped:%" PRIu64
                            ", power efficient decoded:%" PRIu64 "}",
                            entry.frames_decoded, entry.frames_dropped,
                            entry.frames_decoded_power_efficient);
}

};  // namespace

VideoDecodeStatsDBImplFactory::VideoDecodeStatsDBImplFactory(
    base::FilePath db_dir)
    : db_dir_(db_dir) {
  DVLOG(2) << __func__ << " db_dir:" << db_dir_;
}

VideoDecodeStatsDBImplFactory::~VideoDecodeStatsDBImplFactory() = default;

std::unique_ptr<VideoDecodeStatsDB> VideoDecodeStatsDBImplFactory::CreateDB() {
  std::unique_ptr<leveldb_proto::ProtoDatabase<DecodeStatsProto>> db_;

  auto inner_db =
      std::make_unique<leveldb_proto::ProtoDatabaseImpl<DecodeStatsProto>>(
          base::CreateSequencedTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::BACKGROUND,
               base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}));

  return std::make_unique<VideoDecodeStatsDBImpl>(std::move(inner_db), db_dir_);
}

VideoDecodeStatsDBImpl::VideoDecodeStatsDBImpl(
    std::unique_ptr<leveldb_proto::ProtoDatabase<DecodeStatsProto>> db,
    const base::FilePath& db_dir)
    : db_(std::move(db)), db_dir_(db_dir), weak_ptr_factory_(this) {
  DCHECK(db_);
  DCHECK(!db_dir_.empty());
}

VideoDecodeStatsDBImpl::~VideoDecodeStatsDBImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void VideoDecodeStatsDBImpl::Initialize(
    base::OnceCallback<void(bool)> init_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(init_cb);
  DCHECK(!IsInitialized());
  DCHECK(!db_destroy_pending_);

  // "Simple options" will use the default global cache of 8MB. In the worst
  // case our whole DB will be less than 35K, so we aren't worried about
  // spamming the cache.
  // TODO(chcunningham): Keep an eye on the size as the table evolves.
  db_->Init(kDatabaseClientName, db_dir_, leveldb_proto::CreateSimpleOptions(),
            base::BindOnce(&VideoDecodeStatsDBImpl::OnInit,
                           weak_ptr_factory_.GetWeakPtr(), std::move(init_cb)));
}

void VideoDecodeStatsDBImpl::OnInit(base::OnceCallback<void(bool)> init_cb,
                                    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << __func__ << (success ? " succeeded" : " FAILED!");
  UMA_HISTOGRAM_BOOLEAN("Media.VideoDecodeStatsDB.OpSuccess.Initialize",
                        success);

  db_init_ = true;

  // Can't use DB when initialization fails.
  if (!success)
    db_.reset();

  std::move(init_cb).Run(success);
}

bool VideoDecodeStatsDBImpl::IsInitialized() {
  // |db_| will be null if Initialization failed.
  return db_init_ && db_;
}

void VideoDecodeStatsDBImpl::AppendDecodeStats(
    const VideoDescKey& key,
    const DecodeStatsEntry& entry,
    AppendDecodeStatsCB append_done_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());

  DVLOG(3) << __func__ << " Reading key " << KeyToString(key)
           << " from DB with intent to update with " << EntryToString(entry);

  db_->GetEntry(SerializeKey(key),
                base::BindOnce(&VideoDecodeStatsDBImpl::WriteUpdatedEntry,
                               weak_ptr_factory_.GetWeakPtr(), key, entry,
                               std::move(append_done_cb)));
}

void VideoDecodeStatsDBImpl::GetDecodeStats(const VideoDescKey& key,
                                            GetDecodeStatsCB get_stats_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());

  DVLOG(3) << __func__ << " " << KeyToString(key);

  db_->GetEntry(
      SerializeKey(key),
      base::BindOnce(&VideoDecodeStatsDBImpl::OnGotDecodeStats,
                     weak_ptr_factory_.GetWeakPtr(), std::move(get_stats_cb)));
}

void VideoDecodeStatsDBImpl::WriteUpdatedEntry(
    const VideoDescKey& key,
    const DecodeStatsEntry& entry,
    AppendDecodeStatsCB append_done_cb,
    bool read_success,
    std::unique_ptr<DecodeStatsProto> prev_stats_proto) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());

  // Note: outcome of "Write" operation logged in OnEntryUpdated().
  UMA_HISTOGRAM_BOOLEAN("Media.VideoDecodeStatsDB.OpSuccess.Read",
                        read_success);

  if (!read_success) {
    DVLOG(2) << __func__ << " FAILED DB read for " << KeyToString(key)
             << "; ignoring update!";
    std::move(append_done_cb).Run(false);
    return;
  }

  if (!prev_stats_proto) {
    prev_stats_proto.reset(new DecodeStatsProto());
    prev_stats_proto->set_frames_decoded(0);
    prev_stats_proto->set_frames_dropped(0);
    prev_stats_proto->set_frames_decoded_power_efficient(0);
  }

  uint64_t sum_frames_decoded =
      prev_stats_proto->frames_decoded() + entry.frames_decoded;
  uint64_t sum_frames_dropped =
      prev_stats_proto->frames_dropped() + entry.frames_dropped;
  uint64_t sum_frames_decoded_power_efficient =
      prev_stats_proto->frames_decoded_power_efficient() +
      entry.frames_decoded_power_efficient;

  prev_stats_proto->set_frames_decoded(sum_frames_decoded);
  prev_stats_proto->set_frames_dropped(sum_frames_dropped);
  prev_stats_proto->set_frames_decoded_power_efficient(
      sum_frames_decoded_power_efficient);

  DVLOG(3) << __func__ << " Updating " << KeyToString(key) << " with "
           << EntryToString(entry) << " aggregate:"
           << EntryToString(
                  DecodeStatsEntry(sum_frames_decoded, sum_frames_dropped,
                                   sum_frames_decoded_power_efficient));

  using ProtoDecodeStatsEntry = leveldb_proto::ProtoDatabase<DecodeStatsProto>;
  std::unique_ptr<ProtoDecodeStatsEntry::KeyEntryVector> entries =
      std::make_unique<ProtoDecodeStatsEntry::KeyEntryVector>();

  entries->emplace_back(SerializeKey(key), *prev_stats_proto);

  db_->UpdateEntries(std::move(entries),
                     std::make_unique<leveldb_proto::KeyVector>(),
                     base::BindOnce(&VideoDecodeStatsDBImpl::OnEntryUpdated,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    std::move(append_done_cb)));
}

void VideoDecodeStatsDBImpl::OnEntryUpdated(AppendDecodeStatsCB append_done_cb,
                                            bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UMA_HISTOGRAM_BOOLEAN("Media.VideoDecodeStatsDB.OpSuccess.Write", success);
  DVLOG(3) << __func__ << " update " << (success ? "succeeded" : "FAILED!");
  std::move(append_done_cb).Run(success);
}

void VideoDecodeStatsDBImpl::OnGotDecodeStats(
    GetDecodeStatsCB get_stats_cb,
    bool success,
    std::unique_ptr<DecodeStatsProto> stats_proto) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UMA_HISTOGRAM_BOOLEAN("Media.VideoDecodeStatsDB.OpSuccess.Read", success);

  std::unique_ptr<DecodeStatsEntry> entry;
  if (stats_proto) {
    DCHECK(success);
    entry = std::make_unique<DecodeStatsEntry>(
        stats_proto->frames_decoded(), stats_proto->frames_dropped(),
        stats_proto->frames_decoded_power_efficient());
  }

  DVLOG(3) << __func__ << " read " << (success ? "succeeded" : "FAILED!")
           << " entry: " << (entry ? EntryToString(*entry) : "nullptr");

  std::move(get_stats_cb).Run(success, std::move(entry));
}

void VideoDecodeStatsDBImpl::DestroyStats(base::OnceClosure destroy_done_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << __func__;

  // DB is no longer initialized once destruction kicks off.
  db_init_ = false;
  db_destroy_pending_ = true;

  db_->Destroy(base::BindOnce(&VideoDecodeStatsDBImpl::OnDestroyedStats,
                              weak_ptr_factory_.GetWeakPtr(),
                              std::move(destroy_done_cb)));
}

void VideoDecodeStatsDBImpl::OnDestroyedStats(base::OnceClosure destroy_done_cb,
                                              bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << __func__ << (success ? " succeeded" : " FAILED!");

  UMA_HISTOGRAM_BOOLEAN("Media.VideoDecodeStatsDB.OpSuccess.Destroy", success);

  // Allow calls to re-Intialize() now that destruction is complete.
  DCHECK(!db_init_);
  db_destroy_pending_ = false;

  // We don't pass success to |destroy_done_cb|. Clearing is best effort and
  // there is no additional action for callers to take in case of failure.
  // TODO(chcunningham): Monitor UMA and consider more aggressive action like
  // deleting the DB directory.
  std::move(destroy_done_cb).Run();
}

}  // namespace media
