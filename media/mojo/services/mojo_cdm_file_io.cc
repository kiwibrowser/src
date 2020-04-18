// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_cdm_file_io.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace media {

namespace {

using ClientStatus = cdm::FileIOClient::Status;
using StorageStatus = media::mojom::CdmStorage::Status;

// File size limit is 32MB. Licenses saved by the CDM are typically several
// hundreds of bytes.
const int64_t kMaxFileSizeBytes = 32 * 1024 * 1024;

// Maximum length of a file name.
const size_t kFileNameMaxLength = 256;

// File names must only contain letters (A-Za-z), digits(0-9), or "._-",
// and not start with "_". It must contain at least 1 character, and not
// more then |kFileNameMaxLength| characters.
bool IsValidFileName(const std::string& name) {
  if (name.empty() || name.length() > kFileNameMaxLength || name[0] == '_')
    return false;

  for (auto ch : name) {
    if (!base::IsAsciiAlpha(ch) && !base::IsAsciiDigit(ch) && ch != '.' &&
        ch != '_' && ch != '-') {
      return false;
    }
  }

  return true;
}

}  // namespace

MojoCdmFileIO::MojoCdmFileIO(Delegate* delegate,
                             cdm::FileIOClient* client,
                             mojom::CdmStorage* cdm_storage)
    : delegate_(delegate),
      client_(client),
      cdm_storage_(cdm_storage),
      weak_factory_(this) {
  DVLOG(1) << __func__;
  DCHECK(delegate_);
  DCHECK(client_);
  DCHECK(cdm_storage_);
}

MojoCdmFileIO::~MojoCdmFileIO() {
  DVLOG(1) << __func__;
}

void MojoCdmFileIO::Open(const char* file_name, uint32_t file_name_size) {
  std::string file_name_string(file_name, file_name_size);
  DVLOG(3) << __func__ << " file: " << file_name_string;

  TRACE_EVENT1("media", "MojoCdmFileIO::Open", "file_name", file_name_string);

  // Open is only allowed if the current state is kUnopened and the file name
  // is valid.
  if (state_ != State::kUnopened) {
    OnError(ErrorType::kOpenError);
    return;
  }

  if (!IsValidFileName(file_name_string)) {
    OnError(ErrorType::kOpenError);
    return;
  }

  state_ = State::kOpening;
  file_name_ = file_name_string;

  auto callback = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      base::BindOnce(&MojoCdmFileIO::OnFileOpened, weak_factory_.GetWeakPtr()),
      StorageStatus::kFailure, base::File(), nullptr);
  cdm_storage_->Open(file_name_string, std::move(callback));
}

void MojoCdmFileIO::OnFileOpened(StorageStatus status,
                                 base::File file,
                                 mojom::CdmFileAssociatedPtrInfo cdm_file) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", status: " << status;

  TRACE_EVENT2("media", "MojoCdmFileIO::FileOpened", "file_name", file_name_,
               "status", static_cast<int32_t>(status));

  switch (status) {
    case StorageStatus::kSuccess:
      if (!file.IsValid()) {
        NOTREACHED() << "File received is invalid.";
        state_ = State::kError;
        OnError(ErrorType::kOpenError);
        return;
      }

      // File was successfully opened.
      state_ = State::kOpened;
      file_for_reading_ = std::move(file);
      cdm_file_.Bind(std::move(cdm_file));
      client_->OnOpenComplete(ClientStatus::kSuccess);
      return;
    case StorageStatus::kInUse:
      // File already open by somebody else.
      state_ = State::kUnopened;
      OnError(ErrorType::kOpenInUse);
      return;
    case StorageStatus::kFailure:
      // Something went wrong.
      state_ = State::kError;
      OnError(ErrorType::kOpenError);
      return;
  }

  NOTREACHED();
}

