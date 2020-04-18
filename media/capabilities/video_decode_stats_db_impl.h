// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPABILITIES_VIDEO_DECODE_STATS_DB_IMPL_H_
#define MEDIA_CAPABILITIES_VIDEO_DECODE_STATS_DB_IMPL_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "components/leveldb_proto/proto_database.h"
#include "media/base/media_export.h"
#include "media/base/video_codecs.h"
#include "media/capabilities/video_decode_stats_db.h"
#include "ui/gfx/geometry/size.h"

namespace base {
class FilePath;
}  // namespace base

namespace media {

class DecodeStatsProto;

// Factory interface to create a DB instance.
class MEDIA_EXPORT VideoDecodeStatsDBImplFactory
    : public VideoDecodeStatsDBFactory {
 public:
  explicit VideoDecodeStatsDBImplFactory(base::FilePath db_dir);
  ~VideoDecodeStatsDBImplFactory() override;
  std::unique_ptr<VideoDecodeStatsDB> CreateDB() override;

 private:
  base::FilePath db_dir_;

  DISALLOW_COPY_AND_ASSIGN(VideoDecodeStatsDBImplFactory);
};

// LevelDB implementation of VideoDecodeStatsDB. This class is not
// thread safe. All API calls should happen on the same sequence used for
// construction. API callbacks will also occur on this sequence.
class MEDIA_EXPORT VideoDecodeStatsDBImpl : public VideoDecodeStatsDB {
 public:
  // Constructs the database. NOTE: must call Initialize() before using.
  // |db| injects the level_db database instance for storing capabilities info.
  // |dir| specifies where to store LevelDB files to disk. LevelDB generates a
  // handful of files, so its recommended to provide a dedicated directory to
  // keep them isolated.
  VideoDecodeStatsDBImpl(
      std::unique_ptr<leveldb_proto::ProtoDatabase<DecodeStatsProto>> db,
      const base::FilePath& dir);
  ~VideoDecodeStatsDBImpl() override;

  // Implement VideoDecodeStatsDB.
  void Initialize(base::OnceCallback<void(bool)> init_cb) override;
  void AppendDecodeStats(const VideoDescKey& key,
                         const DecodeStatsEntry& entry,
                         AppendDecodeStatsCB append_done_cb) override;
  void GetDecodeStats(const VideoDescKey& key,
                      GetDecodeStatsCB get_stats_cb) override;
  void DestroyStats(base::OnceClosure destroy_done_cb) override;

 private:
  friend class VideoDecodeStatsDBTest;

  // Called when the database has been initialized. Will immediately call
  // |init_cb| to forward |success|.
  void OnInit(base::OnceCallback<void(bool)> init_cb, bool success);

  // Returns true if the DB is successfully initialized.
  bool IsInitialized();

  // Passed as the callback for |OnGotDecodeStats| by |AppendDecodeStats| to
  // update the database once we've read the existing stats entry.
  void WriteUpdatedEntry(const VideoDescKey& key,
                         const DecodeStatsEntry& entry,
                         AppendDecodeStatsCB append_done_cb,
                         bool read_success,
                         std::unique_ptr<DecodeStatsProto> prev_stats_proto);

  // Called when the database has been modified after a call to
  // |WriteUpdatedEntry|. Will run |append_done_cb| when done.
  void OnEntryUpdated(AppendDecodeStatsCB append_done_cb, bool success);

  // Called when GetDecodeStats() operation was performed. |get_stats_cb|
  // will be run with |success| and a |DecodeStatsEntry| created from
  // |stats_proto| or nullptr if no entry was found for the requested key.
  void OnGotDecodeStats(
      GetDecodeStatsCB get_stats_cb,
      bool success,
      std::unique_ptr<DecodeStatsProto> capabilities_info_proto);

  // Internal callback for ClearStats that logs |success| and runs
  // |destroy_done_cb|
  void OnDestroyedStats(base::OnceClosure destroy_done_cb, bool success);

  // Indicates whether initialization is completed. Does not indicate whether it
  // was successful. Will be reset upon calling DestroyStats(). Failed
  // initialization is signaled by setting |db_| to null.
  bool db_init_ = false;

  // Tracks whether db_->Destroy() is in progress. Used to assert that
  // Initialize() is not called until db destruction is complete.
  bool db_destroy_pending_ = false;

  // ProtoDatabase instance. Set to nullptr if fatal database error is
  // encountered.
  std::unique_ptr<leveldb_proto::ProtoDatabase<DecodeStatsProto>> db_;

  // Directory where levelDB should store database files.
  base::FilePath db_dir_;

  // Ensures all access to class members come on the same sequence. API calls
  // and callbacks should occur on the same sequence used during construction.
  // LevelDB operations happen on a separate task runner, but all LevelDB
  // callbacks to this happen on the checked sequence.
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<VideoDecodeStatsDBImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoDecodeStatsDBImpl);
};

}  // namespace media

#endif  // MEDIA_CAPABILITIES_VIDEO_DECODE_STATS_DB_IMPL_H_
