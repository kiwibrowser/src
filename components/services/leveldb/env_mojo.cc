// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/leveldb/env_mojo.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/trace_event/trace_event.h"
#include "third_party/leveldatabase/chromium_logger.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

using leveldb_env::UMALogger;

namespace leveldb {

namespace {

const base::FilePath::CharType table_extension[] = FILE_PATH_LITERAL(".ldb");

Status FilesystemErrorToStatus(base::File::Error error,
                               const std::string& filename,
                               leveldb_env::MethodID method) {
  if (error == base::File::Error::FILE_OK)
    return Status::OK();

  std::string err_str = base::File::ErrorToString(error);

  char buf[512];
  snprintf(buf, sizeof(buf), "%s (MojoFSError: %d::%s)", err_str.c_str(),
           method, MethodIDToString(method));

  // TOOD(https://crbug.com/760362): Map base::File::Error::NOT_FOUND to
  // Status::NotFound, after fixing LevelDB to handle the NotFound correctly.
  return Status::IOError(filename, buf);
}

class MojoFileLock : public FileLock {
 public:
  MojoFileLock(LevelDBMojoProxy::OpaqueLock* lock, const std::string& name)
      : fname_(name), lock_(lock) {}
  ~MojoFileLock() override { DCHECK(!lock_); }

  const std::string& name() const { return fname_; }

  LevelDBMojoProxy::OpaqueLock* TakeLock() {
    LevelDBMojoProxy::OpaqueLock* to_return = lock_;
    lock_ = nullptr;
    return to_return;
  }

 private:
  std::string fname_;
  LevelDBMojoProxy::OpaqueLock* lock_;
};

class MojoSequentialFile : public leveldb::SequentialFile {
 public:
  MojoSequentialFile(const std::string& fname,
                     base::File f,
                     const UMALogger* uma_logger)
      : filename_(fname), file_(std::move(f)), uma_logger_(uma_logger) {}
  ~MojoSequentialFile() override {}

  Status Read(size_t n, Slice* result, char* scratch) override {
    int bytes_read =
        file_.ReadAtCurrentPosNoBestEffort(scratch, static_cast<int>(n));
    if (bytes_read == -1) {
      base::File::Error error = base::File::GetLastFileError();
      uma_logger_->RecordOSError(leveldb_env::kSequentialFileRead, error);
      return MakeIOError(filename_, base::File::ErrorToString(error),
                         leveldb_env::kSequentialFileRead, error);
    }
    if (bytes_read > 0)
      uma_logger_->RecordBytesRead(bytes_read);
    *result = Slice(scratch, bytes_read);
    return Status::OK();
  }

  Status Skip(uint64_t n) override {
    if (file_.Seek(base::File::FROM_CURRENT, n) == -1) {
      base::File::Error error = base::File::GetLastFileError();
      uma_logger_->RecordOSError(leveldb_env::kSequentialFileSkip, error);
      return MakeIOError(filename_, base::File::ErrorToString(error),
                         leveldb_env::kSequentialFileSkip, error);
    }
    return Status::OK();
  }

 private:
  std::string filename_;
  base::File file_;
  const UMALogger* uma_logger_;

  DISALLOW_COPY_AND_ASSIGN(MojoSequentialFile);
};

class MojoRandomAccessFile : public leveldb::RandomAccessFile {
 public:
  MojoRandomAccessFile(const std::string& fname,
                       base::File file,
                       const UMALogger* uma_logger)
      : filename_(fname), file_(std::move(file)), uma_logger_(uma_logger) {}
  ~MojoRandomAccessFile() override {}

  Status Read(uint64_t offset,
              size_t n,
              Slice* result,
              char* scratch) const override {
    int bytes_read = file_.Read(offset, scratch, static_cast<int>(n));
    *result = Slice(scratch, (bytes_read < 0) ? 0 : bytes_read);
    if (bytes_read < 0) {
      uma_logger_->RecordOSError(leveldb_env::kRandomAccessFileRead,
                                 base::File::GetLastFileError());
      return MakeIOError(filename_, "Could not perform read",
                         leveldb_env::kRandomAccessFileRead);
    }
    if (bytes_read > 0)
      uma_logger_->RecordBytesRead(bytes_read);
    return Status::OK();
  }

 private:
  std::string filename_;
  mutable base::File file_;
  const UMALogger* uma_logger_;

