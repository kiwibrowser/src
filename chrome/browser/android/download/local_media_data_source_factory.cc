// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/local_media_data_source_factory.h"

#include <vector>

#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop_current.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace {

using MediaDataCallback =
    SafeMediaMetadataParser::MediaDataSourceFactory::MediaDataCallback;
using ReadFileCallback = base::OnceCallback<void(bool, std::vector<char>)>;

// Reads a chunk of the file on a file thread, and reply the data or error to
// main thread.
void ReadFile(const base::FilePath& file_path,
              int64_t position,
              int64_t length,
              scoped_refptr<base::SequencedTaskRunner> main_task_runner,
              ReadFileCallback cb) {
  base::File file(file_path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
  if (!file.IsValid()) {
    main_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(cb), false /*success*/, std::vector<char>()));
    return;
  }

  auto buffer = std::vector<char>(length);
  int bytes_read = file.Read(position, buffer.data(), length);
  if (bytes_read == -1) {
    main_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(cb), false /*success*/, std::vector<char>()));
    return;
  }
  DCHECK_GE(bytes_read, 0);
  if (bytes_read < length)
    buffer.resize(bytes_read);

  main_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(cb), true /*success*/, std::move(buffer)));
}

// Read media file incrementally and send data to the utility process to parse
// media metadata. Must live and die on main thread and does blocking IO on
// |file_task_runner_|.
class LocalMediaDataSource : public chrome::mojom::MediaDataSource {
 public:
  LocalMediaDataSource(
      chrome::mojom::MediaDataSourcePtr* interface,
      const base::FilePath& file_path,
      scoped_refptr<base::SequencedTaskRunner> file_task_runner,
      MediaDataCallback media_data_callback)
      : file_path_(file_path),
        file_task_runner_(file_task_runner),
        media_data_callback_(media_data_callback),
        binding_(this, mojo::MakeRequest(interface)),
        weak_ptr_factory_(this) {}
  ~LocalMediaDataSource() override = default;

 private:
  // chrome::mojom::MediaDataSource implementation.
  void Read(int64_t position,
            int64_t length,
            chrome::mojom::MediaDataSource::ReadCallback callback) override {
    DCHECK(!ipc_read_callback_);
    ipc_read_callback_ = std::move(callback);

    // Read file on a file thread.
    ReadFileCallback read_file_done = base::BindOnce(
        &LocalMediaDataSource::OnReadFileDone, weak_ptr_factory_.GetWeakPtr());
    file_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&ReadFile, file_path_, position, length,
                       base::MessageLoopCurrent::Get()->task_runner(),
                       std::move(read_file_done)));
  }

  void OnReadFileDone(bool success, std::vector<char> buffer) {
    // TODO(xingliu): Handle file IO error when success is false, the IPC
    // channel for chrome::mojom::MediaParser should be closed.
    DCHECK(ipc_read_callback_);
    media_data_callback_.Run(
        std::move(ipc_read_callback_),
        std::make_unique<std::string>(buffer.begin(), buffer.end()));
  }

  base::FilePath file_path_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  // Called when a chunk of the file is read.
  MediaDataCallback media_data_callback_;

  // Pass through callback that is used to send data across IPC channel.
  chrome::mojom::MediaDataSource::ReadCallback ipc_read_callback_;

  mojo::Binding<chrome::mojom::MediaDataSource> binding_;
  base::WeakPtrFactory<LocalMediaDataSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocalMediaDataSource);
};

}  // namespace

LocalMediaDataSourceFactory::LocalMediaDataSourceFactory(
    const base::FilePath& file_path,
    scoped_refptr<base::SequencedTaskRunner> file_task_runner)
    : file_path_(file_path), file_task_runner_(file_task_runner) {}

LocalMediaDataSourceFactory::~LocalMediaDataSourceFactory() = default;

std::unique_ptr<chrome::mojom::MediaDataSource>
LocalMediaDataSourceFactory::CreateMediaDataSource(
    chrome::mojom::MediaDataSourcePtr* request,
    MediaDataCallback media_data_callback) {
  return std::make_unique<LocalMediaDataSource>(
      request, file_path_, file_task_runner_, media_data_callback);
}
