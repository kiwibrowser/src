// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/dictionary_data_store.h"

#include <set>
#include <utility>

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_string_value_serializer.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/lock.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"

namespace app_list {

#if DCHECK_IS_ON()

namespace {

class ThreadSafeFilePathSet {
 public:
  ThreadSafeFilePathSet() = default;

  // Adds |file_path| to the set. DCHECKs if it is already in the set.
  void AddPath(const base::FilePath& file_path) {
    base::AutoLock auto_lock(lock_);
    auto insert_result = file_paths_.insert(file_path);
    DCHECK(insert_result.second) << "There can only be one active "
                                    "DictionaryDataStore per file at a time.";
  }

  // Removes |file_path| from the set.
  void RemovePath(const base::FilePath& file_path) {
    base::AutoLock auto_lock(lock_);
    file_paths_.erase(file_path);
  }

 private:
  base::Lock lock_;
  std::set<base::FilePath> file_paths_;

  DISALLOW_COPY_AND_ASSIGN(ThreadSafeFilePathSet);
};

base::LazyInstance<ThreadSafeFilePathSet>::Leaky
    g_file_paths_with_active_dictionary_data_store = LAZY_INSTANCE_INITIALIZER;

}  // namespace

#endif  // DCHECK_IS_ON()

DictionaryDataStore::DictionaryDataStore(const base::FilePath& data_file)
    : data_file_(data_file),
      // Uses a SKIP_ON_SHUTDOWN task runner because losing a couple
      // associations is better than blocking shutdown.
      file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      writer_(data_file, file_task_runner_),
      cached_dict_(std::make_unique<base::DictionaryValue>()) {
#if DCHECK_IS_ON()
  g_file_paths_with_active_dictionary_data_store.Get().AddPath(data_file_);
#endif
}

DictionaryDataStore::~DictionaryDataStore() {
  Flush(OnFlushedCallback());

#if DCHECK_IS_ON()
  g_file_paths_with_active_dictionary_data_store.Get().RemovePath(data_file_);
#endif
}

void DictionaryDataStore::Flush(OnFlushedCallback on_flushed) {
  if (writer_.HasPendingWrite())
    writer_.DoScheduledWrite();

  if (on_flushed.is_null())
    return;

  file_task_runner_->PostTaskAndReply(FROM_HERE, base::DoNothing(),
                                      std::move(on_flushed));
}

void DictionaryDataStore::Load(
    DictionaryDataStore::OnLoadedCallback on_loaded) {
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&DictionaryDataStore::LoadAsync, this),
      std::move(on_loaded));
}

void DictionaryDataStore::ScheduleWrite() {
  writer_.ScheduleWrite(this);
}

std::unique_ptr<base::DictionaryValue> DictionaryDataStore::LoadAsync() {
  DCHECK(file_task_runner_->RunsTasksInCurrentSequence());

  int error_code = JSONFileValueDeserializer::JSON_NO_ERROR;
  std::string error_message;
  JSONFileValueDeserializer deserializer(data_file_);
  std::unique_ptr<base::DictionaryValue> dict_value =
      base::DictionaryValue::From(
          deserializer.Deserialize(&error_code, &error_message));
  if (error_code != JSONFileValueDeserializer::JSON_NO_ERROR || !dict_value) {
    return nullptr;
  }

  std::unique_ptr<base::DictionaryValue> return_dict =
      dict_value->CreateDeepCopy();
  cached_dict_ = std::move(dict_value);
  return return_dict;
}

bool DictionaryDataStore::SerializeData(std::string* data) {
  JSONStringValueSerializer serializer(data);
  serializer.set_pretty_print(true);
  return serializer.Serialize(*cached_dict_.get());
}

}  // namespace app_list
