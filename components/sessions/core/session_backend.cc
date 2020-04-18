// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/core/session_backend.h"

#include <stdint.h>
#include <limits>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"

using base::TimeTicks;

namespace sessions {

// File version number.
static const int32_t kFileCurrentVersion = 1;

// The signature at the beginning of the file = SSNS (Sessions).
static const int32_t kFileSignature = 0x53534E53;

namespace {

// The file header is the first bytes written to the file,
// and is used to identify the file as one written by us.
struct FileHeader {
  int32_t signature;
  int32_t version;
};

// SessionFileReader ----------------------------------------------------------

// SessionFileReader is responsible for reading the set of SessionCommands that
// describe a Session back from a file. SessionFileRead does minimal error
// checking on the file (pretty much only that the header is valid).

class SessionFileReader {
 public:
  typedef sessions::SessionCommand::id_type id_type;
  typedef sessions::SessionCommand::size_type size_type;

  explicit SessionFileReader(const base::FilePath& path)
      : errored_(false),
        buffer_(SessionBackend::kFileReadBufferSize, 0),
        buffer_position_(0),
        available_count_(0) {
    file_.reset(new base::File(
        path, base::File::FLAG_OPEN | base::File::FLAG_READ));
  }
  // Reads the contents of the file specified in the constructor, returning
  // true on success, and filling up |commands| with commands.
  bool Read(std::vector<std::unique_ptr<sessions::SessionCommand>>* commands);

 private:
  // Reads a single command, returning it. A return value of NULL indicates
  // either there are no commands, or there was an error. Use errored_ to
  // distinguish the two. If NULL is returned, and there is no error, it means
  // the end of file was successfully reached.
  std::unique_ptr<sessions::SessionCommand> ReadCommand();

  // Shifts the unused portion of buffer_ to the beginning and fills the
  // remaining portion with data from the file. Returns false if the buffer
  // couldn't be filled. A return value of false only signals an error if
  // errored_ is set to true.
  bool FillBuffer();

  // Whether an error condition has been detected (
  bool errored_;

  // As we read from the file, data goes here.
  std::string buffer_;

  // The file.
  std::unique_ptr<base::File> file_;

  // Position in buffer_ of the data.
  size_t buffer_position_;

  // Number of available bytes; relative to buffer_position_.
  size_t available_count_;

