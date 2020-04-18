// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/data_source.h"

#include "base/posix/eintr_wrapper.h"
#include "base/task_scheduler/post_task.h"
#include "components/exo/data_source_delegate.h"
#include "components/exo/data_source_observer.h"

namespace exo {

namespace {
constexpr char kTextMimeTypeUtf8[] = "text/plain;charset=utf-8";

std::vector<uint8_t> ReadDataOnWorkerThread(base::ScopedFD fd) {
  constexpr size_t kChunkSize = 1024;
  std::vector<uint8_t> bytes;
  while (true) {
    uint8_t chunk[kChunkSize];
    ssize_t bytes_read = HANDLE_EINTR(read(fd.get(), chunk, kChunkSize));
    if (bytes_read > 0) {
      bytes.insert(bytes.end(), chunk, chunk + bytes_read);
      continue;
    }
    if (!bytes_read)
      return bytes;
    if (bytes_read < 0) {
      PLOG(ERROR) << "Failed to read selection data from clipboard";
      return std::vector<uint8_t>();
    }
  }
}

void CreatePipe(base::ScopedFD* read_pipe, base::ScopedFD* write_pipe) {
  int raw_pipe[2];
  PCHECK(0 == pipe(raw_pipe));
  read_pipe->reset(raw_pipe[0]);
  write_pipe->reset(raw_pipe[1]);
}
}  // namespace

ScopedDataSource::ScopedDataSource(DataSource* data_source,
                                   DataSourceObserver* observer)
    : data_source_(data_source), observer_(observer) {
  data_source_->AddObserver(observer_);
}

ScopedDataSource::~ScopedDataSource() {
  data_source_->RemoveObserver(observer_);
}

DataSource::DataSource(DataSourceDelegate* delegate)
    : delegate_(delegate),
      cancelled_(false),
      read_data_weak_ptr_factory_(this) {}

DataSource::~DataSource() {
  delegate_->OnDataSourceDestroying(this);
  for (DataSourceObserver& observer : observers_) {
    observer.OnDataSourceDestroying(this);
  }
}

void DataSource::AddObserver(DataSourceObserver* observer) {
  observers_.AddObserver(observer);
}

void DataSource::RemoveObserver(DataSourceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void DataSource::Offer(const std::string& mime_type) {
  mime_types_.insert(mime_type);
}

void DataSource::SetActions(const base::flat_set<DndAction>& dnd_actions) {
  NOTIMPLEMENTED();
}

void DataSource::Cancelled() {
  cancelled_ = true;
  read_data_weak_ptr_factory_.InvalidateWeakPtrs();
  delegate_->OnCancelled();
}

void DataSource::ReadData(ReadDataCallback callback) {
  // This DataSource does not contain text data.
  if (!mime_types_.count(kTextMimeTypeUtf8) || cancelled_)
    return;

  base::ScopedFD read_fd;
  base::ScopedFD write_fd;
  CreatePipe(&read_fd, &write_fd);
  delegate_->OnSend(kTextMimeTypeUtf8, std::move(write_fd));

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&ReadDataOnWorkerThread, std::move(read_fd)),
      base::BindOnce(&DataSource::OnDataRead,
                     read_data_weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void DataSource::OnDataRead(ReadDataCallback callback,
                            const std::vector<uint8_t>& data) {
  std::move(callback).Run(data);
}

}  // namespace exo