  DISALLOW_COPY_AND_ASSIGN(MojoRandomAccessFile);
};

class MojoWritableFile : public leveldb::WritableFile {
 public:
  MojoWritableFile(LevelDBMojoProxy::OpaqueDir* dir,
                   const std::string& fname,
                   base::File f,
                   scoped_refptr<LevelDBMojoProxy> thread,
                   const UMALogger* uma_logger)
      : filename_(fname),
        file_(std::move(f)),
        file_type_(kOther),
        dir_(dir),
        thread_(thread),
        uma_logger_(uma_logger) {
    base::FilePath path = base::FilePath::FromUTF8Unsafe(fname);
    if (base::StartsWith(path.BaseName().AsUTF8Unsafe(), "MANIFEST",
                         base::CompareCase::SENSITIVE)) {
      file_type_ = kManifest;
    } else if (path.MatchesExtension(table_extension)) {
      file_type_ = kTable;
    }
    parent_dir_ =
        base::FilePath::FromUTF8Unsafe(fname).DirName().AsUTF8Unsafe();
  }

  ~MojoWritableFile() override {}

  leveldb::Status Append(const leveldb::Slice& data) override {
    int bytes_written =
        file_.WriteAtCurrentPos(data.data(), static_cast<int>(data.size()));
    if (bytes_written != static_cast<int>(data.size())) {
      base::File::Error error = base::File::GetLastFileError();
      uma_logger_->RecordOSError(leveldb_env::kWritableFileAppend, error);
      return MakeIOError(filename_, base::File::ErrorToString(error),
                         leveldb_env::kWritableFileAppend, error);
    }
    if (bytes_written > 0)
      uma_logger_->RecordBytesWritten(bytes_written);
    return Status::OK();
  }

  leveldb::Status Close() override {
    file_.Close();
    return Status::OK();
  }

  leveldb::Status Flush() override {
    // base::File doesn't do buffered I/O (i.e. POSIX FILE streams) so nothing
    // to flush.
    return Status::OK();
  }

  leveldb::Status Sync() override {
    TRACE_EVENT0("leveldb", "MojoWritableFile::Sync");

    if (!file_.Flush()) {
      base::File::Error error = base::File::GetLastFileError();
      uma_logger_->RecordOSError(leveldb_env::kWritableFileSync, error);
      return MakeIOError(filename_, base::File::ErrorToString(error),
                         leveldb_env::kWritableFileSync, error);
    }

    // leveldb's implicit contract for Sync() is that if this instance is for a
    // manifest file then the directory is also sync'ed. See leveldb's
    // env_posix.cc.
    if (file_type_ == kManifest)
      return SyncParent();

    return Status::OK();
  }

 private:
  enum Type { kManifest, kTable, kOther };

  leveldb::Status SyncParent() {
    base::File::Error error = thread_->SyncDirectory(dir_, parent_dir_);
    if (error != base::File::Error::FILE_OK) {
      uma_logger_->RecordOSError(leveldb_env::kSyncParent,
                                 static_cast<base::File::Error>(error));
    }
    return error == base::File::Error::FILE_OK
               ? Status::OK()
               : Status::IOError(filename_,
                                 base::File::ErrorToString(base::File::Error(
                                     static_cast<int>(error))));
  }

  std::string filename_;
  base::File file_;
  Type file_type_;
  LevelDBMojoProxy::OpaqueDir* dir_;
  std::string parent_dir_;
  scoped_refptr<LevelDBMojoProxy> thread_;
  const UMALogger* uma_logger_;

  DISALLOW_COPY_AND_ASSIGN(MojoWritableFile);
};

class Thread : public base::PlatformThread::Delegate {
 public:
  Thread(void (*function)(void* arg), void* arg)
      : function_(function), arg_(arg) {
    base::PlatformThreadHandle handle;
    bool success = base::PlatformThread::Create(0, this, &handle);
    DCHECK(success);
  }
  ~Thread() override {}
  void ThreadMain() override {
    (*function_)(arg_);
    delete this;
  }

 private:
  void (*function_)(void* arg);
  void* arg_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

class Retrier {
 public:
  Retrier(leveldb_env::MethodID method, MojoRetrierProvider* provider)
      : start_(base::TimeTicks::Now()),
        limit_(start_ + base::TimeDelta::FromMilliseconds(
                            provider->MaxRetryTimeMillis())),
        last_(start_),
        time_to_sleep_(base::TimeDelta::FromMilliseconds(10)),
        success_(true),
        method_(method),
        last_error_(base::File::FILE_OK),
        provider_(provider) {}