void MojoCdmFileIO::Read() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  TRACE_EVENT1("media", "MojoCdmFileIO::Read", "file_name", file_name_);

  // If another operation is in progress, fail.
  if (state_ == State::kReading || state_ == State::kWriting) {
    OnError(ErrorType::kReadInUse);
    return;
  }

  // If the file is not open, fail.
  if (state_ != State::kOpened) {
    OnError(ErrorType::kReadError);
    return;
  }

  // Determine the size of the file (so we know how many bytes to read).
  int64_t num_bytes = file_for_reading_.GetLength();
  if (num_bytes < 0) {
    // Negative bytes mean failure, so fail. This error is not recoverable,
    // so don't allow any more operations other than close on the file.
    state_ = State::kError;
    OnError(ErrorType::kReadError);
    return;
  }

  // Files are limited to 32MB, so fail if file too big. Setting |state_|
  // back to Opened so that the CDM could write something valid to this file.
  if (num_bytes > kMaxFileSizeBytes) {
    DLOG(WARNING) << __func__
                  << " Too much data to read. #bytes = " << num_bytes;
    OnError(ErrorType::kReadError);
    return;
  }

  // Do the actual read asynchronously so that we don't need to copy the
  // data when calling |client_|.
  state_ = State::kReading;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MojoCdmFileIO::DoRead,
                                weak_factory_.GetWeakPtr(), num_bytes));
}

void MojoCdmFileIO::DoRead(int64_t num_bytes) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  DCHECK_EQ(State::kReading, state_);

  TRACE_EVENT2("media", "MojoCdmFileIO::DoRead", "file_name", file_name_,
               "bytes_to_read", num_bytes);

  // We know how much data is available, so read the complete contents of the
  // file into a buffer and passing it back to |client_|. As these should be
  // small files, we don't worry about breaking it up into chunks to read it.

  // Read the contents of the file. Read() sizes (provided and returned) are
  // type int, so cast appropriately.
  int bytes_to_read = base::checked_cast<int>(num_bytes);
  std::vector<uint8_t> buffer(bytes_to_read);

  // If the file has 0 bytes, no need to read anything.
  if (bytes_to_read != 0) {
    TRACE_EVENT0("media", "MojoCdmFileIO::ActualRead");
    base::TimeTicks start = base::TimeTicks::Now();
    int bytes_read = file_for_reading_.Read(
        0, reinterpret_cast<char*>(buffer.data()), bytes_to_read);
    base::TimeDelta read_time = base::TimeTicks::Now() - start;
    if (bytes_to_read != bytes_read) {
      // Unable to read the contents of the file. Setting |state_| to kOpened
      // so that the CDM can write something valid to this file.
      DVLOG(1) << "Failed to read file " << file_name_ << ". Requested "
               << bytes_to_read << " bytes, got " << bytes_read;
      state_ = State::kOpened;
      OnError(ErrorType::kReadError);
      return;
    }

    // Only report reading time for successful reads.
    UMA_HISTOGRAM_TIMES("Media.EME.CdmFileIO.ReadTime", read_time);
  }

  // Call this before OnReadComplete() so that we always have the latest file
  // size before CDM fires errors.
  delegate_->ReportFileReadSize(bytes_to_read);

  state_ = State::kOpened;
  client_->OnReadComplete(ClientStatus::kSuccess, buffer.data(), buffer.size());
}

void MojoCdmFileIO::Write(const uint8_t* data, uint32_t data_size) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", bytes: " << data_size;

  TRACE_EVENT1("media", "MojoCdmFileIO::Write", "file_name", file_name_);

  // If another operation is in progress, fail.
  if (state_ == State::kReading || state_ == State::kWriting) {
    OnError(ErrorType::kWriteInUse);
    return;
  }

  // If the file is not open, fail.
  if (state_ != State::kOpened) {
    OnError(ErrorType::kWriteError);
    return;
  }

  // Files are limited to 32MB, so fail if file too big.
  if (data_size > kMaxFileSizeBytes) {
    DLOG(WARNING) << __func__
                  << " Too much data to write. #bytes = " << data_size;
    OnError(ErrorType::kWriteError);
    return;
  }

  // Now open a temporary file for writing. Close |file_for_reading_| as it
  // won't be used again.
  state_ = State::kWriting;
  file_for_reading_.Close();
  cdm_file_->OpenFileForWriting(
      base::BindOnce(&MojoCdmFileIO::DoWrite, weak_factory_.GetWeakPtr(),
                     std::vector<uint8_t>(data, data + data_size)));
}

