// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_FILE_READER_H_
#define EXTENSIONS_BROWSER_FILE_READER_H_

#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "extensions/common/extension_resource.h"

// This file defines an interface for reading a file asynchronously on a
// background sequence.
// Consider abstracting out a FilePathProvider (ExtensionResource) and moving
// back to chrome/browser/net if other subsystems want to use it.
class FileReader : public base::RefCountedThreadSafe<FileReader> {
 public:
  // Reports success or failure and the data of the file upon success.
  using DoneCallback =
      base::OnceCallback<void(bool, std::unique_ptr<std::string>)>;
  // Lets the caller accomplish tasks on the file data, after the file content
  // has been read.
  // If the file reading doesn't succeed, this will be ignored.
  using OptionalFileSequenceTask = base::OnceCallback<void(std::string*)>;

  FileReader(const extensions::ExtensionResource& resource,
             OptionalFileSequenceTask file_sequence_task,
             DoneCallback done_callback);

  // Called to start reading the file on a background sequence. Upon completion,
  // the callback will be notified of the results.
  void Start();

 private:
  friend class base::RefCountedThreadSafe<FileReader>;

  ~FileReader();

  void ReadFileOnFileSequence();

  extensions::ExtensionResource resource_;
  OptionalFileSequenceTask optional_file_sequence_task_;
  DoneCallback done_callback_;
  const scoped_refptr<base::SingleThreadTaskRunner> origin_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(FileReader);
};

#endif  // EXTENSIONS_BROWSER_FILE_READER_H_