  ~Retrier() {
    if (success_) {
      provider_->RecordRetryTime(method_, last_ - start_);
      if (last_error_ != base::File::FILE_OK) {
        DCHECK_LT(last_error_, 0);
        provider_->RecordRecoveredFromError(method_, last_error_);
      }
    }
  }

  bool ShouldKeepTrying(base::File::Error last_error) {
    DCHECK_NE(last_error, base::File::FILE_OK);
    last_error_ = last_error;
    if (last_ < limit_) {
      base::PlatformThread::Sleep(time_to_sleep_);
      last_ = base::TimeTicks::Now();
      return true;
    }
    success_ = false;
    return false;
  }

 private:
  base::TimeTicks start_;
  base::TimeTicks limit_;
  base::TimeTicks last_;
  base::TimeDelta time_to_sleep_;
  bool success_;
  leveldb_env::MethodID method_;
  base::File::Error last_error_;
  MojoRetrierProvider* provider_;

  DISALLOW_COPY_AND_ASSIGN(Retrier);
};

}  // namespace

MojoEnv::MojoEnv(scoped_refptr<LevelDBMojoProxy> file_thread,
                 LevelDBMojoProxy::OpaqueDir* dir)
    : thread_(file_thread), dir_(dir) {}

MojoEnv::~MojoEnv() {
  thread_->UnregisterDirectory(dir_);
}

Status MojoEnv::NewSequentialFile(const std::string& fname,
                                  SequentialFile** result) {
  TRACE_EVENT1("leveldb", "MojoEnv::NewSequentialFile", "fname", fname);
  base::File f = thread_->OpenFileHandle(
      dir_, fname, filesystem::mojom::kFlagOpen | filesystem::mojom::kFlagRead);
  if (!f.IsValid()) {
    *result = nullptr;
    RecordOSError(leveldb_env::kNewSequentialFile, f.error_details());
    return MakeIOError(fname, "Unable to create sequential file",
                       leveldb_env::kNewSequentialFile, f.error_details());
  }

  *result = new MojoSequentialFile(fname, std::move(f), this);
  return Status::OK();
}

Status MojoEnv::NewRandomAccessFile(const std::string& fname,
                                    RandomAccessFile** result) {
  TRACE_EVENT1("leveldb", "MojoEnv::NewRandomAccessFile", "fname", fname);
  base::File f = thread_->OpenFileHandle(
      dir_, fname, filesystem::mojom::kFlagRead | filesystem::mojom::kFlagOpen);
  if (!f.IsValid()) {
    *result = nullptr;
    base::File::Error error_code = f.error_details();
    RecordOSError(leveldb_env::kNewRandomAccessFile, error_code);
    return MakeIOError(fname, base::File::ErrorToString(error_code),
                       leveldb_env::kNewRandomAccessFile, error_code);
  }

  *result = new MojoRandomAccessFile(fname, std::move(f), this);
  return Status::OK();
}

Status MojoEnv::NewWritableFile(const std::string& fname,
                                WritableFile** result) {
  TRACE_EVENT1("leveldb", "MojoEnv::NewWritableFile", "fname", fname);
  base::File f = thread_->OpenFileHandle(
      dir_, fname,
      filesystem::mojom::kCreateAlways | filesystem::mojom::kFlagWrite);
  if (!f.IsValid()) {
    *result = nullptr;
    RecordOSError(leveldb_env::kNewWritableFile, f.error_details());
    return MakeIOError(fname, "Unable to create writable file",
                       leveldb_env::kNewWritableFile, f.error_details());
  }

  *result = new MojoWritableFile(dir_, fname, std::move(f), thread_, this);
  return Status::OK();
}

Status MojoEnv::NewAppendableFile(const std::string& fname,
                                  WritableFile** result) {
  TRACE_EVENT1("leveldb", "MojoEnv::NewAppendableFile", "fname", fname);
  base::File f = thread_->OpenFileHandle(
      dir_, fname,
      filesystem::mojom::kFlagOpenAlways | filesystem::mojom::kFlagAppend);
  if (!f.IsValid()) {
    *result = nullptr;
    RecordOSError(leveldb_env::kNewAppendableFile, f.error_details());
    return MakeIOError(fname, "Unable to create appendable file",
                       leveldb_env::kNewAppendableFile, f.error_details());
  }

  *result = new MojoWritableFile(dir_, fname, std::move(f), thread_, this);
  return Status::OK();
}

bool MojoEnv::FileExists(const std::string& fname) {
  TRACE_EVENT1("leveldb", "MojoEnv::FileExists", "fname", fname);
  return thread_->FileExists(dir_, fname);
}

Status MojoEnv::GetChildren(const std::string& path,
                            std::vector<std::string>* result) {
  TRACE_EVENT1("leveldb", "MojoEnv::GetChildren", "path", path);
  base::File::Error error = thread_->GetChildren(dir_, path, result);
  if (error != base::File::Error::FILE_OK)
    RecordFileError(leveldb_env::kGetChildren, error);
  return FilesystemErrorToStatus(error, path, leveldb_env::kGetChildren);
}

Status MojoEnv::DeleteFile(const std::string& fname) {
  TRACE_EVENT1("leveldb", "MojoEnv::DeleteFile", "fname", fname);
  base::File::Error error = thread_->Delete(dir_, fname, 0);
  if (error != base::File::Error::FILE_OK)
    RecordFileError(leveldb_env::kDeleteFile, error);
  return FilesystemErrorToStatus(error, fname, leveldb_env::kDeleteFile);
}

Status MojoEnv::CreateDir(const std::string& dirname) {
  TRACE_EVENT1("leveldb", "MojoEnv::CreateDir", "dirname", dirname);
  Retrier retrier(leveldb_env::kCreateDir, this);
  base::File::Error error;
  do {
    error = thread_->CreateDir(dir_, dirname);
  } while (error != base::File::Error::FILE_OK &&
           retrier.ShouldKeepTrying(error));
  if (error != base::File::Error::FILE_OK)
    RecordFileError(leveldb_env::kCreateDir, error);
  return FilesystemErrorToStatus(error, dirname, leveldb_env::kCreateDir);
}

Status MojoEnv::DeleteDir(const std::string& dirname) {
  TRACE_EVENT1("leveldb", "MojoEnv::DeleteDir", "dirname", dirname);
  base::File::Error error =
      thread_->Delete(dir_, dirname, filesystem::mojom::kDeleteFlagRecursive);
  if (error != base::File::Error::FILE_OK)
    RecordFileError(leveldb_env::kDeleteDir, error);
  return FilesystemErrorToStatus(error, dirname, leveldb_env::kDeleteDir);
}

Status MojoEnv::GetFileSize(const std::string& fname, uint64_t* file_size) {
  TRACE_EVENT1("leveldb", "MojoEnv::GetFileSize", "fname", fname);
  base::File::Error error = thread_->GetFileSize(dir_, fname, file_size);
  if (error != base::File::Error::FILE_OK)
    RecordFileError(leveldb_env::kGetFileSize, error);
  return FilesystemErrorToStatus(error, fname, leveldb_env::kGetFileSize);
}

Status MojoEnv::RenameFile(const std::string& src, const std::string& target) {
  TRACE_EVENT2("leveldb", "MojoEnv::RenameFile", "src", src, "target", target);
  if (!thread_->FileExists(dir_, src))
    return Status::OK();
  Retrier retrier(leveldb_env::kRenameFile, this);
  base::File::Error error;
  do {
    error = thread_->RenameFile(dir_, src, target);
  } while (error != base::File::Error::FILE_OK &&
           retrier.ShouldKeepTrying(error));
  if (error != base::File::Error::FILE_OK)
    RecordFileError(leveldb_env::kRenameFile, error);
  return FilesystemErrorToStatus(error, src, leveldb_env::kRenameFile);
}

Status MojoEnv::LockFile(const std::string& fname, FileLock** lock) {
  TRACE_EVENT1("leveldb", "MojoEnv::LockFile", "fname", fname);

  Retrier retrier(leveldb_env::kLockFile, this);
  std::pair<base::File::Error, LevelDBMojoProxy::OpaqueLock*> p;
  do {
    p = thread_->LockFile(dir_, fname);
  } while (p.first != base::File::Error::FILE_OK &&
           retrier.ShouldKeepTrying(p.first));

  if (p.first != base::File::Error::FILE_OK)
    RecordFileError(leveldb_env::kLockFile, p.first);

  if (p.second)
    *lock = new MojoFileLock(p.second, fname);

  return FilesystemErrorToStatus(p.first, fname, leveldb_env::kLockFile);
}

Status MojoEnv::UnlockFile(FileLock* lock) {
  MojoFileLock* my_lock = reinterpret_cast<MojoFileLock*>(lock);

  std::string fname = my_lock ? my_lock->name() : "(invalid)";
  TRACE_EVENT1("leveldb", "MojoEnv::UnlockFile", "fname", fname);

  base::File::Error error = thread_->UnlockFile(my_lock->TakeLock());
  if (error != base::File::Error::FILE_OK)
    RecordFileError(leveldb_env::kUnlockFile, error);
  delete my_lock;
  return FilesystemErrorToStatus(error, fname, leveldb_env::kUnlockFile);
}

Status MojoEnv::GetTestDirectory(std::string* path) {
  // TODO(erg): This method is actually only used from the test harness in
  // leveldb. And when we go and port that test stuff to a
  // service_manager::ServiceTest,
  // we probably won't use it since the mojo filesystem actually handles
  // temporary filesystems just fine.
  NOTREACHED();
  return Status::OK();
}

Status MojoEnv::NewLogger(const std::string& fname, Logger** result) {
  TRACE_EVENT1("leveldb", "MojoEnv::NewLogger", "fname", fname);
  base::File f(thread_->OpenFileHandle(
      dir_, fname,
      filesystem::mojom::kCreateAlways | filesystem::mojom::kFlagWrite));
  if (!f.IsValid()) {
    *result = nullptr;
    RecordOSError(leveldb_env::kNewLogger, f.error_details());
    return MakeIOError(fname, "Unable to create log file",
                       leveldb_env::kNewLogger, f.error_details());
  }
  *result = new leveldb::ChromiumLogger(std::move(f));
  return Status::OK();
}

uint64_t MojoEnv::NowMicros() {
  return base::TimeTicks::Now().ToInternalValue();
}

void MojoEnv::SleepForMicroseconds(int micros) {
  // Round up to the next millisecond.
  base::PlatformThread::Sleep(base::TimeDelta::FromMicroseconds(micros));
}

void MojoEnv::Schedule(void (*function)(void* arg), void* arg) {
  base::PostTaskWithTraits(FROM_HERE,
                           {base::MayBlock(), base::WithBaseSyncPrimitives(),
                            base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
                           base::Bind(function, arg));
}

void MojoEnv::StartThread(void (*function)(void* arg), void* arg) {
  new Thread(function, arg);  // Will self-delete.
}

void MojoEnv::RecordErrorAt(leveldb_env::MethodID method) const {
  UMA_HISTOGRAM_ENUMERATION("MojoLevelDBEnv.IOError", method,
                            leveldb_env::kNumEntries);
}

void MojoEnv::RecordOSError(leveldb_env::MethodID method,
                            base::File::Error error) const {
  RecordErrorAt(method);
  std::string uma_name =
      std::string("MojoLevelDBEnv.IOError.BFE.") + MethodIDToString(method);
  base::UmaHistogramExactLinear(uma_name, -error, -base::File::FILE_ERROR_MAX);
}

void MojoEnv::RecordBytesRead(int amount) const {
  UMA_HISTOGRAM_COUNTS_10M("Storage.BytesRead.MojoLevelDBEnv", amount);
}

void MojoEnv::RecordBytesWritten(int amount) const {
  UMA_HISTOGRAM_COUNTS_10M("Storage.BytesWritten.MojoLevelDBEnv", amount);
}

int MojoEnv::MaxRetryTimeMillis() const {
  return 1000;
}

void MojoEnv::RecordRetryTime(leveldb_env::MethodID method,
                              base::TimeDelta time) const {
  std::string uma_name = std::string("MojoLevelDBEnv.TimeUntilSuccessFor") +
                         MethodIDToString(method);
  UmaHistogramCustomTimes(uma_name, time, base::TimeDelta::FromMilliseconds(1),
                          base::TimeDelta::FromMilliseconds(1001), 42);
}

void MojoEnv::RecordRecoveredFromError(leveldb_env::MethodID method,
                                       base::File::Error error) const {
  std::string uma_name =
      std::string("MojoLevelDBEnv.RetryRecoveredFromErrorIn") +
      MethodIDToString(method);
  base::UmaHistogramExactLinear(uma_name, -error, -base::File::FILE_ERROR_MAX);
}

void MojoEnv::RecordFileError(leveldb_env::MethodID method,
                              base::File::Error error) const {
  RecordOSError(method, static_cast<base::File::Error>(error));
}

}  // namespace leveldb