void MojoCdmFileIO::DoWrite(const std::vector<uint8_t>& data,
                            base::File temporary_file) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", result: "
           << base::File::ErrorToString(temporary_file.error_details());
  DCHECK_EQ(State::kWriting, state_);

  TRACE_EVENT2("media", "MojoCdmFileIO::DoWrite", "file_name", file_name_,
               "bytes_to_write", data.size());

  if (!temporary_file.IsValid()) {
    // Failed to open temporary file.
    state_ = State::kError;
    OnError(ErrorType::kWriteError);
    return;
  }

  // As the temporary file should have been newly created, it should be empty.
  // No need to call write() if there is no data.
  CHECK_EQ(0u, temporary_file.GetLength()) << "Temporary file is not empty.";
  int bytes_to_write = base::checked_cast<int>(data.size());
  if (bytes_to_write > 0) {
    TRACE_EVENT0("media", "MojoCdmFileIO::ActualWrite");
    base::TimeTicks start = base::TimeTicks::Now();
    int bytes_written = temporary_file.Write(
        0, reinterpret_cast<const char*>(data.data()), bytes_to_write);
    base::TimeDelta write_time = base::TimeTicks::Now() - start;
    if (bytes_written != bytes_to_write) {
      // Failed to write to the temporary file.
      state_ = State::kError;
      OnError(ErrorType::kWriteError);
      return;
    }

    // Only report writing time for successful writes.
    UMA_HISTOGRAM_TIMES("Media.EME.CdmFileIO.WriteTime", write_time);
  }

  // Close the temporary file returned before renaming. Original file was
  // closed previously.
  temporary_file.Close();
  DCHECK(!file_for_reading_.IsValid()) << "Original file was not closed.";
  cdm_file_->CommitWrite(base::BindOnce(&MojoCdmFileIO::OnWriteCommitted,
                                        weak_factory_.GetWeakPtr()));
}

void MojoCdmFileIO::OnWriteCommitted(base::File reopened_file) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  DCHECK_EQ(State::kWriting, state_);
  DCHECK(!file_for_reading_.IsValid()) << "Original file was not closed.";

  TRACE_EVENT1("media", "MojoCdmFileIO::WriteDone", "file_name", file_name_);

  if (!reopened_file.IsValid()) {
    // Rename failed, and no file to use.
    state_ = State::kError;
    OnError(ErrorType::kWriteError);
    return;
  }

  state_ = State::kOpened;
  file_for_reading_ = std::move(reopened_file);
  client_->OnWriteComplete(ClientStatus::kSuccess);
}

void MojoCdmFileIO::Close() {
  DVLOG(3) << __func__ << " file: " << file_name_;

  // Note: |this| could be deleted as part of this call.
  delegate_->CloseCdmFileIO(this);
}

void MojoCdmFileIO::OnError(ErrorType error) {
  DVLOG(3) << __func__ << " file: " << file_name_ << ", error: " << (int)error;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MojoCdmFileIO::NotifyClientOfError,
                                weak_factory_.GetWeakPtr(), error));
}

void MojoCdmFileIO::NotifyClientOfError(ErrorType error) {
  switch (error) {
    case ErrorType::kOpenError:
      client_->OnOpenComplete(ClientStatus::kError);
      break;
    case ErrorType::kOpenInUse:
      client_->OnOpenComplete(ClientStatus::kInUse);
      break;
    case ErrorType::kReadError:
      client_->OnReadComplete(ClientStatus::kError, nullptr, 0);
      break;
    case ErrorType::kReadInUse:
      client_->OnReadComplete(ClientStatus::kInUse, nullptr, 0);
      break;
    case ErrorType::kWriteError:
      client_->OnWriteComplete(ClientStatus::kError);
      break;
    case ErrorType::kWriteInUse:
      client_->OnWriteComplete(ClientStatus::kInUse);
      break;
  }
}

}  // namespace media
