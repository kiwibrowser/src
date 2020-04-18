// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_DRIVER_H_
#define CHROMEOS_BINDER_DRIVER_H_

#include <stddef.h>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"

namespace binder {

// Use this class to communicate with the binder driver provided by the kernel.
// This class is stateless and it's safe to access this class from multiple
// threads.
class CHROMEOS_EXPORT Driver {
 public:
  Driver();
  ~Driver();

  // Opens the binder driver, performs necessary initialization, and returns
  // true on success.
  // This method must be called before calling any other method.
  bool Initialize();

  // Returns the file descriptor of the binder driver.
  int GetFD();

  // Sets the maximum number of additional service threads.
  // By default the number is set to 0.
  bool SetMaxThreads(int max_threads);

  // Writes and reads data to/from the driver.
  // The data in the buffer specified by write_buf and write_buf_size will be
  // written, and the number of bytes written will be stored in
  // num_written_bytes. The read data will be stored in the buffer specified by
  // read_buf and read_buf_size, and the number of bytes read will be stored in
  // num_read_bytes.
  bool WriteRead(const char* write_buf,
                 size_t write_buf_size,
                 char* read_buf,
                 size_t read_buf_size,
                 size_t* num_written_bytes,
                 size_t* num_read_bytes);

  // Waits for some data to be available for reading.
  bool Poll();

  // Lets the driver know the current thread is exiting.
  bool NotifyCurrentThreadExiting();

 private:
  // Returns the size of the mmap region for transaction data.
  size_t GetBinderMmapSize() const;

  base::ScopedFD fd_;
  void* mmap_address_;

  DISALLOW_COPY_AND_ASSIGN(Driver);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_DRIVER_H_