  DISALLOW_COPY_AND_ASSIGN(SessionFileReader);
};

bool SessionFileReader::Read(
    std::vector<std::unique_ptr<sessions::SessionCommand>>* commands) {
  if (!file_->IsValid())
    return false;
  FileHeader header;
  int read_count;
  read_count = file_->ReadAtCurrentPos(reinterpret_cast<char*>(&header),
                                       sizeof(header));
  if (read_count != sizeof(header) || header.signature != kFileSignature ||
      header.version != kFileCurrentVersion)
    return false;

  std::vector<std::unique_ptr<sessions::SessionCommand>> read_commands;
  for (std::unique_ptr<sessions::SessionCommand> command = ReadCommand();
       command && !errored_; command = ReadCommand())
    read_commands.push_back(std::move(command));
  if (!errored_)
    read_commands.swap(*commands);
  return !errored_;
}

std::unique_ptr<sessions::SessionCommand> SessionFileReader::ReadCommand() {
  // Make sure there is enough in the buffer for the size of the next command.
  if (available_count_ < sizeof(size_type)) {
    if (!FillBuffer())
      return nullptr;
    if (available_count_ < sizeof(size_type)) {
      VLOG(1) << "SessionFileReader::ReadCommand, file incomplete";
      // Still couldn't read a valid size for the command, assume write was
      // incomplete and return NULL.
      return nullptr;
    }
  }
  // Get the size of the command.
  size_type command_size;
  memcpy(&command_size, &(buffer_[buffer_position_]), sizeof(command_size));
  buffer_position_ += sizeof(command_size);
  available_count_ -= sizeof(command_size);

  if (command_size == 0) {
    VLOG(1) << "SessionFileReader::ReadCommand, empty command";
    // Empty command. Shouldn't happen if write was successful, fail.
    return nullptr;
  }

  // Make sure buffer has the complete contents of the command.
  if (command_size > available_count_) {
    if (command_size > buffer_.size())
      buffer_.resize((command_size / 1024 + 1) * 1024, 0);
    if (!FillBuffer() || command_size > available_count_) {
      // Again, assume the file was ok, and just the last chunk was lost.
      VLOG(1) << "SessionFileReader::ReadCommand, last chunk lost";
      return nullptr;
    }
  }
  const id_type command_id = buffer_[buffer_position_];
  // NOTE: command_size includes the size of the id, which is not part of
  // the contents of the SessionCommand.
  std::unique_ptr<sessions::SessionCommand> command =
      std::make_unique<sessions::SessionCommand>(
          command_id, command_size - sizeof(id_type));
  if (command_size > sizeof(id_type)) {
    memcpy(command->contents(),
           &(buffer_[buffer_position_ + sizeof(id_type)]),
           command_size - sizeof(id_type));
  }
  buffer_position_ += command_size;
  available_count_ -= command_size;
  return command;
}

bool SessionFileReader::FillBuffer() {
  if (available_count_ > 0 && buffer_position_ > 0) {
    // Shift buffer to beginning.
    memmove(&(buffer_[0]), &(buffer_[buffer_position_]), available_count_);
  }
  buffer_position_ = 0;
  DCHECK(buffer_position_ + available_count_ < buffer_.size());
  int to_read = static_cast<int>(buffer_.size() - available_count_);
  int read_count = file_->ReadAtCurrentPos(&(buffer_[available_count_]),
                                           to_read);
  if (read_count < 0) {
    errored_ = true;
    return false;
  }
  if (read_count == 0)
    return false;
  available_count_ += read_count;
  return true;
}

}  // namespace

// SessionBackend -------------------------------------------------------------

// File names (current and previous) for a type of TAB.
static const char* kCurrentTabSessionFileName = "Current Tabs";
static const char* kLastTabSessionFileName = "Last Tabs";

// File names (current and previous) for a type of SESSION.
static const char* kCurrentSessionFileName = "Current Session";
static const char* kLastSessionFileName = "Last Session";

// static
const int SessionBackend::kFileReadBufferSize = 1024;

SessionBackend::SessionBackend(sessions::BaseSessionService::SessionType type,
                               const base::FilePath& path_to_dir)
    : type_(type),
      path_to_dir_(path_to_dir),
      last_session_valid_(false),
      inited_(false),
      empty_file_(true) {
  // NOTE: this is invoked on the main thread, don't do file access here.
}

void SessionBackend::Init() {
  if (inited_)
    return;

  inited_ = true;

  // Create the directory for session info.
  base::CreateDirectory(path_to_dir_);

  MoveCurrentSessionToLastSession();
}

void SessionBackend::AppendCommands(
    std::vector<std::unique_ptr<sessions::SessionCommand>> commands,
    bool reset_first) {
  Init();
  // Make sure and check current_session_file_, if opening the file failed
  // current_session_file_ will be NULL.
  if ((reset_first && !empty_file_) || !current_session_file_ ||
      !current_session_file_->IsValid()) {
    ResetFile();
  }
  // Need to check current_session_file_ again, ResetFile may fail.
  if (current_session_file_.get() && current_session_file_->IsValid() &&
      !AppendCommandsToFile(current_session_file_.get(), commands)) {
    current_session_file_.reset(nullptr);
  }
  empty_file_ = false;
}

void SessionBackend::ReadLastSessionCommands(
    const base::CancelableTaskTracker::IsCanceledCallback& is_canceled,
    const sessions::BaseSessionService::GetCommandsCallback& callback) {
  if (is_canceled.Run())
    return;

  Init();

  std::vector<std::unique_ptr<sessions::SessionCommand>> commands;
  ReadLastSessionCommandsImpl(&commands);
  callback.Run(std::move(commands));
}

bool SessionBackend::ReadLastSessionCommandsImpl(
    std::vector<std::unique_ptr<sessions::SessionCommand>>* commands) {
  Init();
  SessionFileReader file_reader(GetLastSessionPath());
  return file_reader.Read(commands);
}

void SessionBackend::DeleteLastSession() {
  Init();
  base::DeleteFile(GetLastSessionPath(), false);
}

void SessionBackend::MoveCurrentSessionToLastSession() {
  Init();
  current_session_file_.reset(nullptr);

  const base::FilePath current_session_path = GetCurrentSessionPath();
  const base::FilePath last_session_path = GetLastSessionPath();
  if (base::PathExists(last_session_path))
    base::DeleteFile(last_session_path, false);
  if (base::PathExists(current_session_path))
    last_session_valid_ = base::Move(current_session_path, last_session_path);

  if (base::PathExists(current_session_path))
    base::DeleteFile(current_session_path, false);

  // Create and open the file for the current session.
  ResetFile();
}

bool SessionBackend::ReadCurrentSessionCommandsImpl(
    std::vector<std::unique_ptr<sessions::SessionCommand>>* commands) {
  Init();
  SessionFileReader file_reader(GetCurrentSessionPath());
  return file_reader.Read(commands);
}

bool SessionBackend::AppendCommandsToFile(
    base::File* file,
    const std::vector<std::unique_ptr<sessions::SessionCommand>>& commands) {
  for (auto i = commands.begin(); i != commands.end(); ++i) {
    int wrote;
    const size_type content_size = static_cast<size_type>((*i)->size());
    const size_type total_size =  content_size + sizeof(id_type);
    wrote = file->WriteAtCurrentPos(reinterpret_cast<const char*>(&total_size),
                                    sizeof(total_size));
    if (wrote != sizeof(total_size)) {
      NOTREACHED() << "error writing";
      return false;
    }
    id_type command_id = (*i)->id();
    wrote = file->WriteAtCurrentPos(reinterpret_cast<char*>(&command_id),
                                    sizeof(command_id));
    if (wrote != sizeof(command_id)) {
      NOTREACHED() << "error writing";
      return false;
    }
    if (content_size > 0) {
      wrote = file->WriteAtCurrentPos(reinterpret_cast<char*>((*i)->contents()),
                                      content_size);
      if (wrote != content_size) {
        NOTREACHED() << "error writing";
        return false;
      }
    }
  }
#if defined(OS_CHROMEOS)
  file->Flush();
#endif
  return true;
}

SessionBackend::~SessionBackend() {
  if (current_session_file_) {
    // Destructor performs file IO because file is open in sync mode.
    // crbug.com/112512.
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    current_session_file_.reset();
  }
}

void SessionBackend::ResetFile() {
  DCHECK(inited_);
  if (current_session_file_) {
    // File is already open, truncate it. We truncate instead of closing and
    // reopening to avoid the possibility of scanners locking the file out
    // from under us once we close it. If truncation fails, we'll try to
    // recreate.
    const int header_size = static_cast<int>(sizeof(FileHeader));
    if (current_session_file_->Seek(
            base::File::FROM_BEGIN, header_size) != header_size ||
        !current_session_file_->SetLength(header_size))
      current_session_file_.reset(nullptr);
  }
  if (!current_session_file_)
    current_session_file_.reset(OpenAndWriteHeader(GetCurrentSessionPath()));
  empty_file_ = true;
}

base::File* SessionBackend::OpenAndWriteHeader(const base::FilePath& path) {
  DCHECK(!path.empty());
  std::unique_ptr<base::File> file(new base::File(
      path, base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE |
                base::File::FLAG_EXCLUSIVE_WRITE |
                base::File::FLAG_EXCLUSIVE_READ));
  if (!file->IsValid())
    return nullptr;
  FileHeader header;
  header.signature = kFileSignature;
  header.version = kFileCurrentVersion;
  int wrote = file->WriteAtCurrentPos(reinterpret_cast<char*>(&header),
                                      sizeof(header));
  if (wrote != sizeof(header))
    return nullptr;
  return file.release();
}

base::FilePath SessionBackend::GetLastSessionPath() {
  base::FilePath path = path_to_dir_;
  if (type_ == sessions::BaseSessionService::TAB_RESTORE)
    path = path.AppendASCII(kLastTabSessionFileName);
  else
    path = path.AppendASCII(kLastSessionFileName);
  return path;
}

base::FilePath SessionBackend::GetCurrentSessionPath() {
  base::FilePath path = path_to_dir_;
  if (type_ == sessions::BaseSessionService::TAB_RESTORE)
    path = path.AppendASCII(kCurrentTabSessionFileName);
  else
    path = path.AppendASCII(kCurrentSessionFileName);
  return path;
}

}  // namespace sessions
